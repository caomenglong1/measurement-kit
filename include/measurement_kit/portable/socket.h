/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_PORTABLE_SOCKET_H
#define MEASUREMENT_KIT_PORTABLE_SOCKET_H

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

#include <measurement_kit/portable/extern.h>
#include <measurement_kit/portable/noexcept.h>
#include <measurement_kit/portable/types.h>

#ifdef _WIN32
typedef intptr_t mk_socket_t;
#else
typedef int mk_socket_t;
#endif

#ifdef _WIN32
typedef int mk_socklen_t;
#else
typedef socklen_t mk_socklen_t;
#endif

#ifdef _WIN32
typedef char mk_sockopt_t;
#else
typedef void mk_sockopt_t;
#endif

#ifdef _WIN32
#define MK_FIONBIO FIONBIO
#else
#define MK_FIONBIO 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

MK_EXTERN mk_socket_t mk_socket(int domain, int type, int protocol) MK_NOEXCEPT;

MK_EXTERN int mk_connect(mk_socket_t sock, const struct sockaddr *endpoint,
        mk_socklen_t endpoint_length) MK_NOEXCEPT;

MK_EXTERN int mk_ioctlsocket(mk_socket_t sock, long command,
        unsigned long *argument) MK_NOEXCEPT;

MK_EXTERN int mk_getsockopt(mk_socket_t sock, int level, int option_name,
        mk_sockopt_t *option_value, mk_socklen_t *option_len) MK_NOEXCEPT;

MK_EXTERN mk_ssize_t mk_recv(mk_socket_t sock, void *buffer, mk_size_t length,
        int recv_flags) MK_NOEXCEPT;

MK_EXTERN mk_ssize_t mk_send(mk_socket_t sock, const void *buffer,
        mk_size_t length, int send_flags) MK_NOEXCEPT;

MK_EXTERN int mk_closesocket(mk_socket_t sock) MK_NOEXCEPT;

#ifdef __cplusplus
} // extern "C"
#endif
#endif /* MEASUREMENT_KIT_PORTABLE_SOCKET_H */
