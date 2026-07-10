#pragma once

#include "network/TcpSocket.h"
#include "protocol/Protocol.h"

#include <memory>
#include <mutex>
#include <vector>

namespace shareaudio {

struct BroadcastStats {
    std::size_t connected_clients {};
    std::size_t bytes_sent {};
    std::size_t dropped_clients {};
};

class PcmBroadcastServer {
public:
    Result<void> start(std::uint16_t port, AudioMode mode);
    Result<void> start(std::uint16_t port, const StreamHeader& header);
    void stop();
    void broadcast(std::span<const std::uint8_t> packet);

    [[nodiscard]] BroadcastStats stats() const;
    [[nodiscard]] bool running() const;

private:
    struct StreamClient;

    void register_stream_client(std::shared_ptr<TcpSocket> client);
    [[nodiscard]] std::size_t active_client_count_locked() const;

    mutable std::mutex mutex_;
    TcpTransmitterServer server_;
    std::vector<std::shared_ptr<StreamClient>> clients_;
    BroadcastStats stats_;
    StreamHeader active_header_ {};
    std::array<std::uint8_t, ProtocolWriter::stream_header_size> stream_header_ {};
};

} // namespace shareaudio
