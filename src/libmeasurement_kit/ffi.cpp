// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

// TODO(bassosimone): sync up the specification with this file. This file is
// actually the most feature-complete stuff and the spec needs update.

#include <measurement_kit/ffi.h>

#include <assert.h>

#include <measurement_kit/common/logger.hpp>
#include <measurement_kit/common/nlohmann/json.hpp>
#include <measurement_kit/version.h>

double mk_version() noexcept { MK_VERSION_MAJOR##.##MK_VERSION_MINOR; }

struct mk_event_s : public nlohmann::json {
    using nlohmann::json::json;
};

static mk_event_t *mk_event_create(const char *type) noexcept {
#define MK_EVENT_CREATE(event_type)                                            \
    if (strcmp(#event_type, type) == 0) {                                      \
        auto evp = std::make_unique<mk_event_t>();                             \
        (*evp)["type"] = type;                                                 \
        (*evp)["values"] = nlohmann::json::object();                           \
        return evp.release();                                                  \
    }
    MK_ENUM_EVENT_TYPES(MK_EVENT_CREATE)
#undef MK_EVENT_CREATE
    return nullptr;
}

#define MK_EVENT_HAS_ENTRY_IMPL(type_name, nlohmann_is_of_type)                \
    MK_BOOL mk_event_has_##type_name##_entry(                                  \
            const mk_event_t *evp, const char *key) noexcept {                 \
        if (evp != nullptr && key != nullptr) {                                \
            try {                                                              \
                return evp->at("values").at(key).nlohmann_is_of_type();        \
            } catch (const std::exception &) {                                 \
                /* NOTHING */;                                                 \
            }                                                                  \
        }                                                                      \
        return false;                                                          \
    }

MK_EVENT_HAS_ENTRY_IMPL(null, is_null)
MK_EVENT_HAS_ENTRY_IMPL(int, is_number_integer)
MK_EVENT_HAS_ENTRY_IMPL(double, is_number_float)
MK_EVENT_HAS_ENTRY_IMPL(string, is_string)
MK_EVENT_HAS_ENTRY_IMPL(array, is_array)
MK_EVENT_HAS_ENTRY_IMPL(object, is_object)

#undef MK_EVENT_HAS_ENTRY_IMPL

// Important! In some of the following code we're going to return `const char`
// pointers derived from _temporary_ strings. In C++98, these are guaranteed
// to last until the full chain of calls that lead to obtain the `const char *`
// finishes its execution. That is, it is _not_ safe to store these pointers
// and reuse them later. But it should be safe to return them and let the caller
// decide what would be the proper policy to store such pointers.
//
// See <https://stackoverflow.com/a/10006989>.

#define MK_EVENT_GETTER_IMPL(return_type, name, modifier)                      \
    return_type mk_event_get_##name##_entry(                                   \
            const mk_event_t *evp, const char *key) noexcept {                 \
        if (evp != nullptr && key != nullptr) {                                \
            try {                                                              \
                return modifier(evp->at("values"));                            \
            } catch (const std::exception &) {                                 \
                /* NOTHING */;                                                 \
            }                                                                  \
        }                                                                      \
        return (return_type)0;                                                 \
    }

#define AS_INT(v) (v).get<int>()
MK_EVENT_GETTER_IMPL(int, int, AS_INT)
#undef AS_INT

#define AS_DOUBLE(v) (v).get<double>()
MK_EVENT_GETTER_IMPL(double, double, AS_DOUBLE)
#undef AS_DOUBLE

#define AS_TEMPORARY_CSTRING(v) (v).get<std::string>().data()
MK_EVENT_GETTER_IMPL(const char *, string, AS_TEMPORARY_CSTRING)
#undef AS_TEMPORARY_CSTRING

#define AS_TEMPORARY_DUMP(v) (v).dump().data()
MK_EVENT_GETTER_IMPL(const char *, serialized, AS_TEMPORARY_DUMP)
#undef AS_TEMPORARY_DUMP

#undef MK_EVENT_GETTER_IMPL // tidy

const char *mk_event_serialize(const mk_event_t *evp) noexcept {
    if (evp != nullptr) {
        return evp->dump().data(); // temporary!
    }
    return nullptr;
}

void mk_event_destroy(const mk_event_t *evp) noexcept {
    delete evp; // handles nullptr
}

struct mk_settings_s : public nlohmann::json {
    using nlohmann::json::json;
};

#define ENUM_TOPLEVEL_KEYS(XX)                                                 \
    XX(type, string, "")                                                       \
    XX(values, object, object())

#define ENUM_VALUES_KEYS(XX)                                                   \
    XX(annotations, object, object())                                          \
    XX(inputs, array, array())                                                 \
    XX(input_files, array, array())                                            \
    XX(verbosity, string, "QUIET")                                             \
    XX(log_file, string, "")                                                   \
    XX(options, object, object())                                              \
    XX(output_file, string, "")                                                \
    XX(enabled_events, array, array()

#define DO_TOPLEVEL_KEYS(name, type, initializer)                              \
    if (json.count(#name) != 0) {                                              \
        if (!json.at(#name).is_##type) {                                       \
            mk::warn("invalid type for toplevel key: %s", #name);              \
            return false;                                                      \
        }                                                                      \
    } else {                                                                   \
        json.at(#name) = initializer;                                          \
    }

#define DO_VALUES_KEYS(name, type, initializer)                                \
    if (json.at("values").count(#name) != 0) {                                 \
        if (!json.at("values").at(#name).is_##type) {                          \
            mk::warn("invalid type for values key: %s", #name);                \
            return false;                                                      \
        }                                                                      \
    } else {                                                                   \
        json.at("values").at(#name) = initializer;                             \
    }

static bool validate_or_possibly_fix(nlohmann::json &json) noexcept {
    if (!json.is_object()) {
        mk::warn("invalid type for root object");
        return false;
    }
    ENUM_TOPLEVEL_KEYS(DO_TOPLEVEL_KEYS)
    ENUM_VALUES_KEYS(DO_VALUES_KEYS)
    return true;
}

// big tidy
#undef ENUM_TOPLEVEL_KEYS
#undef MK_TEMPLAYE_VALUES_KEYS
#undef DO_TOPLEVEL_KEYS
#undef DO_VALUES_KEYS

mk_settings_t *mk_settings_create(const char *type) noexcept {
    if (type == nullptr) {
        return nullptr;
    }
    nlohmann::json json = nlohmann::json::object{{"type", type}};
    if (!validate_or_possibly_fix_json(json)) {
        abort(); // this should not occur
    }
    return new mk_settings_t{std::move(json)};
}

const char *mk_settings_serialize(const mk_settings_t *ttpl) noexcept {
    if (ttpl != nullptr) {
        return ttpl->dump().data(); // temporary!
    }
    return nullptr;
}

mk_settings_t *mk_settings_parse(const char *str) noexcept {
    if (str != nullptr) {
        nlohmann::json json;
        try {
            json = nlohmann::json::parse(str);
        } catch (const std::exception &) {
            return nullptr;
        }
        if (!validate_or_possibly_fix(json)) {
            return nullptr;
        }
        return new mk_settings_t{std::move(json)};
    }
    return nullptr;
}

const char *mk_settings_get_type(const mk_settings_t *ttpl) noexcept {
    if (ttpl != nullptr) {
        return ttpl->at("type").get<std::string>().data(); // temporary!
    }
    return nullptr;
}

void mk_settings_add_string_annotation(
        mk_settings_t *ttpl, const char *key, const char *value) noexcept {
    if (ttpl != nullptr && key != nullptr && value != nullptr) {
        ttpl->at("values").at("annotations")[key] = value;
    }
}

void mk_settings_add_int_annotation(
        mk_settings_t *ttpl, const char *key, int value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("annotations")[key] = value;
    }
}

void mk_settings_add_double_annotation(
        mk_settings_t *ttpl, const char *key, double value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("annotations")[key] = value;
    }
}

void mk_settings_add_input(mk_settings_t *ttpl, const char *input) noexcept {
    if (ttpl != nullptr && input != nullptr) {
        ttpl->at("values").at("inputs").push_back(input);
    }
}

void mk_settings_add_input_file(
        mk_settings_t *ttpl, const char *path) noexcept {
    if (ttpl != nullptr && path != nullptr) {
        ttpl->at("values").at("input_files").push_back(path);
    }
}

void mk_settings_set_verbosity(
        mk_settings_t *ttpl, const char *verbosity) noexcept {
    if (ttpl != nullptr && verbosity != nullptr) {
        ttpl->at("values").at("verbosity") = verbosity;
    }
}

void mk_settings_set_log_file(mk_settings_t *ttpl, const char *path) noexcept {
    if (ttpl != nullptr && path != nullptr) {
        ttpl->at("values").at("log_file") = path;
    }
}

void mk_settings_set_string_option(
        mk_settings_t *ttpl, const char *key, const char *value) noexcept {
    if (ttpl != nullptr && key != nullptr && value != nullptr) {
        ttpl->at("values").at("options")[key] = value;
    }
}

void mk_settings_set_int_option(
        mk_settings_t *ttpl, const char *key, int value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("options")[key] = value;
    }
}

void mk_settings_set_double_option(
        mk_settings_t *ttpl, const char *key, double value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("options")[key] = value;
    }
}

void mk_settings_set_output_file(
        mk_settings_t *ttpl, const char *path) noexcept {
    if (ttpl != nullptr && path != nullptr) {
        ttpl->at("values").at("output_file") = path;
    }
}

void mk_settings_enable_event(mk_settings_t *ttpl, const char *type) noexcept {
    if (ttpl != nullptr && type != nullptr) {
        ttpl->at("values").at("enabled_events").push_back(type);
    }
}

void mk_settings_enable_all_events(mk_settings_t *ttpl) noexcept {
    mk_settings_enable_event("ALL");
}

void mk_settings_destroy(const mk_settings_t *ttpl) noexcept {
    delete ttpl; // handles nullptr
}

struct mk_task_s {
    std::condition_variable cond;
    std::deque<std::unique_ptr<const mk_event_t>> deque;
    int flags = 0;
#define F_START (1 << 0)     // prevent racing on start
#define F_RUN (1 << 1)       // ensure we delete after we've stopped
#define F_INTERRUPT (1 << 2) // allow for early interrupt
    std::mutex mutex;
    mk::SharedPtr<mk::Reactor> reactor = mk::Reactor::make();
    mk_settings ttpl;
};

// We implement the most tricky code parts here and defer the less tricky
// code parts to static functions available at the bottom of this file.
static void mk_task_main_unlocked(mk_tast_t *task) noexcept;

mk_task_t *mk_task_create(const mk_settings_t *ttpl) noexcept {
    if (ttpl == nullptr) {
        return nullptr;
    }
    auto task = new mk_task_t{};
    task->ttpl = *ttpl; // makes a copy - separates the lifecycles
    return ttpl;
}

void mk_task_start(mk_task_t *task) noexcept {
    if (task != nullptr) {
        std::unique_lock<std::mutex> _{task->mutex};
        if ((task->flags & F_START) != 0) {
            return; // Prevent racing on starting a task
        }
        task->flags |= F_START | F_RUN;
        // TODO(bassosimone): this code would be more robust if we could avoid
        // using detached threads. Refactoring to be done once we start to have
        // this API as something testable and we implement other APIs using
        // as base implementation this API.
        Worker::default_tasks_queue()->call_in_thread(
                mk::Logger::global(), [task]() {
                    task->reactor->run_with_initial_event([task]() {
                        {
                            std::unique_lock<std::mutex> _{task->mutex};
                            if ((task->flags & F_INTERRUPT) != 0) {
                                return; // Catch here early interrupt
                            }
                        }
                        mk_task_main_unlocked(task);
                    });
                    // Unblock people blocked on destroy or wait_for_next_event
                    {
                        std::unique_lock<std::mutex> _{task->mutex};
                        task->flags &= ~F_RUN;
                    }
                    // okay to call unlocked - must be notify_all()!!!
                    task->cond.notify_all();
                });
    }
}

void mk_task_interrupt(mk_task_t *task) noexcept {
    if (task != nullptr) {
        std::unique_lock<std::mutex> _{task->mutex};
        if ((task->flags & (F_START | F_RUN)) != 0) {
            // TODO(bassosimone): ideally it would simplify the implementation
            // if we could (and we can) move this flag inside of the reactor,
            // which we can do quite easily as a refactoring, implementing the
            // Reactor::interrupt() method as a non-restartable stop().
            task->flags |= F_INTERRUPT; // allow for early interrupt
            task->reactor->stop();      // is thread safe
        }
    }
}

const mk_event_t *mk_task_wait_for_next_event(mk_task_t *task) noexcept {
    if (task != nullptr) {
        std::unique_lock<std::mutex> lock{task->mutex};
        // Block here until we are stopped or we have something to process
        task->cond.wait(lock, [&]() {
            return (task->running & F_RUN) == 0 || !task->deque.empty();
        });
        if ((task->running & F_RUN) == 0) {
            return mk_event_create("EOF");
        }
        auto evp = std::move(task->deque.front());
        task->deque.pop_front();
        return evp.release();
    }
    return nullptr;
}

static void mk_task_post_event(
        mk_task_t *task, std::unique_ptr<const mk_event_t> event) noexcept {
    // internal function so just assert
    assert(task);
    assert(event);
    // Post event and unblock people blocked on destroy or wait_for_next_event
    {
        std::unique_lock<std::mutex> lock{task->mutex};
        task->deque.push_back(std::move(event));
    }
    task->cond.notify_all(); // okay to call unlocked - must be notify_all()!!!
}

void mk_task_destroy(mk_task_t *task) noexcept {
    if (task != nullptr) {
        {
            std::unique_lock<std::mutex> lock{task->mutex};
            if ((taks->flags & F_START) != 0) {
                // If we've been started, wait until we are stopped.
                //
                // TODO(bassosimone): as explained above, this would be more
                // robust if we could express it as joining a thread after
                // we have checked whether the thread was joinable.
                task->cond.wait(
                        lock, [task]() { return (task->flags & F_RUN) == 0; });
            }
        }
        // We must destroy with unlocked mutex
        delete task;
    }
}

static void mk_task_main_unlocked(mk_tast_t *task) noexcept {
    // TODO(bassosimone): here we will implement the task. It should basically
    // get the task template and configure the task accordingly. Routing the
    // events occurring as callbacks to events poste on the queue should be easy
    // given that the tricky part are already implemented above.
}
