/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_PORTABLE_TYPES_H
#define MEASUREMENT_KIT_PORTABLE_TYPES_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/types.h>
#endif

#ifdef _WIN32
typedef SSIZE_T mk_ssize_t;
#else
typedef ssize_t mk_ssize_t;
#endif

#ifdef _WIN32
typedef SIZE_T mk_size_t;
#else
typedef size_t mk_size_t;
#endif

#endif /* MEASUREMENT_KIT_PORTABLE_TYPES_H */
