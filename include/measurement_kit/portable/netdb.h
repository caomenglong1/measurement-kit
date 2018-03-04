/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_PORTABLE_NETDB_H
#define MEASUREMENT_KIT_PORTABLE_NETDB_H

#ifdef _WIN32
#include <Ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include <measurement_kit/portable/extern.h>
#include <measurement_kit/portable/noexcept.h>

#define MK_EAI_NONE 0
#define MK_EAI_AGAIN 1
#define MK_EAI_BADFLAGS 2
#define MK_EAI_BADHINTS 3
#define MK_EAI_FAIL 4
#define MK_EAI_FAMILY 5
#define MK_EAI_MEMORY 6
#define MK_EAI_NONAME 7
#define MK_EAI_OVERFLOW 8
#define MK_EAI_PROTOCOL 9
#define MK_EAI_SERVICE 10
#define MK_EAI_SOCKTYPE 11
#define MK_EAI_SYSTEM 12

#ifdef __cplusplus
extern "C" {
#endif

MK_EXTERN int mk_getaddrinfo(const char *hostname, const char *servname,
        const struct addrinfo *hints, struct addrinfo **res) MK_NOEXCEPT;

MK_EXTERN void mk_freeaddrinfo(struct addrinfo *ai) MK_NOEXCEPT;

#ifdef __cplusplus
} // extern "C"
#endif
#endif /* MEASUREMENT_KIT_PORTABLE_NETDB_H */
