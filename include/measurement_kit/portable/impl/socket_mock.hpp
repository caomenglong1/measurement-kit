// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_SOCKET_MOCK_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_SOCKET_MOCK_HPP

#include <assert.h>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

#include <measurement_kit/portable/context.hpp>

namespace mk {
namespace portable {

mk_socket_t Context::MOCK_socket(int domain, int type, int protocol) noexcept {
    auto sock = ::socket(domain, type, protocol);
    assert(sock == -1 || sock >= 0);
    return sock;
}

int Context::MOCK_connect(mk_socket_t sock, const struct sockaddr *endpoint,
        mk_socklen_t endpoint_length) noexcept {
    int rv = ::connect(sock, endpoint, endpoint_length);
    assert(rv == 0 || rv == -1);
    return rv;
}

#ifndef _WIN32
int Context::MOCK_fcntl_void(mk_socket_t sock, int command) noexcept {
    int rv = ::fcntl(sock, command);
    assert(rv == -1 || rv >= 0);
    return rv;
}

int Context::MOCK_fcntl_int(mk_socket_t sock, int command, int value) noexcept {
    int rv = ::fcntl(sock, command, value);
    assert(rv == -1 || rv == 0);
    return rv;
}
#endif

int Context::MOCK_ioctlsocket(
        mk_socket_t sock, long command, unsigned long *argument) noexcept {
#ifdef _WIN32
    auto rv = ::ioctlsocket(sock, command, argument);
    assert(rv == -1 || rv == 0);
    return rv;
#else
    if (command != MK_FIONBIO || argument == nullptr) {
        errno = EINVAL;
        return -1;
    }
    int flags = MOCK_fcntl_void(sock, F_GETFL);
    if (flags == -1) {
        return -1;
    }
    if (*argument == 0) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    return MOCK_fcntl_int(sock, F_SETFL, flags);
#endif
}

int Context::MOCK_getsockopt(mk_socket_t sock, int level, int option_name,
        mk_sockopt_t *option_value, mk_socklen_t *option_len) noexcept {
    auto rv = ::getsockopt(sock, level, option_name, option_value, option_len);
    assert(rv >= -1);
    return rv;
}

mk_ssize_t Context::MOCK_recv(mk_socket_t sock, void *buffer, mk_size_t length,
        int recv_flags) noexcept {
#ifdef _WIN32 // Windows uses `int`
    if (length > INT_MAX) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    auto rv = ::recv(sock, buffer, (int)length, recv_flags);
#else
    auto rv = ::recv(sock, buffer, length, recv_flags);
#endif
    assert(rv == -1 || rv >= 0);
    return rv;
}

mk_ssize_t Context::MOCK_send(mk_socket_t sock, const void *buffer,
        mk_size_t length, int send_flags) noexcept {
#ifdef _WIN32 // Windows uses `int`
    if (length > INT_MAX) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    auto rv = ::send(sock, buffer, (int)length, send_flags);
#else
    auto rv = ::send(sock, buffer, length, send_flags);
#endif
    assert(rv == -1 || rv >= 0);
    return rv;
}

int Context::MOCK_closesocket(mk_socket_t sock) noexcept {
#ifdef _WIN32
    auto rv = ::closesocket(sock);
#else
    auto rv = ::close(sock);
#endif
    assert(rv == 0 || rv == -1);
    return rv;
}

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_SOCKET_MOCK_HPP
