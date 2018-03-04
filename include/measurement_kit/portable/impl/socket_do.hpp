// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_SOCKET_DO_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_SOCKET_DO_HPP

#include <measurement_kit/portable/context.hpp>

#include <assert.h>

#include <measurement_kit/portable/impl/flags.h>

namespace mk {
namespace portable {

mk_socket_t Context::do_socket(int domain, int type, int protocol) noexcept {
    mk_socket_t sock = MOCK_socket(domain, type, protocol);
    if (sock != -1) {
        unsigned long argument = 1;
        int rv = MOCK_ioctlsocket(sock, MK_FIONBIO, &argument);
        if (rv != 0) {
            MOCK_closesocket(sock);
            return -1;
        }
        sockets_[sock] = 0;
    }
    return sock;
}

int Context::do_connect(mk_socket_t sock, const struct sockaddr *endpoint,
        mk_socklen_t endpoint_length) noexcept {
    if (sockets_.count(sock) == 0) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    auto rv = MOCK_connect(sock, endpoint, endpoint_length);
    if (rv == 0) {
        return 0;
    }
    if (MOCK_get_last_error() != MK_EWOULDBLOCK) {
        return -1;
    }
    if ((sockets_.at(sock) & MK_F_NONBLOCK) != 0) {
        return -1;
    }
    // TODO(bassosimone): we can probably have some default timeout for I/O
    // operations in this case as well as for recv and send. This is especially
    // relevant for connect() where otherwise it's a 75 s operation.
    fd_set writeset;
    FD_ZERO(&writeset);
    FD_SET(sock, &writeset);
    auto ctrl = do_select(sock + 1, nullptr, &writeset, nullptr, nullptr);
    if (ctrl == -1) {
        return -1;
    }
    if (ctrl == 0) {
        MOCK_set_last_error(MK_ETIMEDOUT);
        return -1;
    }
    int real_error_code = 0;
    mk_socklen_t len = sizeof(real_error_code);
    auto getsockopt_err = do_getsockopt(
            sock, SOL_SOCKET, SO_ERROR, (mk_sockopt_t *)&real_error_code, &len);
    if (getsockopt_err != 0) {
        return -1;
    }
    if (real_error_code != 0) {
        MOCK_set_last_error(real_error_code);
        return -1;
    }
    return 0;
}

int Context::do_ioctlsocket(
        mk_socket_t sock, long command, unsigned long *argument) noexcept {
    if (sockets_.count(sock) == 0) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    int rv = MOCK_ioctlsocket(sock, command, argument);
    if (rv == 0) {
        assert(command == MK_FIONBIO);
        sockets_.at(sock) |= MK_F_NONBLOCK;
    }
    return rv;
}

int Context::do_getsockopt(mk_socket_t sock, int level, int option_name,
        mk_sockopt_t *option_value, mk_socklen_t *option_len) noexcept {
    if (sockets_.count(sock) == 0) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    return MOCK_getsockopt(sock, level, option_name, option_value, option_len);
}

mk_ssize_t Context::do_recv(mk_socket_t sock, void *buffer, mk_size_t length,
        int recv_flags) noexcept {
    if (sockets_.count(sock) == 0) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    auto rv = MOCK_recv(sock, buffer, length, recv_flags);
    if (rv >= 0) {
        return rv;
    }
    if (MOCK_get_last_error() != MK_EWOULDBLOCK) {
        return -1;
    }
    if ((sockets_.at(sock) & MK_F_NONBLOCK) != 0) {
        return -1;
    }
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(sock, &readset);
    auto ctrl = do_select(sock + 1, &readset, nullptr, nullptr, nullptr);
    if (ctrl == -1) {
        return -1;
    }
    if (ctrl > 0) {
        return MOCK_recv(sock, buffer, length, recv_flags);
    }
    MOCK_set_last_error(MK_ETIMEDOUT);
    return -1;
}

mk_ssize_t Context::do_send(mk_socket_t sock, const void *buffer,
        mk_size_t length, int send_flags) noexcept {
    if (sockets_.count(sock) == 0) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    auto rv = MOCK_send(sock, buffer, length, send_flags);
    if (rv >= 0) {
        return rv;
    }
    if (MOCK_get_last_error() != MK_EWOULDBLOCK) {
        return -1;
    }
    if ((sockets_.at(sock) & MK_F_NONBLOCK) != 0) {
        return -1;
    }
    fd_set writeset;
    FD_ZERO(&writeset);
    FD_SET(sock, &writeset);
    auto ctrl = do_select(sock + 1, nullptr, &writeset, nullptr, nullptr);
    if (ctrl == -1) {
        return -1;
    }
    if (ctrl > 0) {
        return MOCK_send(sock, buffer, length, send_flags);
    }
    MOCK_set_last_error(MK_ETIMEDOUT);
    return -1;
}

int Context::do_closesocket(mk_socket_t sock) noexcept {
    if (sockets_.count(sock) == 0) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    sockets_.erase(sock);
    return MOCK_closesocket(sock);
}

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_IMPL_SOCKET_DO_HPP
