// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_NET_DATAGRAM_HPP
#define MEASUREMENT_KIT_NET_DATAGRAM_HPP

#include <measurement_kit/common/aaa_base.hpp>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/shared_ptr.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace mk {
namespace net {
namespace datagram {

/// `datagram::Socket` is an async datagram socket.
class Socket {
  public:
    /// `Impl` is the opaque implementation of this class.
    class Impl;

    /// `Socket` constructs a new datagram socket. In general you cannot call
    /// this directly, because you don't know the details of `Impl`. To make a
    /// new datagram socket, you should use the `Reactor::make_datagram_socket`
    /// method.
    explicit Socket(SharedPtr<Impl> pimpl);

    /// `close` closes this datagram socket. The callback(s) registered with
    /// the `on_close` method will be called after the socket has been closed.
    /// You must close sockets explicitly, otherwise you would leak the
    /// resources associated with them.
    Error close();

    /// `connect` connects to a remote endpoint.
    Error connect(sockaddr_storage *storage);

    /// `on_close` registers the callback(s) called when the socket is closed.
    void on_close(std::function<void()> &&cb);

    /// `on_datagram` registers the callback(s) called when a datagram is
    /// received by the socket. The first two arguments are the beginning
    /// and the size of the datagram payload. The third argument is the
    /// address of the endpint that sent the datagram.
    void on_datagram(std::function<void(const void *, size_t,
            const sockaddr_storage *)> &&cb);

    /// `on_error` registers the callback(s) called when there is an
    /// error on any async I/O operation. After this event, call resume()
    /// if you want to start reading again.
    void on_error(std::function<void(Error &&)> &&cb);

    /// `on_timeout` registers the callback(s) called when there is a timeout
    /// sending and/or receiving data. After this event, call resume() if you
    /// want to start reading again.
    void on_timeout(std::function<void()> &&cb);

    /// `pause` stops reading. By default the socket is readable and you are
    /// notified of any incoming datagram.
    void pause();

    /// `resume` resumes reading. Call this after you have called `pause`
    /// to start reading again from the socket.
    void resume();

    /// `try_sendto` will send arbitrary binary data to the specified endpoint
    /// and fail, in addition to typical network errors, if the message has
    /// been truncated while sending.
    Error try_sendto(std::string &&binary_data, sockaddr_storage *dest);

    /// `set_timeout` will set the timeout used for I/O in milliseconds. The
    /// default timeout for I/O is 30 seconds. Setting a new timeout will not
    /// affect already pending I/O.
    void set_timeout(uint32_t millisec);

    /// `pimpl` is a shared pointer to the opaque implementation. It is public
    /// because that might be useful when running unit tests.
    SharedPtr<Impl> pimpl;
};

} // namespace datagram
} // namespace net
} // namespace mk
#endif
