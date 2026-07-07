#include "network/LoopbackTest.h"

#include "network/TcpSocket.h"

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace shareaudio {

Result<void> run_loopback_tcp_self_test()
{
    constexpr std::uint16_t port = 39091;
    const std::vector<std::uint8_t> expected { 1, 3, 5, 7, 9, 11 };

    TcpTransmitterServer server;
    auto started = server.start(port, [&](std::shared_ptr<TcpSocket> socket) {
        auto received = socket->receive_exact(expected.size());
        if (received.ok()) {
            socket->send_all(received.value());
        }
    });
    if (!started.ok()) {
        return started;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto client = TcpSocket::connect_to("127.0.0.1", port);
    if (!client.ok()) {
        server.stop();
        return Result<void>::failure(client.error());
    }

    auto sent = client.value().send_all(expected);
    if (!sent.ok()) {
        server.stop();
        return sent;
    }

    auto echoed = client.value().receive_exact(expected.size());
    server.stop();
    if (!echoed.ok()) {
        return Result<void>::failure(echoed.error());
    }
    if (echoed.value() != expected) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, "Loopback TCP echo returned unexpected bytes."));
    }

    return Result<void>::success();
}

} // namespace shareaudio
