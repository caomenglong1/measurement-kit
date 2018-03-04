// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_SELECT_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_SELECT_HPP

#include <assert.h>

#include <measurement_kit/portable/context.hpp>
#include <measurement_kit/portable/impl/flags.h>

namespace mk {
namespace portable {

int Context::MOCK_select(int nfds, fd_set *readfds, fd_set *writefds,
        fd_set *errorfds, struct timeval *timeout) noexcept {
    int ctrl = ::select(nfds, readfds, writefds, errorfds, timeout);
    assert(ctrl == -1 || ctrl >= 0);
    return ctrl;
}

int Context::do_select(int nfds, fd_set *readfds, fd_set *writefds,
        fd_set *errorfds, struct timeval *timeout) noexcept {
    if (nfds < 0) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }
    if (timeout != nullptr && (timeout->tv_sec < 0 || timeout->tv_usec < 0)) {
        MOCK_set_last_error(MK_EINVAL);
        return -1;
    }

    // When the timeout is short behave exactly like select(). This should
    // allow the user to use select() to precisely use select() as a mechanism
    // to wait for a short period of time for some I/O  to occur.
    static constexpr auto short_sleep_interval = 250'000;
    if (timeout != nullptr && timeout->tv_sec == 0 &&
            timeout->tv_usec < short_sleep_interval) {
        auto ctrl = MOCK_select(nfds, readfds, writefds, errorfds, timeout);
        assert(ctrl >= -1);
        if (ctrl != -1 && (flags_ & MK_F_INTR) != 0) {
            MOCK_set_last_error(MK_ENETDOWN);
            ctrl = -1;
        }
        return ctrl;
    }

    // With longer timeouts, prioritize waking up from time to time to ensure
    // we are still allowed to wait for I/O to occur.
    double deadline = 0.0;
    if (timeout != nullptr) {
        if (mk_gettimeofday_as_double(&deadline) != 0) {
            return -1;
        }
        deadline += (double)timeout->tv_sec;
        deadline += (double)timeout->tv_usec / 1'000'000;
    }
    while ((flags_ & MK_F_INTR) == 0) {
        timeval tv{};
        tv.tv_usec = short_sleep_interval;
        int ctrl = MOCK_select(nfds, readfds, writefds, errorfds, &tv);
        switch (ctrl) {
        case -1:
            if (MOCK_get_last_error() != MK_EINTR) {
                return -1;
            }
            // FALLTHRU
        case 0:
            break; // go check the timeout
        default:
            assert(ctrl > 0);
            return ctrl;
        }
        if (timeout == nullptr) {
            continue;
        }
        assert(deadline > 0.0);
        double now = 0.0;
        if (mk_gettimeofday_as_double(&now) != 0) {
            return -1;
        }
        if (now > deadline) {
            MOCK_set_last_error(MK_ETIMEDOUT);
            return -1;
        }
    }
    MOCK_set_last_error(MK_ENETDOWN);
    return -1;
}

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_IMPL_SELECT_HPP
