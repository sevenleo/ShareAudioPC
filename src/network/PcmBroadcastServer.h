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
    mutable std::mutex mutex_;
    TcpTransmitterServer server_;
    std::vector<std::shared_ptr<TcpSocket>> clients_;
    BroadcastStats stats_;
    std::array<std::uint8_t, ProtocolWriter::stream_header_size> stream_header_ {};
};

} // namespace shareaudio
