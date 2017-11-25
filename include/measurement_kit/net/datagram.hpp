// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_NET_DATAGRAM_HPP
#define MEASUREMENT_KIT_NET_DATAGRAM_HPP

// This file defines the datagram socket class. Each `Reactor` will provide
// its own implementation of such class consistently with its I/O loop.

#include <measurement_kit/common/aaa_base.h>
#include <measurement_kit/common/enable_shared_from_this.hpp>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/shared_ptr.hpp>

#include <cstdint>
#include <functional>
#include <string>

namespace mk {
namespace net {
namespace datagram {

/// `datagram::Socket` is an async datagram socket. All the methods are thread
/// safe. All the methods registering callbacks can be called once or many
/// times; calling one of them more than once implies registering more than
/// one handler for the same event. Registering a handler for an event inside
/// of the same-event handler is okay, but the new handler will only be called
/// during the next occurrence of such event.
///
/// Exceptions derived from `std::exception` are raised in case of unrecoverable
/// errors (i.e. unexpected API failures).
///
/// After a datagram socket has been closed, calling any of its methods but
/// `close` -- which is idempotent -- would result in an exception derived
/// from `std::exception` to be raised.
///
/// A datagram socket instance has shared pointer semantics. This means you may
/// end up with reference cycles. To avoid that, make sure you call `close` when
/// it is not needed anymore. `close` has, in fact, a semantics equivalent to
/// calling `reset` on a `std::shared_ptr`. That would break any cycle.
///
/// The current API is designed to accomodate for the use cases of parisitic
/// traceroute and DNS, therefore, we did not provision for dealing with errors
/// like `EWOULDBLOCK` in the write path. This is a limitation that we will
/// fix when we'll add support for the uTP transport.
class Socket {
  public:
    /// `Impl` is the opaque implementation of this class. Each Reactor shall
    /// define an implementation suitable with its I/O loop model.
    class Impl;

    /// `Socket` constructs a new datagram socket. In general you cannot call
    /// this directly, because you don't know the details of `Impl`. To make a
    /// new datagram socket, you should use the `Reactor::make_datagram_socket`
    /// method. The Reactor knows about a specific datagram socket impl.
    explicit Socket(SharedPtr<Impl> &&pimpl);

    /// `close` closes this datagram socket. The callback(s) registered with
    /// the `on_close` method will be called after the socket has been closed.
    /// You must close sockets explicitly, otherwise you would leak the
    /// resources associated with them until the Reactor -- which owns the
    /// socket -- is destroyed. The code will never call `close` for
    /// you, and you should therefore close it in any code path, including
    /// those dealing with timeouts and I/O errors. It should be safe to
    /// call close() multiple times. The effect of close is to internally
    /// reset any state. Hence, calling close when you do not need a datagram
    /// socket anymore is your best defense against reference cycles.
    Error close();

    /// `connect` connects to a remote endpoint. Since this is a datagram
    /// socket, this API would succeed or fail immediately. Passing a nullptr
    /// storage would disconnect the socket from the remote endpoint, if it
    /// is connected; would do nothing otherwise.
    Error connect(sockaddr_storage *storage);

    /// `on_close` registers the callback(s) called when the socket is closed.
    void on_close(std::function<void()> &&cb);

    /// `on_datagram` registers the callback(s) called when a datagram is
    /// received by the socket. The first two arguments are the beginning
    /// and the size of the datagram payload. The third argument is the
    /// address of the endpoint that sent the datagram.
    void on_datagram(
            std::function<void(const void *, size_t, const sockaddr_storage *)>
                    &&cb);

    /// `on_error` registers the callback(s) called when there is an
    /// error while reading from the socket. After this event, call resume()
    /// if you want to start reading again.
    void on_error(std::function<void(Error)> &&cb);

    /// `on_timeout` registers the callback(s) called when there is a timeout
    /// receiving data from the socket. After this event, call resume() if you
    /// want to start reading again.
    void on_timeout(std::function<void()> &&cb);

    /// `pause` stops reading. By default the socket is readable and you are
    /// notified of any incoming datagram. This method is idempotent and it may
    /// be safely called multiple times.
    void pause();

    /// `resume` resumes reading. Call this after you have called `pause`
    /// to start reading again from the socket, or to resume reading after
    /// a timeout or I/O error event. This method is idempotent and may
    /// be safely called multiple times.
    void resume();

    /// `try_sendto` will send arbitrary binary data to the specified endpoint
    /// and return whether it succeeded or failed. We guarantee that you will
    /// know if the message was too big and therefore truncated, because we will
    /// return the `net::MessageSizeError` error. It is currently not specified
    /// whether, after you see this error, you can assume that your datagram
    /// was sent (truncated) or not. This is a limitation that we plan on fixing
    /// in future versions of MK, most likely after v0.9.0 has been released.
    /// Note that you can safely pass a null `dest` if the socket is connected.
    Error try_sendto(std::string &&binary_data, sockaddr_storage *dest);

    /// `set_timeout` will set the timeout used for I/O in milliseconds. The
    /// default timeout for I/O is 30 seconds. Setting a new timeout will not
    /// affect already pending I/O. Currently, the timeout would affect the
    /// read path only, since `try_sendto` -- which is our only method for
    /// sending packets -- will always return immediately.
    void set_timeout(uint32_t millisec);

  private:
    SharedPtr<Impl> pimpl;
};

} // namespace datagram
} // namespace net
} // namespace mk
#endif
