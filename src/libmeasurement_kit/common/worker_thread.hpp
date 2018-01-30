// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef SRC_LIBMEASUREMENT_KIT_COMMON_WORKER_THREAD_HPP
#define SRC_LIBMEASUREMENT_KIT_COMMON_WORKER_THREAD_HPP

#include <stdint.h>   // for size_t

#include <functional> // for std::function
#include <memory>     // for std::unique_ptr

namespace mk {

/// WorkerThread is a worker thread. This class does not leak memory and joins
/// the background thread in case a job throws an exception, but is not designed
/// to be used again after such event. Measurement Kit's core should not catch
/// any exceptions, which should be treated as fatal errors.
class WorkerThread {
  public:
    /// WorkerThread() creates the worker thread.
    WorkerThread();

    /// submit() submits @p job to the worker thread.
    void submit(std::function<void()> &&job);

    /// interrupt() signals the worker thread that it should stop immediately.
    void interrupt();

    /// get_queue_size() returns the jobs' queue size.
    size_t get_queue_size();

    /// ~WorkerThread() calls interrupt() to stop the worker thread ASAP and
    /// then joins such thread to avoid leaking its memory.
    ~WorkerThread();

  private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/// tasks_worker_thread() returns the worker used by Measurement Kit to run
/// network measurements and orchestration (aka "tasks").
WorkerThread &tasks_worker_thread();

} // namespace mk
#endif
