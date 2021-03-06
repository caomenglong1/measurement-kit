// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#define MK_ENGINE_INTERNALS
#include <measurement_kit/engine.h>

#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <sstream>
#include <thread>
#include <tuple>
#include <type_traits>

#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/logger.hpp>
#include <measurement_kit/common/nlohmann/json.hpp>
#include <measurement_kit/common/reactor.hpp>
#include <measurement_kit/common/shared_ptr.hpp>

#include "src/libmeasurement_kit/nettests/runnable.hpp"

namespace mk {
namespace engine {

// # Multi-thread stuff
//
// Comes first because it needs more careful handling.

class Semaphore {
  public:
    Semaphore() = default;

    void acquire() {
        std::unique_lock<std::mutex> lock{mutex_};
        cond_.wait(lock, [this]() { return !active_; });
        active_ = true;
    }

    void release() {
        {
            std::unique_lock<std::mutex> _{mutex_};
            active_ = false;
        }
        cond_.notify_one();
    }

    ~Semaphore() = default;

  private:
    bool active_ = false;
    std::condition_variable cond_;
    std::mutex mutex_;
};

class TaskImpl {
  public:
    std::condition_variable cond;
    std::deque<nlohmann::json> deque;
    std::atomic_bool interrupted{false};
    std::mutex mutex;
    SharedPtr<Reactor> reactor = Reactor::make();
    std::atomic_bool running{false};
    std::thread thread;
};

static void task_run(TaskImpl *pimpl, nlohmann::json &settings);
static bool is_event_valid(const std::string &);

static void emit(TaskImpl *pimpl, nlohmann::json &&event) {

    // In debug mode, make sure that we're emitting an event that we know
    assert(event.is_object());
    assert(event.count("type") != 0);
    assert(event.at("type").is_string());
    assert(is_event_valid(event.at("type").get<std::string>()));

    // Actually emit the event.
    {
        std::unique_lock<std::mutex> _{pimpl->mutex};
        pimpl->deque.push_back(std::move(event));
    }
    pimpl->cond.notify_all(); // more efficient if unlocked
}

Task::Task(nlohmann::json &&settings) {
    pimpl_ = std::make_unique<TaskImpl>();
    std::promise<void> barrier;
    std::future<void> started = barrier.get_future();
    pimpl_->thread = std::thread([this, &barrier,
                                         settings = std::move(
                                                 settings)]() mutable {
        pimpl_->running = true;
        barrier.set_value();
        static Semaphore semaphore;
        semaphore.acquire(); // block until a previous task has finished running
        task_run(pimpl_.get(), settings);
        pimpl_->running = false;
        pimpl_->cond.notify_all(); // tell the readers we're done
        semaphore.release();       // allow another task to run
    });
    started.wait(); // guarantee Task() completes when the thread is running
}

bool Task::is_running() const {
    return pimpl_->running; // is an atomic var
}

void Task::interrupt() {
    // both variables are safe to use in a MT context
    pimpl_->reactor->stop();
    pimpl_->interrupted = true;
}

nlohmann::json Task::wait_for_next_event() {
    std::unique_lock<std::mutex> lock{pimpl_->mutex};
    // purpose: block here until we stop running or we have events to read
    pimpl_->cond.wait(lock, [this]() { //
        return !pimpl_->running || !pimpl_->deque.empty();
    });
    // must be first so we drain the queue before emitting the final null
    if (!pimpl_->deque.empty()) {
        auto rv = std::move(pimpl_->deque.front());
        pimpl_->deque.pop_front();
        return nlohmann::json{std::move(rv)};
    }
    assert(!pimpl_->running);
    return nlohmann::json(); // this is a `null` JSON object
}

Task::~Task() {
    if (pimpl_->thread.joinable()) {
        pimpl_->thread.join();
    }
}

// # Helpers

static std::tuple<int, bool> verbosity_atoi(const std::string &str) {
#define ATOI(value)                                                            \
    if (str == #value) {                                                       \
        return std::make_tuple(MK_LOG_##value, true);                          \
    }
    MK_ENUM_VERBOSITY(ATOI)
#undef ATOI
    return std::make_tuple(0, false);
}

static std::tuple<std::string, bool> verbosity_itoa(int n) {
#define ITOA(value)                                                            \
    if (n == MK_LOG_##value) {                                                 \
        return std::make_tuple(std::string{#value}, true);                     \
    }
    MK_ENUM_VERBOSITY(ITOA)
#undef ITOA
    return std::make_tuple(std::string{}, false);
}

// TODO(hellais): specify the format of the events according to the following
// guidelines that we agreed upon:
//
// 1. events should be nested like {"type": "string", "value": {...}} since
// that simplifies their processing, especially in golang
//
// 2. the event key should be such that one can "switch" using the key and no
// other comparison in theory should be needed to understand the event type
//
// Currently emitted events do not follow the above guidelines because we've
// not finished specifying the events format.

static nlohmann::json make_log_event(uint32_t verbosity, const char *message) {
    auto verbosity_tuple = verbosity_itoa(verbosity);
    assert(std::get<1>(verbosity_tuple));
    const std::string &vs = std::get<0>(verbosity_tuple);
    return nlohmann::json{
            {"type", "LOG"}, {"verbosity", vs}, {"message", message}};
}

static nlohmann::json make_failure_event(const Error &error) {
    return nlohmann::json{{"type", "FAILURE"}, {"failure", error.reason}};
}

static bool is_event_valid(const std::string &str) {
    bool rv = false;
    do {
#define CHECK(value)                                                           \
    if (#value == str) {                                                       \
        rv = true;                                                             \
        break;                                                                 \
    }
        MK_ENUM_EVENT(CHECK)
#undef CHECK
    } while (0);
    return rv;
}

static nlohmann::json known_events() {
    nlohmann::json json;
#define ADD(name) json.push_back(#name);
    MK_ENUM_EVENT(ADD)
#undef ADD
    return json;
}

static std::string known_tasks() {
    nlohmann::json json;
#define ADD(name) json.push_back(#name);
    MK_ENUM_TASK(ADD)
#undef ADD
    return json.dump();
}

static std::string known_verbosity_levels() {
    nlohmann::json json;
#define ADD(name) json.push_back(#name);
    MK_ENUM_VERBOSITY(ADD)
#undef ADD
    return json.dump();
}

static std::unique_ptr<nettests::Runnable> make_runnable(const std::string &s) {
    std::unique_ptr<nettests::Runnable> runnable;
#define ATOP(value)                                                            \
    if (s == #value) {                                                         \
        runnable.reset(new nettests::value##Runnable);                         \
    }
    MK_ENUM_TASK(ATOP)
#undef ATOP
    return runnable;
}

static void emit_settings_failure(TaskImpl *pimpl, const char *reason) {
    emit(pimpl, make_log_event(MK_LOG_ERR, reason));
    emit(pimpl, make_failure_event(ValueError()));
}

static void emit_settings_warning(TaskImpl *pimpl, const char *reason) {
    emit(pimpl, make_log_event(MK_LOG_WARNING, reason));
}

static bool validate_known_settings_shallow(
        TaskImpl *pimpl, const nlohmann::json &settings) {
    auto rv = true;

#define VALIDATE(name, type, mandatory)                                        \
                                                                               \
    /* Make sure that mandatory settings are present */                        \
    if (!settings.count(#name) && mandatory) {                                 \
        std::stringstream ss;                                                  \
        ss << "missing required setting '" << #name << "' (fyi: '" << #name    \
           << "' should be a " << #type << ")";                                \
        emit_settings_warning(pimpl, ss.str().data());                         \
        rv = false;                                                            \
    }                                                                          \
                                                                               \
    /* Make sure that existing settings have the correct type. */              \
    if (settings.count(#name) && !settings.at(#name).is_##type()) {            \
        std::stringstream ss;                                                  \
        ss << "found setting '" << #name << "' with invalid type (fyi: '"      \
           << #name << "' should be a " << #type << ")";                       \
        emit_settings_warning(pimpl, ss.str().data());                         \
        rv = false;                                                            \
    }

    MK_ENUM_SETTINGS(VALIDATE)
#undef VALIDATE

    return rv;
}

// TODO(bassosimone): decide whether this should instead stop processing.
static void remove_unknown_settings_and_warn(
        TaskImpl *pimpl, nlohmann::json &settings) {
    std::set<std::string> expected;
    std::set<std::string> unexpected;
#define FILL(name, type, mandatory) expected.insert(#name);
    MK_ENUM_SETTINGS(FILL)
#undef FILL
    for (auto it : nlohmann::json::iterator_wrapper(settings)) {
        const auto &key = it.key();
        if (expected.count(key) <= 0) {
            std::stringstream ss;
            ss << "found unknown setting key " << key << " which will be "
               << "ignored by Measurement Kit";
            unexpected.insert(key);
            emit_settings_warning(pimpl, ss.str().data());
        }
    }
    for (auto &s : unexpected) {
        settings.erase(s);
    }
}

// # Run task

static void task_run(TaskImpl *pimpl, nlohmann::json &settings) {

    // make sure that `settings` is an object
    if (!settings.is_object()) {
        std::stringstream ss;
        ss << "invalid `settings` type: the `settings` JSON that you pass me "
            << "should be a JSON object (i.e. '{\"type\": \"Ndt\"}') but "
            << "instead you passed me this: '" << settings.dump() << "'";
        emit_settings_failure(pimpl, ss.str().data());
        return;
    }

    // Make sure that the toplevel settings are okay, remove unknown ones, so
    // there cannot be code below attempting to access settings that are not
    // specified also inside of the <engine.h> header file.
    if (!validate_known_settings_shallow(pimpl, settings)) {
        emit_settings_failure(pimpl, "failed to validate settings");
        return;
    }
    remove_unknown_settings_and_warn(pimpl, settings);

    // extract and process `type`
    auto runnable = make_runnable(settings.at("type").get<std::string>());
    if (!runnable) {
        std::stringstream ss;
        ss << "unknown task type '" << settings.at("type").get<std::string>()
            << "' (fyi: known tasks are: " << known_tasks() << ")";
        emit_settings_failure(pimpl, ss.str().data());
        return;
    }
    runnable->reactor = pimpl->reactor; // default is nullptr, we must set it

    // extract and process `options`
    if (settings.count("options") != 0) {
        auto &options = settings.at("options");
        // TODO(bassosimone): enumerate all possible options. Make sure we
        // check their type, warn about unknown options (but don't remove them
        // for now, as it will take time to map all of them).
        for (auto it : nlohmann::json::iterator_wrapper(options)) {
            const auto &key = it.key();
            auto &value = it.value();
            if (value.is_string()) {
                const auto &v = value.get<std::string>();
                // Using emplace() as a workaround for bug #1550.
                runnable->options.emplace(key, v);
            } else if (value.is_number_integer()) {
                auto v = value.get<int64_t>();
                runnable->options[key] = v;
            } else if (value.is_number_float()) {
                auto v = value.get<double>();
                runnable->options[key] = v;
            } else {
                std::stringstream ss;
                ss << "Found option '" << key << "' to have an invalid type"
                    << " (fyi: valid option types are: int, double, string)";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
        }
    }

    // extract and process `verbosity`
    {
        uint32_t verbosity = MK_LOG_QUIET;
        if (settings.count("verbosity") != 0) {
            auto verbosity_string = settings.at("verbosity").get<std::string>();
            auto verbosity_tuple = verbosity_atoi(verbosity_string);
            bool okay = std::get<1>(verbosity_tuple);
            if (!okay) {
                std::stringstream ss;
                ss << "Unknown verbosity level '" << verbosity_string << "' "
                    << "(fyi: known verbosity levels are: " <<
                    known_verbosity_levels() << ")";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
            verbosity = std::get<0>(verbosity_tuple);
        }
        runnable->logger->set_verbosity(verbosity);
    }

    // Mask out events that are user-disabled.
    std::set<std::string> enabled_events = known_events();
    if (settings.count("disabled_events") != 0) {
        for (auto &entry : settings.at("disabled_events")) {
            if (!entry.is_string()) {
                std::stringstream ss;
                ss << "Found invalid entry inside of disabled_events that "
                  << "has value equal to <" << entry.dump() << "> (fyi: all "
                  << "the entries in disabled_events must be strings";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
            std::string s = entry.get<std::string>();
            if (!is_event_valid(s)) {
                std::stringstream ss;
                ss << "Found unknown event inside of disabled_events with "
                   << "name '" << s << "' (fyi: all valid events are: "
                   << known_events().dump() << "). Measurement Kit is going "
                   << "to ignore this invalid event and continue";
                emit_settings_warning(pimpl, ss.str().data());
                continue;
            }
            enabled_events.erase(s);
        }
    }

    // TODO(bassosimone): add code for processing more event types.

    // see whether 'PERFORMANCE' is enabled
    // TODO(bassosimone): adapt this event according to spec when @hellais will
    // have finalized the events specification.
    //
    // TODO(bassosimone): currently events are emitted using the logger and as
    // such they're subject to the verosity, which is really a bummer.
    if (enabled_events.count("PERFORMANCE") != 0) {
        runnable->logger->on_event([pimpl](const char *line) {
            nlohmann::json event;
            try {
                auto inner = nlohmann::json::parse(line);
                if (inner.at("type") == "download-speed") {
                    event["direction"] = "download";
                } else if (inner.at("type") == "upload-speed") {
                    event["direction"] = "upload";
                } else {
                    assert(false);
                    return; // Not an event we wanted to filter
                }
                event["type"] = "PERFORMANCE";
                event["elapsed_seconds"] = inner["elapsed"][0];
                event["num_streams"] = inner["num_streams"];
                event["speed_kbit_s"] = inner["speed"][0];
            } catch (const std::exception &) {
                assert(false);
                return; // Perhaps not the right event format
            }
            emit(pimpl, std::move(event));
        });
    }

    // see whether 'LOG' is enabled
    // TODO(bassosimone): adapt this event according to spec when @hellais will
    // have finalized the events specification.
    if (enabled_events.count("LOG") != 0) {
        runnable->logger->on_log([pimpl](uint32_t verbosity, const char *line) {
            if ((verbosity & ~MK_LOG_VERBOSITY_MASK) != 0) {
                return; // mask out non-logging events
            }
            emit(pimpl, make_log_event(verbosity, line));
        });
    } else {
        // Here we should silence the logger but we cannot do that since events
        // and logs are deeply related. So our second best is to just set up
        // a dummy logger that prevents output from going on stderr.
        //
        // TODO(bassosimone): decouple logging and events.
        runnable->logger->on_log([](uint32_t, const char *) { /* NOTHING */ });
    }

    // start the task (reactor and interrupted are MT safe)
    pimpl->reactor->run_with_initial_event([&]() {
        if (pimpl->interrupted) {
            return; // allow for early interruption
        }
        runnable->begin([&](Error) {
            runnable->end([&](Error) {
                // NOTHING
            });
        });
    });
}

} // namespace engine
} // namespace mk
