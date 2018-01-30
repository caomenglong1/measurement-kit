// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "src/libmeasurement_kit/common/worker_thread.hpp"

#include <atomic>             // for std::atomic_bool
#include <chrono>             // for std::chrono::milliseconds
#include <condition_variable> // for std::condition_variable
#include <deque>              // for std::deque
#include <functional>         // for std::function
#include <memory>             // for std::unique_ptr, std::make_unique
#include <mutex>              // for std::mutex
#include <thread>             // for std::thread

#include <iostream>

namespace mk {

class WorkerThread::Impl {
  public:
    std::condition_variable condition_variable;
    std::atomic_bool interrupted{false};
    std::mutex mutex;
    std::deque<std::function<void()>> jobs;
    std::thread thread;
};

WorkerThread::WorkerThread() {
    pimpl_ = std::make_unique<WorkerThread::Impl>();
    pimpl_->thread = std::thread([this]() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock{pimpl_->mutex};
                pimpl_->condition_variable.wait_for(
                        lock, std::chrono::milliseconds(250), [this]() {
                            return pimpl_->interrupted || !pimpl_->jobs.empty();
                        });
                if (pimpl_->interrupted) {
                    return;
                }
                std::swap(pimpl_->jobs.front(), task);
                pimpl_->jobs.pop_front();
            }
            task(); // This can be unlocked since it doesn't share state
        }
    });
}

void WorkerThread::submit(std::function<void()> &&job) {
    {
        std::unique_lock<std::mutex> _{pimpl_->mutex};
        pimpl_->jobs.push_back(std::move(job));
    }
    pimpl_->condition_variable.notify_one(); // no lock needed
}

void WorkerThread::interrupt() {
    pimpl_->interrupted = true;              // is atomic
    pimpl_->condition_variable.notify_one(); // no lock needed
}

size_t WorkerThread::get_queue_size() {
    std::unique_lock<std::mutex> _{pimpl_->mutex};
    return pimpl_->jobs.size();
}

WorkerThread::~WorkerThread() {
    interrupt();
    if (pimpl_->thread.joinable()) {
        pimpl_->thread.join();
    }
}

WorkerThread &tasks_worker_thread() {
    static WorkerThread singleton;
    return singleton;
}

} // namespace mk
