#include "network/PcmBroadcastServer.h"

#include "app/Logger.h"

#include <algorithm>

namespace shareaudio {

Result<void> PcmBroadcastServer::start(std::uint16_t port, AudioMode mode)
{
    StreamHeader stream;
    stream.mode = mode;
    stream.codec = mode == AudioMode::Quality ? StreamCodec::Opus : StreamCodec::PcmS16Le;
    stream.packet_size = mode == AudioMode::Quality ? Defaults::max_opus_frame_bytes : static_cast<std::uint32_t>(packet_size_for_mode(mode));
    return start(port, stream);
}

Result<void> PcmBroadcastServer::start(std::uint16_t port, const StreamHeader& header)
{
    auto encoded_header = ProtocolWriter::make_stream_header(header);
    if (!encoded_header.ok()) {
        return Result<void>::failure(encoded_header.error());
    }
    stream_header_ = encoded_header.value();

    return server_.start(port, [this](std::shared_ptr<TcpSocket> client) {
        auto sent_header = client->send_all(stream_header_);
        if (!sent_header.ok()) {
            Logger::warning("Dropping PCM broadcast client before registration: " + sent_header.error().message);
            client->close();
            std::scoped_lock lock(mutex_);
            ++stats_.dropped_clients;
            return;
        }

        std::scoped_lock lock(mutex_);
        clients_.push_back(std::move(client));
        stats_.connected_clients = clients_.size();
        Logger::info("PCM broadcast client connected.");
    });
}

void PcmBroadcastServer::stop()
{
    server_.stop();
    std::scoped_lock lock(mutex_);
    for (auto& client : clients_) {
        if (client) {
            client->close();
        }
    }
    clients_.clear();
    stats_.connected_clients = 0;
}

void PcmBroadcastServer::broadcast(std::span<const std::uint8_t> packet)
{
    std::scoped_lock lock(mutex_);
    auto remove_from = std::remove_if(clients_.begin(), clients_.end(), [&](const std::shared_ptr<TcpSocket>& client) {
        if (!client || !client->valid()) {
            ++stats_.dropped_clients;
            return true;
        }

        auto result = client->send_all(packet);
        if (!result.ok()) {
            client->close();
            ++stats_.dropped_clients;
            Logger::warning("Dropping PCM broadcast client: " + result.error().message);
            return true;
        }

        stats_.bytes_sent += packet.size();
        return false;
    });

    clients_.erase(remove_from, clients_.end());
    stats_.connected_clients = clients_.size();
}

BroadcastStats PcmBroadcastServer::stats() const
{
    std::scoped_lock lock(mutex_);
    BroadcastStats result = stats_;
    result.connected_clients = clients_.size();
    return result;
}

bool PcmBroadcastServer::running() const
{
    return server_.running();
}

} // namespace shareaudio
