/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_PORTABLE_SELECT_H
#define MEASUREMENT_KIT_PORTABLE_SELECT_H

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/select.h>
#endif

#include <measurement_kit/portable/extern.h>
#include <measurement_kit/portable/noexcept.h>

#ifdef __cplusplus
extern "C" {
#endif

MK_EXTERN int mk_select(int nfds, fd_set *readfds, fd_set *writefds,
        fd_set *errorfds, struct timeval *timeout) MK_NOEXCEPT;

#ifdef __cplusplus
} // extern "C"
#endif
#endif /* MEASUREMENT_KIT_PORTABLE_SELECT_H */
