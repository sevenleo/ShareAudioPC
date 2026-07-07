#include "network/TcpSocket.h"

#include "app/Logger.h"

#include <chrono>
#include <string>

namespace shareaudio {
namespace {

std::string asio_error_message(const std::string& context, const asio::error_code& error)
{
    return context + " failed: " + error.message();
}

} // namespace

TcpSocket::TcpSocket()
    : io_(std::make_shared<asio::io_context>())
    , socket_(std::make_shared<Tcp::socket>(*io_))
{
}

TcpSocket::TcpSocket(std::shared_ptr<asio::io_context> io, std::shared_ptr<Tcp::socket> socket)
    : io_(std::move(io))
    , socket_(std::move(socket))
{
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : io_(std::move(other.io_))
    , socket_(std::move(other.socket_))
{
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept
{
    if (this != &other) {
        close();
        io_ = std::move(other.io_);
        socket_ = std::move(other.socket_);
    }
    return *this;
}

TcpSocket::~TcpSocket()
{
    close();
}

Result<TcpSocket> TcpSocket::connect_to(const std::string& host, std::uint16_t port)
{
    auto io = std::make_shared<asio::io_context>();
    auto socket = std::make_shared<Tcp::socket>(*io);

    asio::error_code error;
    Tcp::resolver resolver(*io);
    auto endpoints = resolver.resolve(host, std::to_string(port), error);
    if (error) {
        return Result<TcpSocket>::failure(make_error(ErrorCode::NetworkError, asio_error_message("resolve", error)));
    }

    asio::steady_timer timer(*io);
    asio::error_code connect_error = asio::error::would_block;

    asio::async_connect(*socket, endpoints, [&](const asio::error_code& async_error, const Tcp::endpoint&) {
        connect_error = async_error;
        timer.cancel();
    });

    timer.expires_after(std::chrono::seconds(5));
    timer.async_wait([&](const asio::error_code& timer_error) {
        if (!timer_error && connect_error == asio::error::would_block) {
            connect_error = asio::error::timed_out;
            asio::error_code ignored;
            socket->close(ignored);
        }
    });

    io->run();
    if (connect_error == asio::error::timed_out) {
        return Result<TcpSocket>::failure(make_error(ErrorCode::Timeout, "Connection timed out."));
    }
    if (connect_error) {
        return Result<TcpSocket>::failure(make_error(ErrorCode::NetworkError, asio_error_message("connect", connect_error)));
    }

    return Result<TcpSocket>::success(TcpSocket(std::move(io), std::move(socket)));
}

Result<void> TcpSocket::send_all(std::span<const std::uint8_t> bytes)
{
    if (!valid()) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, "Cannot send on a closed socket."));
    }

    asio::error_code error;
    asio::write(*socket_, asio::buffer(bytes.data(), bytes.size()), error);
    if (error) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, asio_error_message("send", error)));
    }
    return Result<void>::success();
}

Result<std::vector<std::uint8_t>> TcpSocket::receive_exact(std::size_t byte_count)
{
    if (!valid()) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NetworkError, "Cannot receive on a closed socket."));
    }

    std::vector<std::uint8_t> output(byte_count);
    asio::error_code error;
    asio::read(*socket_, asio::buffer(output.data(), output.size()), error);
    if (error) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NetworkError, asio_error_message("receive", error)));
    }
    return Result<std::vector<std::uint8_t>>::success(std::move(output));
}

void TcpSocket::close()
{
    if (!socket_) {
        return;
    }

    asio::error_code ignored;
    if (socket_->is_open()) {
        socket_->shutdown(Tcp::socket::shutdown_both, ignored);
        socket_->close(ignored);
    }
}

bool TcpSocket::valid() const
{
    return socket_ && socket_->is_open();
}

Result<void> TcpTransmitterServer::start(std::uint16_t port, ClientHandler handler)
{
    if (running_) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "TCP server is already running."));
    }

    io_ = std::make_shared<asio::io_context>();
    asio::error_code error;
    const TcpSocket::Tcp::endpoint endpoint(TcpSocket::Tcp::v4(), port);
    acceptor_ = std::make_unique<TcpSocket::Tcp::acceptor>(*io_);
    acceptor_->open(endpoint.protocol(), error);
    if (error) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, asio_error_message("open", error)));
    }

    acceptor_->set_option(TcpSocket::Tcp::acceptor::reuse_address(true), error);
    if (error) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, asio_error_message("set reuse_address", error)));
    }

    acceptor_->bind(endpoint, error);
    if (error) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, asio_error_message("bind", error)));
    }

    acceptor_->listen(asio::socket_base::max_listen_connections, error);
    if (error) {
        return Result<void>::failure(make_error(ErrorCode::NetworkError, asio_error_message("listen", error)));
    }

    running_ = true;
    accept_thread_ = std::thread([this, handler = std::move(handler)] {
        while (running_) {
            auto client_socket = std::make_shared<TcpSocket::Tcp::socket>(*io_);
            asio::error_code accept_error;
            acceptor_->accept(*client_socket, accept_error);

            if (accept_error) {
                if (running_) {
                    Logger::warning(asio_error_message("accept", accept_error));
                }
                continue;
            }

            ++accepted_clients_;
            std::shared_ptr<TcpSocket> socket(new TcpSocket(io_, std::move(client_socket)));
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

    if (acceptor_) {
        asio::error_code ignored;
        acceptor_->cancel(ignored);
        acceptor_->close(ignored);
    }
    if (io_) {
        io_->stop();
    }

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
    acceptor_.reset();
    io_.reset();
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
