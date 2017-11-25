// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef PRIVATE_NET_DATAGRAM_HPP
#define PRIVATE_NET_DATAGRAM_HPP

// This file defines the abstract implementation of a datagram socket. Each
// reactor (e.g. the libevent reactor) will have ownership of the socket and
// will define the specific, concrete implementation.

#include <measurement_kit/net/datagram.hpp>

namespace mk {
namespace net {
namespace datagram {

class Socket::Impl {
  public:
    virtual Error close() = 0;

    virtual Error connect(sockaddr_storage *storage) = 0;

    virtual void on_close(std::function<void()> &&cb) = 0;

    virtual void on_datagram(std::function<void(const void *, size_t,
            const sockaddr_storage *)> &&cb) = 0;

    virtual void on_error(std::function<void(Error)> &&cb) = 0;

    virtual void on_timeout(std::function<void()> &&cb) = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual Error try_sendto(std::string &&, sockaddr_storage *) = 0;

    virtual void set_timeout(uint32_t millisec) = 0;

    virtual ~Impl();
};

} // namespace datagram
} // namespace net
} // namespace mk
#endif
