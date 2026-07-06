#include "network/TcpSocket.h"

#include "app/Logger.h"

#include <cstring>
#include <cerrno>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace shareaudio {
namespace {

#ifdef _WIN32
constexpr NativeSocket invalid_socket_value = static_cast<NativeSocket>(INVALID_SOCKET);
#else
constexpr NativeSocket invalid_socket_value = -1;
#endif

Result<void> ensure_socket_runtime()
{
#ifdef _WIN32
    static bool initialized = false;
    if (!initialized) {
        WSADATA data {};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            return Result<void>::failure(make_error(ErrorCode::NetworkError, "Failed to initialize Winsock."));
        }
        initialized = true;
    }
#endif
    return Result<void>::success();
}

void close_native_socket(NativeSocket socket)
{
    if (socket == invalid_socket_value) {
        return;
    }
#ifdef _WIN32
    closesocket(static_cast<SOCKET>(socket));
#else
    close(socket);
#endif
}

auto system_socket(NativeSocket socket)
{
#ifdef _WIN32
    return static_cast<SOCKET>(socket);
#else
    return socket;
#endif
}

std::string last_socket_error_message(const std::string& context)
{
#ifdef _WIN32
    return context + " failed with Winsock error " + std::to_string(WSAGetLastError()) + ".";
#else
    return context + " failed: " + std::strerror(errno);
#endif
}

} // namespace

TcpSocket::TcpSocket()
    : socket_(invalid_socket_value)
{
}

TcpSocket::TcpSocket(NativeSocket socket)
    : socket_(socket)
{
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : socket_(other.socket_)
{
    other.socket_ = invalid_socket_value;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept
{
    if (this != &other) {
        close();
        socket_ = other.socket_;
        other.socket_ = invalid_socket_value;
    }
    return *this;
}

TcpSocket::~TcpSocket()
{
    close();
}

Result<TcpSocket> TcpSocket::connect_to(const std::string& host, std::uint16_t port)
{
    auto runtime = ensure_socket_runtime();
    if (!runtime.ok()) {
        return Result<TcpSocket>::failure(runtime.error());
    }

    addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* results = nullptr;
    const std::string port_text = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_text.c_str(), &hints, &results) != 0) {
        return Result<TcpSocket>::failure(make_error(ErrorCode::NetworkError, "Unable to resolve host: " + host));
    }

    NativeSocket connected = invalid_socket_value;
    for (addrinfo* item = results; item != nullptr; item = item->ai_next) {
        const auto candidate = static_cast<NativeSocket>(socket(item->ai_family, item->ai_socktype, item->ai_protocol));
        if (candidate == invalid_socket_value) {
            continue;
        }

        if (::connect(system_socket(candidate), item->ai_addr, static_cast<int>(item->ai_addrlen)) == 0) {
            connected = candidate;
            break;
        }
        close_native_socket(candidate);
    }

    freeaddrinfo(results);

    if (connected == invalid_socket_value) {
        return Result<TcpSocket>::failure(make_error(ErrorCode::NetworkError, "Unable to connect to " + host + ":" + port_text));
    }

    return Result<TcpSocket>::success(TcpSocket(connected));
}

Result<void> TcpSocket::send_all(std::span<const std::uint8_t> bytes)
{
    if (!valid()) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, "Cannot send on a closed socket."));
    }

    std::size_t sent = 0;
    while (sent < bytes.size()) {
        const auto remaining = static_cast<int>(bytes.size() - sent);
        const int rc = send(system_socket(socket_), reinterpret_cast<const char*>(bytes.data() + sent), remaining, 0);
        if (rc <= 0) {
            return Result<void>::failure(make_error(ErrorCode::NetworkError, last_socket_error_message("send")));
        }
        sent += static_cast<std::size_t>(rc);
    }
    return Result<void>::success();
}

Result<std::vector<std::uint8_t>> TcpSocket::receive_exact(std::size_t byte_count)
{
    if (!valid()) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NetworkError, "Cannot receive on a closed socket."));
    }

    std::vector<std::uint8_t> output(byte_count);
    std::size_t received = 0;
    while (received < byte_count) {
        const auto remaining = static_cast<int>(byte_count - received);
        const int rc = recv(system_socket(socket_), reinterpret_cast<char*>(output.data() + received), remaining, 0);
        if (rc == 0) {
            return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NetworkError, "Socket disconnected before enough bytes were received."));
        }
        if (rc < 0) {
            return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NetworkError, last_socket_error_message("recv")));
        }
        received += static_cast<std::size_t>(rc);
    }
    return Result<std::vector<std::uint8_t>>::success(std::move(output));
}

void TcpSocket::close()
{
    close_native_socket(socket_);
    socket_ = invalid_socket_value;
}

bool TcpSocket::valid() const
{
    return socket_ != invalid_socket_value;
}

Result<void> TcpTransmitterServer::start(std::uint16_t port, ClientHandler handler)
{
    auto runtime = ensure_socket_runtime();
    if (!runtime.ok()) {
        return runtime;
    }
    if (running_) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "TCP server is already running."));
    }

    listen_socket_ = static_cast<NativeSocket>(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (listen_socket_ == invalid_socket_value) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, last_socket_error_message("socket")));
    }

    int yes = 1;
    setsockopt(system_socket(listen_socket_), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(system_socket(listen_socket_), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close_native_socket(listen_socket_);
        listen_socket_ = invalid_socket_value;
        return Result<void>::failure(make_error(ErrorCode::NetworkError, last_socket_error_message("bind")));
    }

    if (listen(system_socket(listen_socket_), SOMAXCONN) != 0) {
        close_native_socket(listen_socket_);
        listen_socket_ = invalid_socket_value;
        return Result<void>::failure(make_error(ErrorCode::NetworkError, last_socket_error_message("listen")));
    }

    running_ = true;
    accept_thread_ = std::thread([this, handler = std::move(handler)] {
        while (running_) {
            sockaddr_storage client_address {};
            socklen_t client_size = sizeof(client_address);
            const auto client_socket = static_cast<NativeSocket>(accept(system_socket(listen_socket_), reinterpret_cast<sockaddr*>(&client_address), &client_size));
            if (client_socket == invalid_socket_value) {
                if (running_) {
                    Logger::warning(last_socket_error_message("accept"));
                }
                continue;
            }

            ++accepted_clients_;
            auto socket = std::make_shared<TcpSocket>(client_socket);
            std::scoped_lock lock(client_threads_mutex_);
            client_threads_.emplace_back([handler, socket] {
                handler(socket);
            });
        }
    });

    return Result<void>::success();
}

void TcpTransmitterServer::stop()
{
    running_ = false;
    close_native_socket(listen_socket_);
    listen_socket_ = invalid_socket_value;

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    std::scoped_lock lock(client_threads_mutex_);
    for (auto& thread : client_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    client_threads_.clear();
}

bool TcpTransmitterServer::running() const
{
    return running_;
}

std::size_t TcpTransmitterServer::accepted_clients() const
{
    return accepted_clients_;
}

Result<void> TcpReceiverClient::connect(const std::string& host, std::uint16_t port)
{
    std::scoped_lock lock(mutex_);
    if (socket_.valid()) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "TCP client is already connected."));
    }

    auto connected = TcpSocket::connect_to(host, port);
    if (!connected.ok()) {
        return Result<void>::failure(connected.error());
    }
    socket_ = std::move(connected.value());
    return Result<void>::success();
}

void TcpReceiverClient::disconnect()
{
    std::scoped_lock lock(mutex_);
    socket_.close();
}

bool TcpReceiverClient::connected() const
{
    std::scoped_lock lock(mutex_);
    return socket_.valid();
}

Result<void> TcpReceiverClient::send_all(std::span<const std::uint8_t> bytes)
{
    std::scoped_lock lock(mutex_);
    return socket_.send_all(bytes);
}

Result<std::vector<std::uint8_t>> TcpReceiverClient::receive_exact(std::size_t byte_count)
{
    std::scoped_lock lock(mutex_);
    return socket_.receive_exact(byte_count);
}

} // namespace shareaudio
