#include "platform/LocalIp.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>

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

std::string normalize_host(std::string host)
{
    if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
        host = host.substr(1, host.size() - 2);
    }
    std::ranges::transform(host, host.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return host;
}

void add_addrinfo_results(std::set<std::string>& addresses, const char* host)
{
    addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* results = nullptr;
    if (getaddrinfo(host, nullptr, &hints, &results) != 0) {
        return;
    }

    char buffer[INET6_ADDRSTRLEN] {};
    for (addrinfo* item = results; item != nullptr; item = item->ai_next) {
        void* addr_ptr = nullptr;
        if (item->ai_family == AF_INET) {
            auto* ipv4 = reinterpret_cast<sockaddr_in*>(item->ai_addr);
            addr_ptr = &(ipv4->sin_addr);
        } else if (item->ai_family == AF_INET6) {
            auto* ipv6 = reinterpret_cast<sockaddr_in6*>(item->ai_addr);
            addr_ptr = &(ipv6->sin6_addr);
        }

        if (addr_ptr != nullptr && inet_ntop(item->ai_family, addr_ptr, buffer, sizeof(buffer)) != nullptr) {
            addresses.insert(buffer);
        }
    }

    freeaddrinfo(results);
}

} // namespace

bool is_loopback_address(const std::string& host)
{
    const std::string normalized = normalize_host(host);
    return normalized == "localhost"
        || normalized == "::1"
        || normalized == "0:0:0:0:0:0:0:1"
        || normalized.rfind("127.", 0) == 0;
}

Result<std::vector<std::string>> list_local_ip_addresses()
{
    auto runtime = ensure_socket_runtime();
    if (!runtime.ok()) {
        return Result<std::vector<std::string>>::failure(runtime.error());
    }

    std::set<std::string> addresses;
    addresses.insert("127.0.0.1");
    addresses.insert("::1");

    char hostname[256] {};
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        add_addrinfo_results(addresses, hostname);
    }

    return Result<std::vector<std::string>>::success(std::vector<std::string>(addresses.begin(), addresses.end()));
}

Result<bool> is_local_address(const std::string& host)
{
    if (is_loopback_address(host)) {
        return Result<bool>::success(true);
    }

    auto local_ips = list_local_ip_addresses();
    if (!local_ips.ok()) {
        return Result<bool>::failure(local_ips.error());
    }

    const std::string normalized = normalize_host(host);
    const auto& addresses = local_ips.value();
    const bool found = std::ranges::any_of(addresses, [&](const std::string& address) {
        return normalize_host(address) == normalized;
    });
    return Result<bool>::success(found);
}

} // namespace shareaudio
