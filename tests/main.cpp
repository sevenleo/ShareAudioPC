#include "app/AppController.h"
#include "app/Config.h"
#include "app/SessionController.h"
#include "audio/AudioAbstractions.h"
#include "audio/AudioPipeline.h"
#include "codec/OpusCodec.h"
#include "network/LoopbackTest.h"
#include "network/PcmBroadcastServer.h"
#include "platform/LocalIp.h"
#include "protocol/JitterBuffer.h"
#include "protocol/PcmChunker.h"
#include "protocol/Protocol.h"
#include "storage/RecentDevices.h"
#include "app/SingleInstance.h"
#include "ui/ConsoleUi.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void test_config()
{
    shareaudio::AppConfig config;
    expect(shareaudio::validate(config).ok(), "default config is valid");
    expect(config.audio.bytes_per_frame() == 4, "stereo s16 frame is 4 bytes");
    expect(shareaudio::packet_size_for_mode(shareaudio::AudioMode::Balanced) == 2048, "balanced packet size");
    expect(shareaudio::packet_size_for_mode(shareaudio::AudioMode::Ultrafast) == 1024, "ultrafast packet size");
    expect(shareaudio::parse_audio_mode("quality") == shareaudio::AudioMode::Quality, "parse quality mode");

    config.audio.channels = 1;
    expect(!shareaudio::validate(config).ok(), "mono config is rejected");

    const auto path = std::filesystem::temp_directory_path() / "shareaudio-config-test.json";
    shareaudio::AppConfig saved;
    saved.receiver.host = "192.168.1.55";
    saved.transmitter.mode = shareaudio::AudioMode::Ultrafast;
    saved.receiver.mode = shareaudio::AudioMode::Ultrafast;
    expect(shareaudio::save_config_file(path, saved).ok(), "config saves to JSON");
    auto loaded = shareaudio::load_config_file(path);
    expect(loaded.ok(), "config loads from JSON");
    if (loaded.ok()) {
        expect(loaded.value().receiver.host == "192.168.1.55", "loaded config preserves receiver host");
        expect(loaded.value().transmitter.mode == shareaudio::AudioMode::Ultrafast, "loaded config preserves mode");
    }
    std::filesystem::remove(path);
}

void test_protocol()
{
    std::vector<std::uint8_t> balanced(2048, 1);
    std::vector<std::uint8_t> ultrafast(1024, 2);
    std::vector<std::uint8_t> wrong(7, 3);

    expect(shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Balanced, balanced).ok(), "balanced PCM packet accepted");
    expect(shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Ultrafast, ultrafast).ok(), "ultrafast PCM packet accepted");
    expect(!shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Balanced, wrong).ok(), "wrong PCM packet rejected");

    auto header = shareaudio::ProtocolWriter::encode_opus_length(513);
    expect(header[0] == 0x02 && header[1] == 0x01, "opus length is big endian");
    expect(shareaudio::ProtocolReader::decode_opus_length(header).value() == 513, "opus length decodes");
    expect(!shareaudio::ProtocolReader::decode_opus_length(std::vector<std::uint8_t> { 1 }).ok(), "short opus header rejected");
    expect(!shareaudio::ProtocolReader::validate_opus_frame_length(0).ok(), "zero opus frame length rejected");

    std::vector<std::uint8_t> opus_frame(20, 9);
    auto packet = shareaudio::ProtocolWriter::make_opus_packet(opus_frame);
    expect(packet.ok(), "opus packet is created");
    expect(packet.value().size() == 22, "opus packet includes 2 byte header");

    auto stream_header = shareaudio::ProtocolWriter::make_stream_header(shareaudio::AudioMode::Ultrafast);
    expect(stream_header.ok(), "stream header is created");
    auto parsed_header = shareaudio::ProtocolReader::parse_stream_header(stream_header.value());
    expect(parsed_header.ok(), "stream header parses");
    if (parsed_header.ok()) {
        expect(parsed_header.value().mode == shareaudio::AudioMode::Ultrafast, "stream header mode is ultrafast");
        expect(parsed_header.value().codec == shareaudio::StreamCodec::PcmS16Le, "stream header codec is pcm");
        expect(parsed_header.value().sample_rate == 48000, "stream header sample rate is preserved");
        expect(parsed_header.value().packet_size == 1024, "stream header packet size is preserved");
    }

    auto invalid_magic = stream_header.value();
    invalid_magic[0] = 'X';
    expect(!shareaudio::ProtocolReader::parse_stream_header(invalid_magic).ok(), "stream header rejects invalid magic");

    auto invalid_packet_size = stream_header.value();
    invalid_packet_size[9] = 0;
    invalid_packet_size[10] = 9;
    expect(!shareaudio::ProtocolReader::parse_stream_header(invalid_packet_size).ok(), "stream header rejects mismatched packet size");
}

void test_jitter_buffer()
{
    shareaudio::JitterBuffer buffer(8);
    buffer.push(std::vector<std::uint8_t> { 1, 2, 3, 4 });
    expect(buffer.depth_bytes() == 4, "jitter depth after push");
    auto popped = buffer.pop(2);
    expect(popped.size() == 2 && popped[0] == 1 && popped[1] == 2, "jitter pop returns bytes");
    auto underrun = buffer.pop(8);
    expect(underrun.size() == 8, "jitter underrun pads output");
    expect(buffer.metrics().underruns == 1, "jitter underrun is counted");

    buffer.push(std::vector<std::uint8_t> { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    expect(buffer.depth_bytes() == 8, "jitter keeps capacity after oversized push");
    expect(buffer.metrics().overruns >= 1, "jitter overrun is counted");
    buffer.reset();
    expect(buffer.depth_bytes() == 0 && buffer.metrics().underruns == 0, "jitter reset clears metrics");
}

void test_pcm_chunker()
{
    shareaudio::PcmChunker chunker(shareaudio::AudioMode::Ultrafast);
    expect(chunker.packet_size() == 1024, "ultrafast chunker packet size");
    chunker.push(std::vector<std::uint8_t>(1000, 1));
    expect(!chunker.has_packet(), "chunker waits for a full packet");
    chunker.push(std::vector<std::uint8_t>(48, 2));
    expect(chunker.has_packet(), "chunker has full packet after enough bytes");
    auto packet = chunker.pop_packet();
    expect(packet.size() == 1024, "chunker emits exact packet size");
    expect(chunker.buffered_bytes() == 24, "chunker keeps remainder bytes");
    chunker.reset();
    expect(chunker.buffered_bytes() == 0, "chunker reset clears bytes");
}

void test_pcm_pipelines()
{
    shareaudio::PcmTransmitterPipeline transmitter(shareaudio::AudioMode::Balanced, 2);
    transmitter.on_captured_pcm(std::vector<std::uint8_t>(4096, 7));
    auto stats = transmitter.stats();
    expect(stats.bytes_captured == 4096, "transmitter pipeline tracks captured bytes");
    expect(stats.packets_produced == 2, "transmitter pipeline produces balanced packets");
    expect(stats.queued_packets == 2, "transmitter pipeline queues packets");

    std::vector<std::uint8_t> packet;
    expect(transmitter.try_pop_packet(packet), "transmitter pipeline pops first packet");
    expect(packet.size() == 2048, "transmitter packet is balanced size");

    shareaudio::NullAudioPlayback playback;
    shareaudio::AudioFormat format;
    expect(playback.initialize(format, "null").ok(), "pipeline playback initializes");
    shareaudio::PcmReceiverPipeline receiver(playback, 4096);
    expect(receiver.start().ok(), "receiver pipeline starts playback");
    expect(receiver.receive_pcm(std::vector<std::uint8_t>(2048, 3)).ok(), "receiver pipeline accepts PCM");
    expect(receiver.pump_playback(1024).ok(), "receiver pipeline pumps playback");
    auto receiver_stats = receiver.stats();
    expect(receiver_stats.bytes_received == 2048, "receiver tracks received bytes");
    expect(receiver_stats.bytes_played == 1024, "receiver tracks played bytes");
}

void test_local_ip()
{
    expect(shareaudio::is_loopback_address("localhost"), "localhost is loopback");
    expect(shareaudio::is_loopback_address("127.0.0.1"), "127.0.0.1 is loopback");
    expect(shareaudio::is_loopback_address("::1"), "::1 is loopback");
    auto local = shareaudio::is_local_address("127.0.0.1");
    expect(local.ok() && local.value(), "127.0.0.1 is local");
}

void test_recent_devices()
{
    const auto path = std::filesystem::temp_directory_path() / "shareaudio-recent-devices-test.json";
    shareaudio::RecentDevices recent(path, 2);
    expect(recent.load().ok(), "missing recent devices file loads");
    expect(recent.add("192.168.1.10").ok(), "add first recent device");
    expect(recent.add("192.168.1.11").ok(), "add second recent device");
    expect(recent.add("192.168.1.10").ok(), "dedupe recent device");
    expect(recent.entries().size() == 2, "recent devices limit maintained");

    shareaudio::RecentDevices reloaded(path, 2);
    expect(reloaded.load().ok(), "recent devices reload");
    expect(!reloaded.entries().empty() && reloaded.entries().front() == "192.168.1.10", "recent devices order persisted");
    expect(reloaded.clear().ok(), "recent devices clear");
    std::filesystem::remove(path);
}

void test_app_controller()
{
    shareaudio::AppController controller;
    expect(controller.set_audio_mode(shareaudio::AudioMode::Ultrafast).ok(), "mode can be set while idle");
    expect(controller.start_transmitter().ok(), "transmitter can start");
    expect(!controller.connect_receiver("192.168.1.20").ok(), "receiver cannot start while transmitter active");
    expect(controller.stop_transmitter().ok(), "transmitter can stop");
    expect(!controller.connect_receiver("127.0.0.1").ok(), "self connection is blocked");
}

void test_session_controller()
{
    shareaudio::AppConfig config;
    config.transmitter.network.port = 0;
    config.receiver.port = 0;
    shareaudio::SessionControllerOptions options;
    options.backend = shareaudio::SessionAudioBackend::Fake;
    options.recent_devices_path = std::filesystem::temp_directory_path() / "shareaudio-session-recent-test.json";
    shareaudio::SessionController session(config, options);

    expect(session.start_sharing(shareaudio::AudioMode::Quality).error().code == shareaudio::ErrorCode::NotSupported, "session rejects quality sharing");
    expect(session.start_sharing(shareaudio::AudioMode::Balanced).ok(), "session starts fake sharing");
    expect(session.start_listening("192.168.1.50").error().code == shareaudio::ErrorCode::InvalidState, "session blocks listening while sharing");
    auto sharing_status = session.status_snapshot();
    expect(sharing_status.mode == shareaudio::SessionMode::Sharing, "session status is sharing");
    expect(session.stop_sharing().ok(), "session stops fake sharing");
    expect(session.stop_sharing().ok(), "session sharing stop is idempotent");

    shareaudio::SessionController self_blocking(config, options);
    expect(!self_blocking.start_listening("127.0.0.1").ok(), "session blocks self connection");
}

void test_session_controller_listen_loopback()
{
    constexpr std::uint16_t port = 39093;

    shareaudio::PcmBroadcastServer server;
    auto started = server.start(port, shareaudio::AudioMode::Balanced);
    expect(started.ok(), started.ok() ? "session loopback server started" : started.error().message);
    if (!started.ok()) {
        return;
    }

    shareaudio::AppConfig config;
    config.receiver.port = port;
    config.transmitter.network.port = port;
    shareaudio::SessionControllerOptions options;
    options.backend = shareaudio::SessionAudioBackend::Fake;
    options.allow_self_connection = true;
    options.recent_devices_path = std::filesystem::temp_directory_path() / "shareaudio-session-loopback-recent-test.json";
    shareaudio::SessionController session(config, options);
    expect(session.start_listening("127.0.0.1").ok(), "session starts loopback listening");

    bool listening = false;
    for (int i = 0; i < 50; ++i) {
        if (session.status_snapshot().mode == shareaudio::SessionMode::Listening) {
            listening = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    expect(listening, "session reaches listening state after SAL1 header");
    auto status = session.status_snapshot();
    expect(status.has_detected_mode && status.detected_mode == shareaudio::AudioMode::Balanced, "session autodetects balanced mode");
    expect(session.stop_listening().ok(), "session stops loopback listening");
    expect(session.stop_listening().ok(), "session listening stop is idempotent");
    server.stop();
}

void test_audio_fakes()
{
    shareaudio::NullAudioPlayback playback;
    shareaudio::AudioFormat format;
    expect(playback.initialize(format, "null").ok(), "null playback initializes");
    expect(!playback.submit(std::vector<std::uint8_t> { 1, 2 }).ok(), "null playback rejects submit before start");
    expect(playback.start().ok(), "null playback starts");
    expect(playback.submit(std::vector<std::uint8_t> { 1, 2, 3, 4 }).ok(), "null playback accepts PCM");
    expect(playback.stats().bytes_processed == 4, "null playback tracks bytes");
}

void test_opus_codec()
{
    shareaudio::OpusEncoder encoder;
    shareaudio::OpusDecoder decoder;
    shareaudio::AudioFormat format; // 48kHz, stereo

    auto init_enc = encoder.initialize(format, shareaudio::Defaults::opus_bitrate_bps);
    auto init_dec = decoder.initialize(format);

#if SHAREAUDIO_HAS_LIBOPUS
    expect(init_enc.ok(), "opus encoder initialization succeeds");
    expect(init_dec.ok(), "opus decoder initialization succeeds");

    if (init_enc.ok() && init_dec.ok()) {
        // Create simple 20ms sine wave or dummy data: 48000 Hz * 0.02s = 960 frames.
        // 960 frames * 2 channels * 2 bytes/sample = 3840 bytes.
        std::vector<std::uint8_t> pcm_in(3840);
        for (std::size_t i = 0; i < pcm_in.size() / 2; ++i) {
            std::int16_t sample = static_cast<std::int16_t>(1000.0 * sin(2.0 * 3.14159 * 440.0 * i / 48000.0));
            std::memcpy(&pcm_in[i * 2], &sample, sizeof(sample));
        }

        auto encoded = encoder.encode(pcm_in);
        expect(encoded.ok(), "opus encode succeeds");
        if (encoded.ok()) {
            expect(!encoded.value().empty(), "encoded opus packet is not empty");
            expect(encoded.value().size() < pcm_in.size(), "encoded opus packet is compressed (smaller than raw pcm)");

            auto decoded = decoder.decode(encoded.value());
            expect(decoded.ok(), "opus decode succeeds");
            if (decoded.ok()) {
                expect(decoded.value().size() == pcm_in.size(), "decoded pcm size matches input size");
            }
        }
    }
#else
    expect(init_enc.error().code == shareaudio::ErrorCode::NotSupported, "opus encoder reports not supported without libopus");
    expect(init_dec.error().code == shareaudio::ErrorCode::NotSupported, "opus decoder reports not supported without libopus");
#endif
}

void test_tcp_loopback()
{
    auto result = shareaudio::run_loopback_tcp_self_test();
    expect(result.ok(), result.ok() ? "loopback TCP self-test passed" : result.error().message);
}

void test_pcm_broadcast_server()
{
    constexpr std::uint16_t port = 39092;
    std::vector<std::uint8_t> packet(128, 42);

    shareaudio::PcmBroadcastServer server;
    auto started = server.start(port, shareaudio::AudioMode::Balanced);
    expect(started.ok(), started.ok() ? "broadcast server started" : started.error().message);
    if (!started.ok()) {
        return;
    }

    auto client = shareaudio::TcpSocket::connect_to("127.0.0.1", port);
    expect(client.ok(), client.ok() ? "broadcast client connected" : client.error().message);
    if (!client.ok()) {
        server.stop();
        return;
    }

    for (int i = 0; i < 20 && server.stats().connected_clients == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto received_header = client.value().receive_exact(shareaudio::ProtocolWriter::stream_header_size);
    expect(received_header.ok(), received_header.ok() ? "broadcast client received stream header" : received_header.error().message);
    if (received_header.ok()) {
        auto parsed_header = shareaudio::ProtocolReader::parse_stream_header(received_header.value());
        expect(parsed_header.ok(), parsed_header.ok() ? "broadcast stream header parsed" : parsed_header.error().message);
        if (parsed_header.ok()) {
            expect(parsed_header.value().mode == shareaudio::AudioMode::Balanced, "broadcast stream header mode is balanced");
        }
    }

    server.broadcast(packet);
    auto received = client.value().receive_exact(packet.size());
    expect(received.ok(), received.ok() ? "broadcast client received packet after header" : received.error().message);
    if (received.ok()) {
        expect(received.value() == packet, "broadcast packet content matches");
    }
    expect(server.stats().bytes_sent >= packet.size(), "broadcast server tracks bytes sent");
    server.stop();
}

void test_console_commands()
{
    shareaudio::AppController controller;
    shareaudio::ConsoleUi ui(controller);

    expect(ui.run(std::vector<std::string> { "--status" }) == 2, "old --status command is removed");
    expect(ui.run(std::vector<std::string> { "--list-ips" }) == 2, "old --list-ips command is removed");
    expect(ui.run(std::vector<std::string> { "share", "--mode", "quality" }) == 2, "quality mode is rejected while Opus is unavailable");
    expect(ui.run(std::vector<std::string> { "listen" }) == 2, "listen requires a host");
    expect(ui.run(std::vector<std::string> { "help" }) == 0, "help command succeeds");
}

void test_single_instance()
{
    auto res1 = shareaudio::enforce_single_instance();
    expect(res1.ok(), "first single instance lock succeeds");

    auto res2 = shareaudio::enforce_single_instance();
    expect(res2.ok(), "second single instance lock with same PID succeeds");
}

} // namespace

int main()
{
    test_config();
    test_protocol();
    test_jitter_buffer();
    test_pcm_chunker();
    test_pcm_pipelines();
    test_local_ip();
    test_recent_devices();
    test_app_controller();
    test_session_controller();
    test_session_controller_listen_loopback();
    test_audio_fakes();
    test_opus_codec();
    test_tcp_loopback();
    test_pcm_broadcast_server();
    test_console_commands();
    test_single_instance();

    if (failures != 0) {
        std::cerr << failures << " test expectation(s) failed.\n";
        return 1;
    }
    std::cout << "All ShareAudioLite tests passed.\n";
    return 0;
}
