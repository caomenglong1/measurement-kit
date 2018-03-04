// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_IMPL_NETDB_HPP
#define MEASUREMENT_KIT_PORTABLE_IMPL_NETDB_HPP

#include <assert.h>

#include <measurement_kit/portable/context.hpp>

namespace mk {
namespace portable {

int Context::MOCK_getaddrinfo(const char *hostname, const char *servname,
        const struct addrinfo *hints, struct addrinfo **res) noexcept {
    return ::getaddrinfo(hostname, servname, hints, res);
}

#define ENUM_EASY_TO_MAP_ERRORS(XX)                                            \
    XX(EAI_AGAIN)                                                              \
    XX(EAI_BADFLAGS)                                                           \
    XX(EAI_FAIL)                                                               \
    XX(EAI_FAMILY)                                                             \
    XX(EAI_MEMORY)                                                             \
    XX(EAI_NONAME)                                                             \
    XX(EAI_SERVICE)                                                            \
    XX(EAI_SOCKTYPE)

#define EASY_MAP(error_) case error_: return MK_##error_;

int Context::mk_getaddrinfo(const char *hostname, const char *servname,
        const struct addrinfo *hints, struct addrinfo **res) noexcept {
    int ctrl = MOCK_getaddrinfo(hostname, servname, hints, res);
    switch (ctrl) {
        ENUM_EASY_TO_MAP_ERRORS(EASY_MAP)
#ifdef EAI_OVERFLOW
    case EAI_OVERFLOW:
        return MK_EAI_OVERFLOW;
#endif
#ifdef EAI_BADHINTS
    case EAI_BADHINTS:
        return MK_EAI_BADHINTS;
#endif
#ifdef EAI_PROTOCOL
    case EAI_PROTOCOL:
        return MK_EAI_PROTOCOL;
#endif
#ifdef EAI_SYSTEM
    case EAI_SYSTEM:
        return MK_EAI_SYSTEM;
#endif
    case 0:
        return 0;
    default:
        break;
    }
    //
    // On Windows, calling WSAGetLastError() - i.e. mk_get_last_error() - is
    // going to return the exact error that occurred. On Unix, this case should
    // not happen, since EAI_SYSTEM should occur instead.
    //
#ifndef _WIN32
    assert(false); // Should really not happen on Unix
    errno = EIO;   // Must be set to something
#endif
    return MK_EAI_SYSTEM;
}

#undef EASY_MAP
#undef EASY_TO_MAP_ERRORS

// Currently the two functions below are a bit redundant. I think they will
// become less so when there will also be a getaddrinfo() using c-ares.

void Context::MOCK_freeaddrinfo(struct addrinfo *ai) noexcept {
    ::freeaddrinfo(ai);
}

void Context::mk_freeaddrinfo(struct addrinfo *ai) noexcept {
    MOCK_freeaddrinfo(ai);
}

} // namespace portable
} // namespace mk

#endif // MEASUREMENT_KIT_PORTABLE_IMPL_NETDB_HPP
