/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_PORTABLE_ERRNO_H
#define MEASUREMENT_KIT_PORTABLE_ERRNO_H

#ifdef _WIN32
#include <Winerror.h>
#else
#include <errno.h>
#endif

#include <measurement_kit/portable/extern.h>
#include <measurement_kit/portable/noexcept.h>

#ifdef __cplusplus
extern "C" {
#endif

MK_EXTERN int mk_get_last_error(void) MK_NOEXCEPT;

MK_EXTERN void mk_set_last_error(int error_code) MK_NOEXCEPT;

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef _WIN32
#define MK_EPREFIX(_name) WSAE##_name
#else
#define MK_EPREFIX(_name) E##_name
#endif

#define MK_ENONE 0
#define MK_EALREADY MK_EPREFIX(ALREADY)
#define MK_ECONNABORTED MK_EPREFIX(CONNABORTED)
#define MK_ECONNREFUSED MK_EPREFIX(CONNREFUSED)
#define MK_ECONNRESET MK_EPREFIX(CONNRESET)
#define MK_EHOSTUNREACH MK_EPREFIX(HOSTUNREACH)
#define MK_EFAULT MK_EPREFIX(FAULT)
#define MK_EINTR MK_EPREFIX(INTR)
#define MK_EINVAL MK_EPREFIX(INVAL)
#define MK_ENETDOWN MK_EPREFIX(NETDOWN)
#define MK_ENETRESET MK_EPREFIX(NETRESET)
#define MK_ENETUNREACH MK_EPREFIX(NETUNREACH)
#define MK_ENOBUFS MK_EPREFIX(NOBUFS)
#define MK_EPIPE MK_EPREFIX(ESHUTDOWN) /* Slightly different semantic. */
#define MK_ESHUTDOWN MK_EPREFIX(SHUTDOWN)
#define MK_ETIMEDOUT MK_EPREFIX(TIMEDOUT)
#define MK_EWOULDBLOCK MK_EPREFIX(WOULDBLOCK)

#endif /* MEASUREMENT_KIT_PORTABLE_ERRNO_H */
