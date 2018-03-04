/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_PORTABLE_TIME_H
#define MEASUREMENT_KIT_PORTABLE_TIME_H

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/time.h>
#endif

#include <measurement_kit/portable/extern.h>
#include <measurement_kit/portable/noexcept.h>

#ifdef _WIN32
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

MK_EXTERN int mk_gettimeofday(
        struct timeval *tv, struct timezone *tz) MK_NOEXCEPT;

MK_EXTERN int mk_gettimeofday_as_double(double *now) MK_NOEXCEPT;

#ifdef __cplusplus
} // extern "C"
#endif
#endif /* MEASUREMENT_KIT_PORTABLE_TIME_H */
