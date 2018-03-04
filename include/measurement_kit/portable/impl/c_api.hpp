// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_C_API_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_C_API_HPP

#include <measurement_kit/portable/context.hpp>

#define CTX mk::portable::Context::get_instance(std::this_thread::get_id())

int mk_get_last_error() noexcept { return CTX->MOCK_get_last_error(); }

void mk_set_last_error(int error_code) noexcept {
    CTX->MOCK_set_last_error(error_code);
}

int mk_getaddrinfo(const char *hostname, const char *servname,
        const addrinfo *hints, addrinfo **res) noexcept {
    return CTX->mk_getaddrinfo(hostname, servname, hints, res);
}

void mk_freeaddrinfo(addrinfo *ai) noexcept { return CTX->mk_freeaddrinfo(ai); }

int mk_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds,
        timeval *timeout) noexcept {
    return CTX->mk_select(nfds, readfds, writefds, errorfds, timeout);
}

mk_socket_t mk_socket(int domain, int type, int protocol) noexcept {
    return CTX->mk_socket(domain, type, protocol);
}

int mk_connect(mk_socket_t sock, const sockaddr *endpoint,
        mk_socklen_t endpoint_length) noexcept {
    return CTX->mk_connect(sock, endpoint, endpoint_length);
}

int mk_ioctlsocket(
        mk_socket_t sock, long command, unsigned long *argument) noexcept {
    return CTX->mk_ioctlsocket(sock, command, argument);
}

mk_ssize_t mk_recv(mk_socket_t sock, void *buffer, mk_size_t length,
        int recv_flags) noexcept {
    return CTX->mk_recv(sock, buffer, length, recv_flags);
}

mk_ssize_t mk_send(mk_socket_t sock, const void *buffer, mk_size_t length,
        int send_flags) noexcept {
    return CTX->mk_send(sock, buffer, length, send_flags);
}

int mk_closesocket(mk_socket_t sock) noexcept {
    return CTX->mk_closesocket(sock);
}

int mk_gettimeofday(timeval *tv, timezone *tz) noexcept {
    return CTX->mk_gettimeofday(tv, tz);
}

int mk_gettimeofday_as_double(double *now) noexcept {
    if (now == nullptr) {
        mk_set_last_error(MK_EINVAL);
        return -1;
    }
    timeval tv{};
    if (mk_gettimeofday(&tv, nullptr) != 0) {
        return -1;
    }
    *now = (double)tv.tv_sec;
    *now += (double)tv.tv_usec / 1'000'000;
    return 0;
}

#undef CTX // Tidy

#endif // MEASUREMENT_KIT_PORTABLE_IMPL_C_API_HPP
