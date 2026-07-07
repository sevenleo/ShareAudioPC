#pragma once

#include "app/Result.h"

#include <asio.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

namespace shareaudio {

class TcpSocket {
public:
    TcpSocket();
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;
    ~TcpSocket();

    static Result<TcpSocket> connect_to(const std::string& host, std::uint16_t port);

    Result<void> send_all(std::span<const std::uint8_t> bytes);
    Result<std::vector<std::uint8_t>> receive_exact(std::size_t byte_count);
    Result<std::vector<std::uint8_t>> receive_with_timeout(std::size_t byte_count, std::uint32_t timeout_ms);
    void close();
    [[nodiscard]] bool valid() const;

private:
    friend class TcpTransmitterServer;

    using Tcp = asio::ip::tcp;

    TcpSocket(std::shared_ptr<asio::io_context> io, std::shared_ptr<Tcp::socket> socket);

    std::shared_ptr<asio::io_context> io_;
    std::shared_ptr<Tcp::socket> socket_;
};

class TcpTransmitterServer {
public:
    using ClientHandler = std::function<void(std::shared_ptr<TcpSocket>)>;

    Result<void> start(std::uint16_t port, ClientHandler handler);
    void stop();
    [[nodiscard]] bool running() const;
    [[nodiscard]] std::size_t accepted_clients() const;

private:
    std::atomic_bool running_ { false };
    std::atomic_size_t accepted_clients_ { 0 };
    std::shared_ptr<asio::io_context> io_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    std::thread accept_thread_;
    std::mutex client_threads_mutex_;
    std::vector<std::thread> client_threads_;
};

class TcpReceiverClient {
public:
    Result<void> connect(const std::string& host, std::uint16_t port);
    void disconnect();
    [[nodiscard]] bool connected() const;
    Result<void> send_all(std::span<const std::uint8_t> bytes);
    Result<std::vector<std::uint8_t>> receive_exact(std::size_t byte_count);

private:
    mutable std::mutex mutex_;
    TcpSocket socket_;
};

} // namespace shareaudio
