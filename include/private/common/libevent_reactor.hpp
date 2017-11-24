// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef PRIVATE_COMMON_LIBEVENT_REACTOR_HPP
#define PRIVATE_COMMON_LIBEVENT_REACTOR_HPP

// # Libevent Reactor

#include <event2/event.h>                           // for event_base_*
#include <event2/thread.h>                          // for evthread_use_*
#include <event2/util.h>                            // for evutil_socket_t
#include <signal.h>                                 // for sigaction
#include <cassert>                                  // for assert
#include <measurement_kit/common/callback.hpp>      // for mk::Callback
#include <measurement_kit/common/data_usage.hpp>    // for mk::DataUsage
#include <measurement_kit/common/error.hpp>         // for mk::Error
#include <measurement_kit/common/logger.hpp>        // for mk::warn
#include <measurement_kit/common/non_copyable.hpp>  // for mk::NonCopyable
#include <measurement_kit/common/non_movable.hpp>   // for mk::NonMovable
#include <measurement_kit/common/reactor.hpp>       // for mk::Reactor
#include <measurement_kit/common/socket.hpp>        // for mk::socket_t
#include <measurement_kit/common/unique_ptr.hpp>    // for mk::UniquePtr
#include <measurement_kit/net/error.hpp>            // for mk::net::*Error
#include <mutex>                                    // for std::recursive_mutex
#include <set>                                      // for std::set
#include <stdexcept>                                // for std::runtime_error
#include <utility>                                  // for std::move
#include "private/common/locked.hpp"                // for mk::locked_global
#include "private/common/mock.hpp"                  // for MK_MOCK
#include "private/common/utils.hpp"                 // for mk::timeval_init
#include "private/common/worker.hpp"                // for mk::Worker
#include "private/net/datagram.hpp"                 // for mk::Worker

extern "C" {
static inline void mk_pollfd_cb(evutil_socket_t, short, void *);
static inline void mk_datagram_read(evutil_socket_t, short, void *);
}

namespace mk {

// Deleter for an event_base pointer.
class EventBaseDeleter {
   public:
    void operator()(event_base *evbase) {
        if (evbase != nullptr) {
            event_base_free(evbase);
        }
    }
};

// Deleter for an event pointer.
class EventDeleter {
   public:
    void operator()(event *evp) {
        if (evp != nullptr) {
            event_free(evp);
        }
    }
};

// Deleter for a file descriptor pointer.
class FdDeleter {
   public:
    void operator()(evutil_socket_t *fd) {
        if (fd != nullptr) {
            if (*fd != -1) (void)evutil_closesocket(*fd);
            delete fd;
        }
    }
};

// LibeventReactor is an mk::Reactor implementation using libevent.
//
// The current implementation as of 2017-11-01 does not need to be explicitly
// non-copyable and non-movable. But, given that in the future we will need
// probably to pass `this` to some libevent functions, and that anyway it is
// always used as mk::SharedPtr<mk::Reactor>, it seems more robust to keep it
// explicitly non-copyable and non-movable.
//
// TODO: add more mocks
template <MK_MOCK(event_base_new), MK_MOCK(event_base_once),
        MK_MOCK(event_base_dispatch), MK_MOCK(event_base_loopbreak)>
class LibeventReactor : public Reactor, public NonCopyable, public NonMovable {
   public:
    // ## Initialization

    template <MK_MOCK(evthread_use_pthreads), MK_MOCK(sigaction)>
    static inline void libevent_init_once() {
        return locked_global([]() {
            static bool initialized = false;
            if (initialized) {
                return;
            }
            mk::debug("initializing libevent once");
            if (evthread_use_pthreads() != 0) {
                throw std::runtime_error("evthread_use_pthreads");
            }
            struct sigaction sa = {};
            sa.sa_handler = SIG_IGN;
            if (sigaction(SIGPIPE, &sa, nullptr) != 0) {
                throw std::runtime_error("sigaction");
            }
            initialized = true;
        });
    }

    LibeventReactor() {
        libevent_init_once();
        evbase.reset(event_base_new());
        if (evbase.get() == nullptr) {
            throw std::runtime_error("event_base_new");
        }
    }

    ~LibeventReactor() override {}

    // ## Event loop management

    event_base *get_event_base() override { return evbase.get(); }

    void run() override {
        do {
            auto ev_status = event_base_dispatch(evbase.get());
            if (ev_status < 0) {
                throw std::runtime_error("event_base_dispatch");
            }
            /*
                Explanation: event_base_loop() returns one when there are no
                pending events. In such case, before leaving the event loop, we
                make sure we have no pending background threads. They are, as
                of now, mostly used to perform DNS queries with getaddrinfo(),
                which is blocking. If there are threads running, treat them
                like pending events, even though they are not managed by
                libevent, and continue running the loop. To avoid spawning
                and to be sure we're ready to deal /pronto/ with any upcoming
                libevent event, schedule a call for the near future so to
                keep the libevent loop active, and ready to react.

                The exact possible values for `ev_status` are -1, 0, and +1, but
                I have coded more broad checks for robustness.
            */
            if (ev_status > 0 && worker.concurrency() <= 0) {
                break;
            }
            call_later(0.250, []() {});
        } while (true);
    }

    void stop() override {
        if (event_base_loopbreak(evbase.get()) != 0) {
            throw std::runtime_error("event_base_loopbreak");
        }
    }

    // ## Call later

    void call_in_thread(SharedPtr<Logger> logger, Callback<> &&cb) override {
        worker.call_in_thread(logger, std::move(cb));
    }

    void call_soon(Callback<> &&cb) override { call_later(0.0, std::move(cb)); }

    void call_later(double delay, Callback<> &&cb) override {
        // Note: according to libevent documentation, it is not necessary to
        // pass `EV_TIMEOUT` to get a timeout. But I find passing it more clear.
        pollfd(-1, EV_TIMEOUT,
                delay, [cb = std::move(cb)](Error, short) { cb(); });
    }

    // ## Poll sockets

    void pollin_once(socket_t fd, double timeo, Callback<Error> &&cb) override {
        pollfd(fd, EV_READ, timeo, [cb = std::move(cb)](Error err, short) {
            cb(std::move(err));
        });
    }

    void pollout_once(
            socket_t fd, double timeo, Callback<Error> &&cb) override {
        pollfd(fd, EV_WRITE, timeo, [cb = std::move(cb)](Error err, short) {
            cb(std::move(err));
        });
    }

    // ## Datagram sockets

    class LibeventDatagramSocket
        : public net::datagram::Socket::Impl,
          public std::enable_shared_from_this<LibeventDatagramSocket> {
       public:
        Error close() override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            evp.reset();
            net::clear_last_error();
            (void)evutil_closesocket(*fd);
            auto err = net::get_last_error();
            *fd = -1;
            owner->close_datagram_socket(shared_from_this());
            auto cbs = std::move(close_cbs);
            for (auto &cb : cbs) cb();
            return err;
        }

        Error connect(sockaddr_storage *storage) override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            socklen_t sslen{};
            switch (storage->ss_family) {
                case AF_INET:
                    sslen = sizeof(sockaddr_in);
                    break;
                case AF_INET6:
                    sslen = sizeof(sockaddr_in6);
                    break;
                default:
                    throw std::runtime_error("invalid_family");
                    // NOTREACHED
            }
            net::clear_last_error();
            (void)connect(*fd, (sockaddr *)storage, sslen);
            return net::get_last_error();
        }

        void on_close(std::function<void()> &&cb) override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            close_cbs.push_back(std::move(cb));
        }

        void read_cb(short evflags) {
            std::unique_lock<std::recursive_mutex> _{mutex};
            if ((evflags & EV_TIMEOUT) != 0) {
                pause();  // TODO(sbs): is this call needed?
                auto cbs = std::move(timeout_cbs);
                for (auto &cb : cbs) cb();
                if (!timeout_cbs.empty()) cbs.splice(cbs.end(), timeout_cbs);
                timeout_cbs = std::move(cbs);
                return;
            }
            assert((evflags & EV_WRITE) == 0);
            assert((evflags & EV_READ) != 0);
            constexpr auto maxreads = 7;  // Eventually stop reading
            for (auto i = 0; i < maxreads; ++i) {
                net::clear_last_error();
                sockaddr_storage storage{};
                socklen_t sslen{};
                auto rv = recvfrom(*fd, buffer, sizeof(buffer), 0,
                        (sockaddr *)&storage, &sslen);
                if (rv < 0) {
                    auto err = net::get_last_error();
                    if (err == net::OperationWouldBlockError()) {
                        break;  // Read again later
                    }
                    pause();
                    auto cbs = std::move(error_cbs);
                    for (auto &cb : cbs) cb(err);
                    if (!error_cbs.empty()) cbs.splice(cbs.end(), error_cbs);
                    error_cbs = std::move(cbs);
                    return;
                }
                auto cbs = std::move(datagram_cbs);
                for (auto &cb : cbs) cb(buffer, (size_t)rv, &storage);
                if (!datagram_cbs.empty()) cbs.splice(cbs.end(), datagram_cbs);
                datagram_cbs = std::move(cbs);
            }
        }

        void on_datagram(std::function<void(const void *, size_t,
                        const sockaddr_storage *)> &&cb) override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            datagram_cbs.push_back(std::move(cb));
        }

        void on_error(std::function<void(Error &&)> &&cb) override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            error_cbs.push_back(std::move(cb));
        }

        void on_timeout(std::function<void()> &&cb) override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            timeout_cbs.push_back(std::move(cb));
        }

        void pause() override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            if (event_del(evp.get()) != 0) {
                throw std::runtime_error("event_del");
            }
        }

        void resume() override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            if (event_add(evp.get(), &timeo) != 0) {
                throw std::runtime_error("event_add");
            }
        }

        Error try_sendto(
                std::string &&binary_data, sockaddr_storage *dest) override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            net::clear_last_error();
            socklen_t sslen{};
            switch (dest->ss_family) {
                case AF_INET:
                    sslen = sizeof(sockaddr_in);
                    break;
                case AF_INET6:
                    sslen = sizeof(sockaddr_in6);
                    break;
                default:
                    throw std::runtime_error("invalid_family");
                    // NOTREACHED
            }
            auto count = sendto(*fd, binary_data.data(), binary_data.size(), 0,
                    (sockaddr *)dest, sslen);
            auto err = net::get_last_error();
            if (err) return err;
            // TODO(sbs): figure out whether we want a specific error
            if (count != binary_data.size()) return net::MessageSizeError();
            return NoError();
        }

        void set_timeout(uint32_t millisec) override {
            std::unique_lock<std::recursive_mutex> _{mutex};
            timeo.tv_sec = millisec / 1000;
            timeo.tv_usec = (millisec % 1000) * 1000;
        }

        ~LibeventDatagramSocket() override {}

        LibeventDatagramSocket(LibeventReactor *reactor, int family) {
            {
                auto sd = socket(family, SOCK_DGRAM, 0);
                if (sd == -1) throw std::runtime_error("socket");
                fd.reset(new evutil_socket_t{sd});
            }
            evp.reset(event_new(evbase, *fd, EV_READ, mk_datagram_read, this));
            if (!evp) throw std::runtime_error("event_new");
            if (evutil_make_socket_nonblocking(*fd) != 0) {
                throw std::runtime_error("evutil_make_socket_nonblocking");
            }
            owner = reactor;
        }

        char buffer[4096];
        std::list<std::function<void()>> close_cbs;
        std::list<std::function<void(
                const void *, size_t, const sockaddr_storage *)>>
                datagram_cbs;
        std::list<std::function<void(Error &&)>> error_cbs;
        UniquePtr<event, EventDeleter> evp;
        UniquePtr<evutil_socket_t, FdDeleter> fd;
        std::recursive_mutex mutex;
        LibeventReactor *owner = nullptr;
        timeval timeo{30, 0};
        std::list<std::function<void()>> timeout_cbs;
    };

    net::datagram::Socket make_datagram_socket(int family) override {
        SharedPtr<net::datagram::Socket::Impl> pimpl;
        pimpl.reset(new LibeventDatagramSocket{this, family});
        net::datagram::Socket dsock{pimpl};
        std::unique_lock<std::recursive_mutex> _{mutex};
        active_datagram_sockets.insert(pimpl.underlying());
        return dsock;
    }

    void close_datagram_socket(std::shared_ptr<LibeventDatagramSocket> so) {
        std::unique_lock<std::recursive_mutex> _{mutex};
        active_datagram_sockets.erase(so);
    }

    // ## Internals

    void pollfd(socket_t sockfd, short evflags, double timeout,
            Callback<Error, short> &&callback) {
        timeval tv{};
        auto cbp = new Callback<Error, short>(callback);
        if (event_base_once(evbase.get(), sockfd, evflags, mk_pollfd_cb, cbp,
                    timeval_init(&tv, timeout)) != 0) {
            delete cbp;
            throw std::runtime_error("event_base_once");
        }
    }

    static void pollfd_cb(short evflags, void *opaque) {
        auto cbp = static_cast<mk::Callback<mk::Error, short> *>(opaque);
        mk::Error err = mk::NoError();
        assert((evflags & (~(EV_TIMEOUT | EV_READ | EV_WRITE))) == 0);
        if ((evflags & EV_TIMEOUT) != 0) {
            err = mk::TimeoutError();
        }
        // In case of exception here, the stack is going to unwind, tearing down
        // the libevent loop and leaking forever `cbp` and the event once that
        // was used to invoke this callback.
        (*cbp)(std::move(err), evflags);
        delete cbp;
    }

    // ## Data usage

    void with_current_data_usage(Callback<DataUsage &> &&cb) override {
        std::unique_lock<std::recursive_mutex> _{mutex};
        cb(data_usage);
    }

   private:
    // ## Private attributes

    std::set<std::shared_ptr<LibeventDatagramSocket>> active_datagram_sockets;
    UniquePtr<event_base, EventBaseDeleter> evbase;
    DataUsage data_usage;
    std::recursive_mutex mutex;
    Worker worker;
};

}  // namespace mk

// ## C linkage callbacks

static inline void mk_pollfd_cb(evutil_socket_t, short evflags, void *opaque) {
    mk::LibeventReactor<>::pollfd_cb(evflags, opaque);
}

static inline void mk_datagram_read(
        evutil_socket_t, short evflags, void *opaque) {
    using namespace mk;
    auto ptr = static_cast<LibeventReactor<>::LibeventDatagramSocket *>(opaque);
    ptr->read_cb(evflags);
}
#endif
