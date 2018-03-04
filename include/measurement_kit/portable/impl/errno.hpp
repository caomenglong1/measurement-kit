// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_ERRNO_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_ERRNO_HPP

#include <measurement_kit/portable/context.hpp>

namespace mk {
namespace portable {

int Context::MOCK_get_last_error() noexcept {
#ifdef _WIN32
    return ::WSAGetLastError();
#else
    auto err = errno;
#if EAGAIN != EWOULDBLOCK // theoretically possible but unlikely
    if (err == EAGAIN) {
        err = EWOULDBLOCK; // prefer EWOUBLOCK since is used by Windows
    }
#endif
    if (err == EINPROGRESS) {
        err = EWOULDBLOCK; // behave like Winsock2
    }
    return err;
#endif
}

void Context::MOCK_set_last_error(int error_code) noexcept {
#ifdef _WIN32
    ::WSASetLastError(error_code);
#else
    errno = error_code;
#endif
}

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_CONTEXT_HPP
