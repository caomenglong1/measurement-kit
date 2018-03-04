/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_PORTABLE_NOEXCEPT_H
#define MEASUREMENT_KIT_PORTABLE_NOEXCEPT_H

#if (defined __cplusplus)
#if __cplusplus >= 201103L
#define MK_NOEXCEPT noexcept
#else
#define MK_NOEXCEPT throw()
#endif
#else
#define MK_NOEXCEPT /* Nothing. */
#endif

#endif /* MEASUREMENT_KIT_PORTABLE_NOEXCEPT_H */
