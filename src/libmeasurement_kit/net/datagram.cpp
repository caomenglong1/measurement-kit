// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

// This file defines the implementation of a datagram socket in terms of
// a specific virtual implementation.

#include "private/net/datagram.hpp"

namespace mk {
namespace net {
namespace datagram {

Error Socket::close() { return pimpl->close(); }

Error Socket::connect(sockaddr_storage *storage) {
    return pimpl->connect(storage);
}

void Socket::on_close(std::function<void()> &&cb) {
    pimpl->on_close(std::move(cb));
}

void Socket::on_datagram(
        std::function<void(const void *, size_t, const sockaddr_storage *)>
                &&cb) {
    pimpl->on_datagram(std::move(cb));
}

void Socket::on_error(std::function<void(Error &&)> &&cb) {
    pimpl->on_error(std::move(cb))l
}

void Socket::on_timeout(std::function<void()> &&cb) {
    pimpl->on_timeout(std::move(cb));
}

void Socket::pause() { pimpl->pause(); }

void Socket::resume() { pimpl->resume(); }

Error Socket::try_sendto(std::string &&binary_data, sockaddr_storage *dest) {
    return pimpl->try_sendto(std::move(binary_data), dest);
}

void Socket::set_timeout(uint32_t millisec) {
    pimpl->set_timeout(millisec);
}

Socket::~Impl() override {}

} // namespace datagram
} // namespace net
} // namespace mk
#endif
