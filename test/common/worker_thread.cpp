// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "src/libmeasurement_kit/common/worker_thread.hpp"

#include <atomic>    // for std::atomic
#include <future>    // for std::future, std::promise
#include <stdexcept> // for std::runtime_error

#define CATCH_CONFIG_MAIN
#include "src/libmeasurement_kit/ext/catch.hpp"

TEST_CASE("WorkerThread works in the common case") {
    mk::WorkerThread worker;
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    std::atomic<size_t> counter{0};

    worker.submit([&counter]() { counter += 1; });
    worker.submit([&counter]() { counter += 1; });
    worker.submit([&counter]() { counter += 1; });
    worker.submit([&promise]() { promise.set_value(); });

    // We should reach the final job and have run all the jobs. If we don't
    // reach the final job, we'll hang here indefinitely.
    future.wait();
    REQUIRE(counter == 3);
}

TEST_CASE("interrupt() interrupts the worker thread") {
    std::atomic<size_t> counter{0};
    mk::WorkerThread worker;

    worker.submit([&counter]() {
        // Sleep for one second so that interrupt() is most likely
        // called _before_ subsequent jobs are executed.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        counter += 1;
    });

    // If this is ever executed, the test will fail
    worker.submit([]() { throw std::runtime_error("should_not_happen"); });

    // Make sure the worker thread can start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    worker.interrupt();

    while (counter <= 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

TEST_CASE("~WorkerThread interrupts the worker thread") {
    std::atomic<size_t> counter{0};
    {
        mk::WorkerThread worker;

        worker.submit([&counter]() {
            // Sleep for one second so that interrupt() is most likely
            // called _before_ subsequent jobs are executed.
            std::this_thread::sleep_for(std::chrono::seconds(1));
            counter += 1;
        });

        // If this is ever executed, the test will fail
        worker.submit([]() { throw std::runtime_error("should_not_happen"); });

        // Make sure the worker thread can start waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    while (counter <= 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}
