// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

// This file implements Measurement Kit public API.

#include <measurement_kit/mk.h>

#include <measurement_kit/common/nlohmann/json.hpp>

// Implementation note: since this API is expected to be used through FFI or
// through SWIG generated wrappers, we need to place extra care into making sure
// that we gracefully accept `nullptr` in input.

// # API

int mk_get_api_major(void) noexcept { return MK_API_MAJOR; }

int mk_get_api_minor(void) noexcept { return MK_API_MINOR; }

// # Event
//
// This is just a wrapper around the very flexible nlohmann::json. We decided
// to put the event type inside of the object itself, so that a Node.js consumer
// can skip also the event checking, and directly use the serialization.

struct mk_event_s : public nlohmann::json {
    using nlohmann::json::son;
};

#define EVTYPE_KEY "event_type" // Avoid possible typos

static mk_event_t *mk_event_create(const char *type) noexcept {
#define XX(event_name)                                                         \
    if (strcmp(#event_name, type) == 0) {                                      \
        mk_event_t *event = new mk_event_t{};                                  \
        (*event)[EVTYPE_KEY] = type;                                           \
        return event;                                                          \
    }
    MK_ENUMERATE_EVENT_TYPES(XX)
#undef XX
    mk::warn("mk_event_create: invalid event: %s", type);
    assert(false);
    return nullptr;
}

const char *mk_event_get_type(mk_event_t *event) noexcept {
    return (event) ? (*event)[EVTYPE_KEY].get<std::string>().data() : nullptr;
}

const char *mk_event_as_serialized_json(mk_event_t *event) noexcept {
    return (event) ? event->dump() : nullptr;
}

MK_BOOL mk_event_has_null_entry(mk_event_t *event, const char *key) noexcept {
    return (event) && (key) && (event->count(key)) && (*event)[key].is_null();
}

MK_BOOL mk_event_has_string_entry(mk_event_t *event, const char *key) noexcept {
    return (event) && (key) && (event->count(key)) && (*event)[key].is_string();
}

const char *mk_event_get_string_entry(
        mk_event_t *event, const char *key) noexcept {
    return ((event) && (key) && (event->count(key)))
                   ? (*event)[key].get<const char *>()
                   : nullptr;
}

MK_BOOL mk_event_has_int_entry(mk_event_t *event, const char *key) noexcept {
    return (event) && (key) && (event->count(key)) &&
           (*event)[key].is_number_integer();
}

int mk_event_get_int_entry(mk_event_t *event, const char *key) noexcept {
    return ((event) && (key) && (event->count(key))) ? (*event)[key].get<int>()
                                                     : 0;
}

MK_BOOL mk_event_has_double_entry(mk_event_t *event, const char *key) noexcept {
    return (event) && (key) && (event->count(key)) &&
           (*event)[key].is_number_float();
}

double mk_event_get_double_entry(mk_event_t *event, const char *key) noexcept {
    return ((event) && (key) && (event->count(key)))
                   ? (*event)[key].get<double>()
                   : 0.0;
}

MK_BOOL mk_event_has_array_entry(mk_event_t *event, const char *key) noexcept {
    return (event) && (key) && (event->count(key)) && (*event)[key].is_array();
}

const char *mk_event_get_serialized_array_entry(
        mk_event_t *event, const char *key) noexcept {
    return (event) && (key) && (event->count(key)) ? (*event)[key].dump()
                                                   : nullptr;
}

MK_BOOL mk_event_has_object_entry(mk_event_t *event, const char *key) noexcept {
    return (event) && (key) && (event->count(key)) && (*event)[key].is_object();
}

const char *mk_event_get_serialized_object_entry(
        mk_event_t *event, const char *key) noexcept {
    return ((event) && (key) && (event->count(key))) ? (*event)[key].dump()
                                                     : nullptr;
}

void mk_event_destroy(mk_event_t *event) noexcept {
#ifndef NDEBUG
    // This is what Arturo would call a "Leonidism". An ex post check to
    // ensure that no other piece of code changed the event type.
    if ((event)) {
        bool event_type_okay = false;
        if ((event->count(EVTYPE_KEY)) && (*event)[EVTYPE_KEY].is_string()) {
            const char *evtype = (*event)[EVTYPE_KEY].get<std::string>().data();
#define XX(event_name)                                                         \
    if (strcmp(#event_name, evtype) == 0) {                                    \
        event_type_okay = true;                                                \
    }
            MK_ENUMERATE_EVENT_TYPES(XX)
#undef XX
        }
        assert(event_type_okay);
    }
#endif
    delete event; // Note that `delete` handles nullptr gracefully
}

// # Task
//
// Implemented as a wrapper around the internal `Runnable` class.
// XXX

struct mk_task_s {
    std::condition_variable condition;
    std::deque<std::unique_ptr<mk_event_t>> deque;
    uint32_t enabled = MK_EVENT_END;
    std::atomic_bool interrupted = false;
    std::mutex mutex;
    std::unique_ptr<mk::nettests::Runnable> runnable;
    std::thread thread;
    std::string type;
};

mk_task_t *mk_task_create(const char *type) noexcept {
#define XX(task_name)                                                          \
    if (strcmp(#task_name, type) == 0) {                                       \
        auto task = std::make_unique<mk_task_t>();                             \
        task->runnable = std::make_unique<task_name##Runnable>();              \
        task->runnable->reactor = mk::Reactor::make();                         \
        task->type = type;                                                     \
        return task.relase();                                                  \
    }
    MK_ENUM_TASK_TYPES(XX)
#undef XX
    mk::warn("mk_task_create: invalid task: %s", type);
    assert(false);
    return nullptr;
}

// FIXME:
//
// 1. come gestisco la thread safety?
//
// 2. come gestisco la distruzione finale?

char *mk_task_get_type(const mk_task_t *task) noexcept {
    return ((task)) ? task->type.data() : nullptr;
}

// TODO: use nlohmann::json for annotations

void mk_task_add_string_annotation(
        mk_task_t *task, const char *key, const char *value) noexcept {
    if ((task) && (key) && (value)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->annotations[key] = value;
    }
}

void mk_task_add_int_annotation(
        mk_task_t *task, const char *key, int value) noexcept {
    if ((task) && (key)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->annotations[key] = value;
    }
}

void mk_task_add_double_annotation(
        mk_task_t *task, const char *key, double value) noexcept {
    if ((task) && (key)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->annotations[key] = value;
    }
}

void mk_task_add_input(mk_task_t *task, const char *input) noexcept {
    if ((task) && (input)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->inputs.push_back(input);
    }
}

void mk_task_add_input_file(mk_task_t *task, const char *path) noexcept {
    if ((task) && (path)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->input_filepaths.push_back(path);
    }
}

void mk_task_set_verbosity(mk_task_t *task, const char *verbosity) noexcept {
    if (!task || !verbosity) {
        return;
    }
    if (task->thread.is_joinable()) {
        abort();
    }
#define XX(verbosity_name)                                                     \
    if (strcmp(#verbosity_name, verbosity) == 0) {                             \
        task->runnable->logger->set_verbosity(MK_LOG_##verbosity_name);        \
        return;                                                                \
    }
    MK_ENUM_VERBOSITY_LEVELS(XX)
#undef XX
    mk::warn("mk_task_set_verbosity: unknown verbosity: %s", verbosity);
}

void mk_task_set_log_file(mk_task_t *task, const char *path) noexcept {
    if ((task) && (path)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->set_logfile(path);
    }
}

// TODO: improve the API of options

void mk_task_set_string_option(
        mk_task_t *task, const char *key, const char *value) noexcept {
    if ((task) && (key) && (value)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->options->set_string(key, value);
    }
}

void mk_task_set_int_option(
        mk_task_t *task, const char *key, int value) noexcept {
    if ((task) && (key)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->options->set_int(key, value);
    }
}

void mk_task_set_double_option(
        mk_task_t *task, const char *key, double value) noexcept {
    if ((task) && (key)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->options->set_double(key, value);
    }
}

MK_BOOL mk_task_set_options(
        mk_task_t *task, const char *serialized_json) noexcept {
    if (!task || !serialized_json) {
        return false;
    }
    if (task->thread.is_joinable()) {
        abort();
    }
    mk::warn("mk_task_set_options: not yet implemented");
    return false; // XXX not yet implemented
}

void mk_task_set_output_file(mk_task_t *task, const char *path) noexcept {
    if ((task) && (path)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->runnable->output_filepath(path);
    }
}

void mk_task_enable_event(mk_task_t *task, const char *type) noexcept {
    if (!task || !type) {
        return;
    }
    if (task->thread.is_joinable()) {
        abort();
    }
#define XX(event_name)                                                         \
    if (strcmp(#event_name, type) == 0) {                                      \
        task->enabled |= MK_EVENT_##event_name;                                \
        return;                                                                \
    }
    MK_ENUM_EVENT_TYPES(XX)
#undef XX
    mk::warn("mk_task_enable_event: unknown event: %s", type);
}

void mk_task_enable_all_events(mk_task_t *task) noexcept {
    if ((task)) {
        if (task->thread.is_joinable()) {
            abort();
        }
        task->enabled |= ~0;
    }
}

static void mk_task_post(mk_task_t *task, const char *event_type,
        std::function<void(mk_event_t &)> &&edit = nullptr) {
    std::unique_ptr<mk_event_t> event{mk_event_create(event_type)};
    if ((edit)) {
        edit(*event);
    }
    {
        std::unique_lock<std::mutex> lock{task->mutex};
        task->deque.push_back(std::move(event));
    }
    task->condition.notify_one();
}

static void mk_task_main(mk_task_t *task) noexcept {
    if ((task->enabled & MK_EVENT_LOG)) {
        task->runner->log->on_log([task](uint32_t severity, const char *line) {
            mk_task_post(task, "LOG", [&](mk_event_t &event) {
                switch (severity) {
                case MK_LOG_WARNING:
                    event["verbosity"] = "WARNING";
                    break;
                case MK_LOG_INFO:
                    event["verbosity"] = "INFO";
                    break;
                case MK_LOG_DEBUG:
                    event["verbosity"] = "DEBUG";
                    break;
                case MK_LOG_DEBUG2:
                    event["verbosity"] = "DEBUG2";
                    break;
                }
                event["message"] = line;
            });
        });
    }
    if ((task->enabled & MK_EVENT_CONFIGURED)) {
        // XXX
    }
    if ((task->enabled & MK_EVENT_PROGRESS)) {
        task->runner->log->on_progress(
                [task](double percentage, const char *message) {
                    mk_task_post(task, "PROGRESS", [&](mk_event_t &event) {
                        event["percentage"] = percentage;
                        event["message"] = message;
                    });
                });
    }
    if ((task->enabled & MK_EVENT_PERFORMANCE)) {
        // XXX
    }
    if ((task->enabled & MK_EVENT_MEASUREMENT_ERROR)) {
        // XXX
    }
    if ((task->enabled & MK_EVENT_REPORT_SUBMISSION_ERROR)) {
        // XXX
    }
    if ((task->enabled & MK_EVENT_RESULT)) {
        task->runner->entry_cb = [task](std::string entry) {
            mk_task_post(task, "RESULT", [&](mk_event_t &event) {
                event.parse(entry); // FIXME: cannot put type in there then...
            });
        };
    }
    mk::DataUsage dusage;
    task->runner->data_usage_cb = [task, &](DataUsage du) { dusage = du; };
    // FIXME: code to compute error is broken.
    mk::Error err = NoError();
    bool final_state = false;
    task->runnable->reactor->run_with_initial_event([&]() {
        task->runnable->begin([&](Error error) {
            if ((error)) {
                err = error;
            }
            task->runnable->end([task](Error error) {
                if ((error) && !err) {
                    err = error;
                }
                final_state = true;
            });
        });
    });
    mk_task_post(task, "END", [&](mk_event_t &event) {
        if (final_state && err == NoError()) {
            event["failure"] = nullptr;
        } else {
            event["failure"] = err.reason;
        }
    });
}

void mk_task_start(mk_task_t *task) noexcept {
    if ((task)) {
        std::unique_lock<std::mutex> _{task->sync_start}; // Prevent races
        if (task->thread.is_joinable()) {
            return; // Start semantics is idempotent
        }
        task->thread = std::thread([task]() {
            if ((task->enabled & MK_EVENT_QUEUED)) {
                mk_task_post(task, "QUEUED");
            }
            ThreadsSemaphore::singleton()->wait();
            if ((task->enabled & MK_EVENT_STARTED)) {
                mk_task_post(task, "STARTED");
            }
            if (!task->interrupted) {
                mk_task_main(task);
            }
            ThreadsSemaphone::singleton()->signal();
            mk_task_post(task, "TERMINATED");
        });
    }
}

MK_BOOL mk_task_is_running(const mk_task_t *task) noexcept {
    return (task) && task->thread.is_joinable();
}

void mk_task_interrupt(mk_task_t *task) noexcept {
    if ((task)) {
        task->interrupted = true; // Needed to interrupt a waiting task
        assert((task->reactor));
        task->reactor->stop();
    }
}

mk_event_t *mk_task_wait_for_next_event(mk_task_t *task) noexcept {
    if (!task) {
        return nullptr;
    }
    if (!task->thread.is_joinable()) {
        mk::warn("mk_task_wait_for_next_event: thread not running");
        return nullptr;
    }
    std::unique_lock<std::mutex> lock{task->mutex};
    task->condition.wait(lock, [task]() { return task->deque.count() > 0; });
    auto front = std::move(task->deque.front());
    task->deque.pop_front();
    return front.release();
}

void mk_task_destroy(mk_task_t *task) noexcept {
    if ((task) && task->thread.is_joinable()) {
        mk_task_interrupt(task);
        task->thread.join();
    }
    delete task; // Note that `delete` handles nullptr gracefully
}
