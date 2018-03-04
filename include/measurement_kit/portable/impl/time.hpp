// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_TIME_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_TIME_HPP

#include <measurement_kit/portable/context.hpp>
#include <measurement_kit/portable/impl/flags.h>

namespace mk {
namespace portable {

#ifdef _WIN32
int Context::MOCK_timespec_get(timespec *ts, int base) noexcept {
    return ::timespec_get(ts, base);
}
#else
int Context::MOCK_gettimeofday(timeval *tv, struct timezone *tz) noexcept {
    return ::gettimeofday(tv, tz);
}
#endif

int Context::do_gettimeofday(timeval *tv, struct timezone *tz) noexcept {
#ifdef _WIN32
    if (tz != nullptr) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    timespec tspec{};
    if (MOCK_timespec_get(&ts, TIME_UTC) != TIME_UTC) {
        MOCK_set_last_error(MK_EINVAL); /* XXX */
        return -1;
    }
    tv->tv_sec = tspec.tv_sec;
    tv->tv_usec = tspec.tv_nsec * 1'000;
    return 0;
#else
    return MOCK_gettimeofday(tv, tz);
#endif
}

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_IMPL_TIME_HPP
