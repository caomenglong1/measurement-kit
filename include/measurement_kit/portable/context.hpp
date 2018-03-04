// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_PORTABLE_CONTEXT_HPP
#define MEASUREMENT_KIT_PORTABLE_CONTEXT_HPP

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

#include <measurement_kit/portable/errno.h>
#include <measurement_kit/portable/netdb.h>
#include <measurement_kit/portable/select.h>
#include <measurement_kit/portable/socket.h>
#include <measurement_kit/portable/time.h>

namespace mk {
namespace portable {

class Context {
  public:
    // errno.h

    virtual int MOCK_get_last_error() noexcept;

    virtual void MOCK_set_last_error(int error_code) noexcept;

    // netdb.h

    virtual int MOCK_getaddrinfo(const char *hostname, const char *servname,
            const struct addrinfo *hints, struct addrinfo **res) noexcept;

    virtual int mk_getaddrinfo(const char *hostname, const char *servname,
            const struct addrinfo *hints, struct addrinfo **res) noexcept;

    virtual void MOCK_freeaddrinfo(struct addrinfo *ai) noexcept;

    virtual void mk_freeaddrinfo(struct addrinfo *ai) noexcept;

    // sys/select.h

    virtual int MOCK_select(int nfds, fd_set *readfds, fd_set *writefds,
            fd_set *errorfds, struct timeval *timeout) noexcept;

    virtual int mk_select(int nfds, fd_set *readfds, fd_set *writefds,
            fd_set *errorfds, struct timeval *timeout) noexcept;

    // sys/socket.h

    virtual mk_socket_t MOCK_socket(
            int domain, int type, int protocol) noexcept;

    virtual mk_socket_t mk_socket(int domain, int type, int protocol) noexcept;

    virtual int MOCK_connect(mk_socket_t sock, const struct sockaddr *endpoint,
            mk_socklen_t endpoint_length) noexcept;

    virtual int mk_connect(mk_socket_t sock, const struct sockaddr *endpoint,
            mk_socklen_t endpoint_length) noexcept;

#ifndef _WIN32
    virtual int MOCK_fcntl_void(mk_socket_t sock, int command) noexcept;

    virtual int MOCK_fcntl_int(
            mk_socket_t sock, int command, int value) noexcept;
#endif

    virtual int MOCK_ioctlsocket(
            mk_socket_t sock, long command, unsigned long *argument) noexcept;

    virtual int mk_ioctlsocket(
            mk_socket_t sock, long command, unsigned long *argument) noexcept;

    virtual int MOCK_getsockopt(mk_socket_t sock, int level, int option_name,
            mk_sockopt_t *option_value, mk_socklen_t *option_len) noexcept;

    virtual int mk_getsockopt(mk_socket_t sock, int level, int option_name,
            mk_sockopt_t *option_value, mk_socklen_t *option_len) noexcept;

    virtual mk_ssize_t MOCK_recv(mk_socket_t sock, void *buffer,
            mk_size_t length, int recv_flags) noexcept;

    virtual mk_ssize_t mk_recv(mk_socket_t sock, void *buffer, mk_size_t length,
            int recv_flags) noexcept;

    virtual mk_ssize_t MOCK_send(mk_socket_t sock, const void *buffer,
            mk_size_t length, int send_flags) noexcept;

    virtual mk_ssize_t mk_send(mk_socket_t sock, const void *buffer,
            mk_size_t length, int send_flags) noexcept;

    virtual int MOCK_closesocket(mk_socket_t sock) noexcept;

    virtual int mk_closesocket(mk_socket_t sock) noexcept;

    // sys/time.h
    //
    // Note: must use `struct timezone` because in Darwin there is also
    // an external `long` variable named `timezone`.

#ifdef _WIN32
    virtual int MOCK_timespec_get(timespec *ts, int base) noexcept;
#else
    virtual int MOCK_gettimeofday(timeval *tv, struct timezone *tz) noexcept;
#endif

    virtual int mk_gettimeofday(timeval *tv, struct timezone *tz) noexcept;

    // context proper

    virtual ~Context() noexcept;

    void interrupt() noexcept;

    static Context *get_instance(std::thread::id id) noexcept;

    static void set_instance(std::thread::id id, Context *ctx) noexcept;

    static void clear_instance(std::thread::id id) noexcept;

  private:
    static std::map<std::thread::id, Context *> global_map_;
    static std::mutex global_mutex_;
    std::atomic_uint64_t flags_{0};
    std::map<mk_socket_t, uint64_t> sockets_;
};

} // namespace portable
} // namespace mk
#endif // MEASUREMENT_KIT_PORTABLE_CONTEXT_HPP
