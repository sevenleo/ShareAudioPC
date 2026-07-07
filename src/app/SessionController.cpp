#include "app/SessionController.h"

#include "audio/AudioAbstractions.h"
#include "audio/MiniaudioBackend.h"
#include "platform/LocalIp.h"
#include "protocol/Protocol.h"

#include <algorithm>
#include <chrono>

namespace shareaudio {
namespace {

constexpr std::size_t max_log_events = 100;

bool pcm_mode_supported(AudioMode mode)
{
    return mode == AudioMode::Balanced || mode == AudioMode::Ultrafast;
}

} // namespace

SessionController::SessionController(AppConfig config, SessionControllerOptions options)
    : config_(std::move(config))
    , options_(options)
    , recent_(options_.recent_devices_path.empty() ? default_recent_devices_path() : options_.recent_devices_path)
{
    recent_.load();
}

SessionController::~SessionController()
{
    stop();
}

std::unique_ptr<IAudioCapture> SessionController::make_capture() const
{
    if (options_.backend == SessionAudioBackend::Fake) {
        return std::make_unique<GeneratedToneCapture>();
    }
#if SHAREAUDIO_ENABLE_MINIAUDIO
    return std::make_unique<MiniaudioCapture>();
#else
    return std::make_unique<GeneratedToneCapture>();
#endif
}

std::unique_ptr<IAudioPlayback> SessionController::make_playback() const
{
    if (options_.backend == SessionAudioBackend::Fake) {
        return std::make_unique<NullAudioPlayback>();
    }
#if SHAREAUDIO_ENABLE_MINIAUDIO
    return std::make_unique<MiniaudioPlayback>();
#else
    return std::make_unique<NullAudioPlayback>();
#endif
}

Result<void> SessionController::start_sharing(AudioMode mode, std::string capture_device_id)
{
    if (!pcm_mode_supported(mode)) {
        return Result<void>::failure(make_error(ErrorCode::NotSupported, "Quality mode requires Opus implementation."));
    }

    {
        std::scoped_lock lock(mutex_);
        if (mode_ != SessionMode::Idle) {
            return Result<void>::failure(make_error(ErrorCode::InvalidState, "Stop the current session before sharing audio."));
        }
        last_error_.clear();
        has_detected_mode_ = false;
        config_.transmitter.mode = mode;
        config_.receiver.mode = mode;
        config_.transmitter.capture_device_id = capture_device_id;
        mode_ = SessionMode::Sharing;
        add_log_locked("Starting sharing session.");
    }

    auto capture = make_capture();
    auto init_capture = capture->initialize(config_.audio, capture_device_id);
    if (!init_capture.ok()) {
        stop_sharing();
        return Result<void>::failure(init_capture.error());
    }

    auto server = std::make_unique<PcmBroadcastServer>();
    auto start_server = server->start(config_.transmitter.network.port, mode);
    if (!start_server.ok()) {
        stop_sharing();
        return Result<void>::failure(start_server.error());
    }

    auto transmitter = std::make_unique<PcmTransmitterPipeline>(mode);
    auto* transmitter_ptr = transmitter.get();
    auto* server_ptr = server.get();

    {
        std::scoped_lock lock(mutex_);
        capture_ = std::move(capture);
        server_ = std::move(server);
        transmitter_ = std::move(transmitter);
    }

    transmitter_running_ = true;
    transmitter_worker_ = std::thread([this, transmitter_ptr, server_ptr] {
        while (transmitter_running_) {
            std::vector<std::uint8_t> packet;
            if (transmitter_ptr->try_pop_packet(packet)) {
                server_ptr->broadcast(packet);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });

    auto start_capture = capture_->start([transmitter_ptr](std::span<const std::uint8_t> pcm) {
        transmitter_ptr->on_captured_pcm(pcm);
    });
    if (!start_capture.ok()) {
        stop_sharing();
        return Result<void>::failure(start_capture.error());
    }

    add_log("Sharing session started.");
    state_changed_.notify_all();
    return Result<void>::success();
}

Result<void> SessionController::stop_sharing()
{
    {
        std::scoped_lock lock(mutex_);
        if (mode_ != SessionMode::Sharing && !capture_ && !server_ && !transmitter_worker_.joinable()) {
            return Result<void>::success();
        }
        mode_ = SessionMode::Idle;
        add_log_locked("Stopping sharing session.");
    }

    transmitter_running_ = false;
    if (capture_) {
        capture_->stop();
    }
    if (transmitter_worker_.joinable()) {
        transmitter_worker_.join();
    }
    if (server_) {
        server_->stop();
    }

    SessionStatus completed;
    completed.mode = SessionMode::Idle;
    completed.selected_mode = config_.transmitter.mode;
    completed.port = config_.transmitter.network.port;
    if (server_) {
        const auto server_stats = server_->stats();
        completed.connected_clients = server_stats.connected_clients;
        completed.bytes_sent = server_stats.bytes_sent;
        completed.dropped_packets = server_stats.dropped_clients;
    }
    if (transmitter_) {
        const auto transmitter_stats = transmitter_->stats();
        completed.packets_produced = transmitter_stats.packets_produced;
        completed.dropped_packets += transmitter_stats.dropped_packets;
    }

    {
        std::scoped_lock lock(mutex_);
        last_completed_status_ = completed;
        capture_.reset();
        server_.reset();
        transmitter_.reset();
        add_log_locked("Sharing session stopped.");
    }
    state_changed_.notify_all();
    return Result<void>::success();
}

Result<void> SessionController::start_listening(std::string host, std::string playback_device_id)
{
    if (host.empty()) {
        return Result<void>::failure(make_error(ErrorCode::InvalidArgument, "Host/IP is required."));
    }
    if (!options_.allow_self_connection) {
        auto local = is_local_address(host);
        if (!local.ok()) {
            return Result<void>::failure(local.error());
        }
        if (local.value()) {
            return Result<void>::failure(make_error(ErrorCode::InvalidArgument, "Refusing to connect receiver to this same machine."));
        }
    }

    {
        std::scoped_lock lock(mutex_);
        if (mode_ != SessionMode::Idle) {
            return Result<void>::failure(make_error(ErrorCode::InvalidState, "Stop the current session before listening."));
        }
        last_error_.clear();
        has_detected_mode_ = false;
        config_.receiver.host = host;
        config_.receiver.playback_device_id = playback_device_id;
        mode_ = SessionMode::Connecting;
        add_log_locked("Connecting to transmitter.");
    }

    listener_stop_requested_ = false;
    listener_worker_ = std::thread([this, host = std::move(host), playback_device_id = std::move(playback_device_id)] {
        while (!listener_stop_requested_) {
            {
                std::scoped_lock lock(mutex_);
                mode_ = SessionMode::Connecting;
                add_log_locked("Connecting to transmitter...");
            }
            state_changed_.notify_all();

            auto connected = TcpSocket::connect_to(host, config_.receiver.port);
            if (!connected.ok()) {
                set_error("Connection failed: " + connected.error().message + ". Retrying...");
                for (int i = 0; i < 30 && !listener_stop_requested_; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                continue;
            }

            auto socket = std::make_shared<TcpSocket>(std::move(connected.value()));
            {
                std::scoped_lock lock(mutex_);
                receiver_socket_ = socket;
            }

            StreamHeader header;
            bool is_native = false;
            bool is_http = false;
            bool http_parse_success = false;

            auto header_bytes = socket->receive_with_timeout(ProtocolWriter::stream_header_size, 300);
            if (header_bytes.ok()) {
                auto parsed = ProtocolReader::parse_stream_header(header_bytes.value());
                if (parsed.ok()) {
                    header = parsed.value();
                    is_native = true;
                }
            }

            if (!is_native) {
                socket->close();
                auto info_conn = TcpSocket::connect_to(host, config_.receiver.port);
                if (info_conn.ok()) {
                    auto info_socket = std::make_shared<TcpSocket>(std::move(info_conn.value()));
                    std::string req = "GET /info HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
                    auto sent = info_socket->send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(req.data()), req.size()));
                    if (sent.ok()) {
                        std::string res;
                        while (info_socket->valid() && res.size() < 4096) {
                            auto byte = info_socket->receive_with_timeout(1, 1000);
                            if (!byte.ok() || byte.value().empty()) {
                                break;
                            }
                            res.push_back(static_cast<char>(byte.value()[0]));
                        }

                        auto separator = res.find("\r\n\r\n");
                        if (separator != std::string::npos) {
                            std::string body = res.substr(separator + 4);

                            std::string codec_val = "pcm";
                            auto codec_idx = body.find("\"codec\"");
                            if (codec_idx != std::string::npos) {
                                auto colon_idx = body.find(":", codec_idx);
                                if (colon_idx != std::string::npos) {
                                    auto quote1 = body.find("\"", colon_idx);
                                    if (quote1 != std::string::npos) {
                                        auto quote2 = body.find("\"", quote1 + 1);
                                        if (quote2 != std::string::npos) {
                                            codec_val = body.substr(quote1 + 1, quote2 - (quote1 + 1));
                                        }
                                    }
                                }
                            }

                            std::uint32_t chunk_size = 2048;
                            auto chunk_idx = body.find("\"chunkSize\"");
                            if (chunk_idx != std::string::npos) {
                                auto colon_idx = body.find(":", chunk_idx);
                                if (colon_idx != std::string::npos) {
                                    std::string val_str;
                                    for (std::size_t i = colon_idx + 1; i < body.size(); ++i) {
                                        char c = body[i];
                                        if (std::isdigit(c)) {
                                            val_str.push_back(c);
                                        } else if (!val_str.empty() && (std::isspace(c) || c == ',' || c == '}')) {
                                            break;
                                        }
                                    }
                                    if (!val_str.empty()) {
                                        chunk_size = static_cast<std::uint32_t>(std::stoul(val_str));
                                    }
                                }
                            }

                            header.mode = (chunk_size == 1024) ? AudioMode::Ultrafast : AudioMode::Balanced;
                            header.codec = (codec_val == "opus") ? StreamCodec::Opus : StreamCodec::PcmS16Le;
                            header.packet_size = chunk_size;
                            header.channels = 2;
                            header.bytes_per_sample = 2;
                            header.sample_rate = 48000;
                            is_http = true;
                            http_parse_success = true;
                        }
                    }
                    info_socket->close();
                }
            }

            if (!is_native && !http_parse_success) {
                set_error("Failed to parse stream metadata (SAL1 or HTTP /info). Retrying...");
                socket->close();
                for (int i = 0; i < 30 && !listener_stop_requested_; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                continue;
            }

            if (is_http) {
                auto stream_conn = TcpSocket::connect_to(host, config_.receiver.port);
                if (!stream_conn.ok()) {
                    set_error("HTTP stream connection failed: " + stream_conn.error().message + ". Retrying...");
                    for (int i = 0; i < 30 && !listener_stop_requested_; ++i) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    continue;
                }
                socket = std::make_shared<TcpSocket>(std::move(stream_conn.value()));
                {
                    std::scoped_lock lock(mutex_);
                    receiver_socket_ = socket;
                }

                std::string req = "GET /stream HTTP/1.1\r\nHost: " + host + "\r\nConnection: keep-alive\r\n\r\n";
                auto sent = socket->send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(req.data()), req.size()));
                if (!sent.ok()) {
                    set_error("HTTP GET /stream request failed: " + sent.error().message + ". Retrying...");
                    socket->close();
                    for (int i = 0; i < 30 && !listener_stop_requested_; ++i) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    continue;
                }

                std::string http_res;
                bool found_sep = false;
                while (socket->valid() && http_res.size() < 4096) {
                    auto byte = socket->receive_with_timeout(1, 2000);
                    if (!byte.ok() || byte.value().empty()) {
                        break;
                    }
                    http_res.push_back(static_cast<char>(byte.value()[0]));
                    if (http_res.size() >= 4 && http_res.substr(http_res.size() - 4) == "\r\n\r\n") {
                        found_sep = true;
                        break;
                    }
                }

                if (!found_sep) {
                    set_error("HTTP stream response headers malformed. Retrying...");
                    socket->close();
                    for (int i = 0; i < 30 && !listener_stop_requested_; ++i) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    continue;
                }
            }

            if (header.codec != StreamCodec::PcmS16Le || header.mode == AudioMode::Quality) {
                set_error("This build can only listen to balanced or ultrafast PCM streams. Quality mode requires Opus implementation.");
                socket->close();
                break;
            }

            auto playback = make_playback();
            auto init_playback = playback->initialize(config_.audio, playback_device_id);
            if (!init_playback.ok()) {
                set_error("Playback initialization failed: " + init_playback.error().message);
                socket->close();
                break; // Fatal error: do not retry
            }

            auto receiver = std::make_unique<PcmReceiverPipeline>(*playback, header.packet_size * 8);
            auto start_playback = receiver->start();
            if (!start_playback.ok()) {
                set_error("Playback start failed: " + start_playback.error().message);
                socket->close();
                break; // Fatal error: do not retry
            }

            {
                std::scoped_lock lock(mutex_);
                playback_ = std::move(playback);
                receiver_ = std::move(receiver);
                detected_mode_ = header.mode;
                has_detected_mode_ = true;
                mode_ = SessionMode::Listening;
                config_.receiver.mode = header.mode;
                add_log_locked("Listening session started.");
            }
            {
                std::scoped_lock lock(mutex_);
                recent_.add(host);
            }
            state_changed_.notify_all();

            const auto packet_size = static_cast<std::size_t>(header.packet_size);
            bool connection_lost = false;
            while (!listener_stop_requested_) {
                auto packet = socket->receive_exact(packet_size);
                if (!packet.ok()) {
                    if (!listener_stop_requested_) {
                        set_error("Connection lost: " + packet.error().message + ". Retrying...");
                        connection_lost = true;
                    }
                    break;
                }

                Result<void> received = Result<void>::success();
                Result<void> pumped = Result<void>::success();
                {
                    std::scoped_lock lock(mutex_);
                    if (receiver_) {
                        received = receiver_->receive_pcm(packet.value());
                        pumped = receiver_->pump_playback(packet_size);
                    }
                }
                if (!received.ok()) {
                    set_error("Receive error: " + received.error().message + ". Retrying...");
                    connection_lost = true;
                    break;
                }
                if (!pumped.ok()) {
                    set_error("Playback pump error: " + pumped.error().message + ". Retrying...");
                    connection_lost = true;
                    break;
                }
            }

            // Save final stats for this attempt before cleanup
            {
                std::scoped_lock lock(mutex_);
                if (receiver_) {
                    const auto receiver_stats = receiver_->stats();
                    last_completed_status_.bytes_received = receiver_stats.bytes_received;
                    last_completed_status_.bytes_played = receiver_stats.bytes_played;
                    last_completed_status_.underruns = receiver_stats.buffer.underruns + receiver_stats.playback.underruns;
                    last_completed_status_.jitter_buffer_depth = receiver_stats.buffer.depth_bytes;
                }
            }

            // Cleanup for this attempt
            if (playback_) {
                playback_->stop();
            }
            socket->close();

            {
                std::scoped_lock lock(mutex_);
                playback_.reset();
                receiver_.reset();
                receiver_socket_.reset();
                has_detected_mode_ = false;
            }

            if (!connection_lost) {
                break;
            }
        }

        finish_listening();
    });

    state_changed_.notify_all();
    return Result<void>::success();
}

Result<void> SessionController::stop_listening()
{
    {
        std::scoped_lock lock(mutex_);
        if (mode_ != SessionMode::Listening && mode_ != SessionMode::Connecting && !listener_worker_.joinable()) {
            return Result<void>::success();
        }
        listener_stop_requested_ = true;
        if (receiver_socket_) {
            receiver_socket_->close();
        }
        add_log_locked("Stopping listening session.");
    }

    if (listener_worker_.joinable()) {
        listener_worker_.join();
    }
    return Result<void>::success();
}

Result<void> SessionController::stop()
{
    stop_listening();
    stop_sharing();
    return Result<void>::success();
}

std::vector<std::string> SessionController::list_local_ips() const
{
    auto addresses = list_local_ip_addresses();
    if (!addresses.ok()) {
        return {};
    }
    return addresses.value();
}

std::vector<AudioDevice> SessionController::list_capture_devices() const
{
    auto capture = make_capture();
    return capture->devices();
}

std::vector<AudioDevice> SessionController::list_playback_devices() const
{
    auto playback = make_playback();
    return playback->devices();
}

SessionStatus SessionController::status_snapshot() const
{
    std::scoped_lock lock(mutex_);
    SessionStatus status;
    if (mode_ == SessionMode::Idle) {
        status = last_completed_status_;
    }
    status.mode = mode_;
    status.selected_mode = config_.transmitter.mode;
    status.detected_mode = detected_mode_;
    status.has_detected_mode = has_detected_mode_;
    status.host = config_.receiver.host;
    status.port = config_.transmitter.network.port;
    status.last_error = last_error_;
    status.log_events = log_events_;

    if (server_) {
        const auto server_stats = server_->stats();
        status.connected_clients = server_stats.connected_clients;
        status.bytes_sent = server_stats.bytes_sent;
        status.dropped_packets = server_stats.dropped_clients;
    }
    if (transmitter_) {
        const auto transmitter_stats = transmitter_->stats();
        status.packets_produced = transmitter_stats.packets_produced;
        status.dropped_packets += transmitter_stats.dropped_packets;
    }
    if (receiver_) {
        const auto receiver_stats = receiver_->stats();
        status.bytes_received = receiver_stats.bytes_received;
        status.bytes_played = receiver_stats.bytes_played;
        status.underruns = receiver_stats.buffer.underruns + receiver_stats.playback.underruns;
        status.jitter_buffer_depth = receiver_stats.buffer.depth_bytes;
    }
    return status;
}

AppConfig SessionController::config_snapshot() const
{
    std::scoped_lock lock(mutex_);
    return config_;
}

std::vector<std::string> SessionController::recent_devices() const
{
    std::scoped_lock lock(mutex_);
    return recent_.entries();
}

bool SessionController::wait_until_idle(std::chrono::milliseconds timeout)
{
    std::unique_lock lock(mutex_);
    return state_changed_.wait_for(lock, timeout, [this] {
        return mode_ == SessionMode::Idle;
    });
}

void SessionController::set_error(std::string message)
{
    std::scoped_lock lock(mutex_);
    last_error_ = std::move(message);
    if (!last_error_.empty()) {
        add_log_locked("Error: " + last_error_);
    }
}

void SessionController::add_log(std::string message)
{
    std::scoped_lock lock(mutex_);
    add_log_locked(message);
}

void SessionController::add_log_locked(const std::string& message)
{
    log_events_.push_back(message);
    if (log_events_.size() > max_log_events) {
        log_events_.erase(log_events_.begin(), log_events_.begin() + static_cast<std::ptrdiff_t>(log_events_.size() - max_log_events));
    }
}

void SessionController::finish_listening()
{
    SessionStatus completed;
    {
        std::scoped_lock lock(mutex_);
        completed.mode = SessionMode::Idle;
        completed.selected_mode = config_.transmitter.mode;
        completed.detected_mode = detected_mode_;
        completed.has_detected_mode = has_detected_mode_;
        completed.host = config_.receiver.host;
        completed.port = config_.receiver.port;
        completed.last_error = last_error_;
        if (receiver_) {
            const auto receiver_stats = receiver_->stats();
            completed.bytes_received = receiver_stats.bytes_received;
            completed.bytes_played = receiver_stats.bytes_played;
            completed.underruns = receiver_stats.buffer.underruns + receiver_stats.playback.underruns;
            completed.jitter_buffer_depth = receiver_stats.buffer.depth_bytes;
        } else {
            completed.bytes_received = last_completed_status_.bytes_received;
            completed.bytes_played = last_completed_status_.bytes_played;
            completed.underruns = last_completed_status_.underruns;
            completed.jitter_buffer_depth = last_completed_status_.jitter_buffer_depth;
        }
    }

    if (playback_) {
        playback_->stop();
    }
    if (receiver_socket_) {
        receiver_socket_->close();
    }

    {
        std::scoped_lock lock(mutex_);
        playback_.reset();
        receiver_.reset();
        receiver_socket_.reset();
        mode_ = SessionMode::Idle;
        last_completed_status_ = completed;
        add_log_locked("Listening session stopped.");
    }
    state_changed_.notify_all();
}

const char* to_string(SessionMode mode)
{
    switch (mode) {
    case SessionMode::Idle:
        return "idle";
    case SessionMode::Sharing:
        return "sharing";
    case SessionMode::Connecting:
        return "connecting";
    case SessionMode::Listening:
        return "listening";
    }
    return "idle";
}

} // namespace shareaudio
