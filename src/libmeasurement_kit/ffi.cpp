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

struct mk_serialization_t : public std::string {
    using std::string::string;
};

static mk_serialization_t *mk_serialization_create(
        const std::string &s) noexcept {
    return new mk_serialization_t{s};
}

const char *mk_serialization_get_string(const mk_serialization_t *s) noexcept {
    return ((s)) ? s.data() : nullptr;
}

void mk_serialization_destroy(mk_serialization_t *s) noexcept {
    delete s; // handles nullptr
}

struct mk_event_s : public nlohmann::json {
    using nlohmann::json::json;
};

static mk_event_t *mk_event_create(const char *type) noexcept {
    auto evp = std::make_unique<mk_event_t>();
    // TODO(bassosimone): make sure the event type is valid.
    (*evp)["type"] = type;
    (*evp)["values"] = nlohmann::json::object();
    return evp.release();
}

MK_BOOL mk_event_has_null_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).is_null();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return false;
}

MK_BOOL mk_event_has_int_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).is_number_integer();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return false;
}

MK_BOOL mk_event_has_double_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).is_number_float();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return false;
}

MK_BOOL mk_event_has_string_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).is_string();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return false;
}

MK_BOOL mk_event_has_array_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).is_array();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return false;
}

MK_BOOL mk_event_has_object_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).is_object();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return false;
}

int mk_event_get_int_entry(const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).get<int>();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return 0;
}

double mk_event_get_double_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).get<double>();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return 0.0;
}

const char *mk_event_get_string_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return evp->at("values").at(key).get<std::string>().data();
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return nullptr;
}

mk_serialization_t *mk_event_serialize_entry(
        const mk_event_t *evp, const char *key) noexcept {
    if (evp != nullptr && key != nullptr) {
        try {
            return mk_serialization_create(evp->at("values").at(key).dump());
        } catch (const std::exception &) {
            /* NOTHING */;
        }
    }
    return nullptr;
}

mk_serialization_t *mk_event_serialize(const mk_event_t *evp) noexcept {
    if (evp != nullptr) {
        return mk_serialization_create(evp->dump());
    }
    return nullptr;
}

void mk_event_destroy(const mk_event_t *evp) noexcept {
    delete evp; // handles nullptr
}

struct mk_task_template_s : public nlohmann::json {
    using nlohmann::json::json;
};

#define TOPLEVEL_KEYS(XX)                                                      \
    XX(type, string, "")                                                       \
    XX(values, object, object())

#define VALUES_KEYS(XX)                                                        \
    XX(annotations, object, object())                                          \
    XX(inputs, array, array())                                                 \
    XX(input_files, array, array())                                            \
    XX(verbosity, string, "QUIET")                                             \
    XX(log_file, string, "")                                                   \
    XX(options, object, object())                                              \
    XX(output_file, string, "")                                                \
    XX(enabled_events, array, array({"END"})

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
    TOPLEVEL_KEYS(DO_TOPLEVEL_KEYS)
    VALUES_KEYS(DO_VALUES_KEYS)
    return true;
}

#undef TOPLEVEL_KEYS
#undef MK_TEMPLAYE_VALUES_KEYS
#undef DO_TOPLEVEL_KEYS
#undef DO_VALUES_KEYS

mk_task_template_t *mk_task_template_create(const char *type) noexcept {
    if (type == nullptr) {
        return nullptr;
    }
    nlohmann::json json;
    json["type"] = type;
    if (!validate_or_possibly_fix_json(json)) {
        abort();
    }
    return new mk_task_template_t{std::move(json)};
}

mk_serialization_t *mk_task_template_serialize(
        const mk_task_template_t *ttpl) noexcept {
    if (ttpl != nullptr) {
        return mk_serialization_create(ttpl->dump());
    }
    return nullptr;
}

mk_task_template_t *mk_task_template_parse(const char *str) noexcept {
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
        return new mk_task_template_t{std::move(json)};
    }
    return nullptr;
}

const char *mk_task_template_get_type(const mk_task_template_t *ttpl) noexcept {
    if (ttpl != nullptr) {
        return ttpl->at("type").get<std::string>().data();
    }
    return nullptr;
}

void mk_task_template_add_string_annotation(
        mk_task_template_t *ttpl, const char *key, const char *value) noexcept {
    if (ttpl != nullptr && key != nullptr && value != nullptr) {
        ttpl->at("values").at("annotations")[key] = value;
    }
}

void mk_task_template_add_int_annotation(
        mk_task_template_t *ttpl, const char *key, int value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("annotations")[key] = value;
    }
}

void mk_task_template_add_double_annotation(
        mk_task_template_t *ttpl, const char *key, double value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("annotations")[key] = value;
    }
}

void mk_task_template_add_input(
        mk_task_template_t *ttpl, const char *input) noexcept {
    if (ttpl != nullptr && input != nullptr) {
        ttpl->at("values").at("inputs").push_back(input);
    }
}

void mk_task_template_add_input_file(
        mk_task_template_t *ttpl, const char *path) noexcept {
    if (ttpl != nullptr && path != nullptr) {
        ttpl->at("values").at("input_files").push_back(path);
    }
}

void mk_task_template_set_verbosity(
        mk_task_template_t *ttpl, const char *verbosity) noexcept {
    if (ttpl != nullptr && verbosity != nullptr) {
        ttpl->at("values").at("verbosity") = verbosity;
    }
}

void mk_task_template_set_log_file(
        mk_task_template_t *ttpl, const char *path) noexcept {
    if (ttpl != nullptr && path != nullptr) {
        ttpl->at("values").at("log_file") = path;
    }
}

void mk_task_template_set_string_option(
        mk_task_template_t *ttpl, const char *key, const char *value) noexcept {
    if (ttpl != nullptr && key != nullptr && value != nullptr) {
        ttpl->at("values").at("options")[key] = value;
    }
}

void mk_task_template_set_int_option(
        mk_task_template_t *ttpl, const char *key, int value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("options")[key] = value;
    }
}

void mk_task_template_set_double_option(
        mk_task_template_t *ttpl, const char *key, double value) noexcept {
    if (ttpl != nullptr && key != nullptr) {
        ttpl->at("values").at("options")[key] = value;
    }
}

void mk_task_template_set_output_file(
        mk_task_template_t *ttpl, const char *path) noexcept {
    if (ttpl != nullptr && path != nullptr) {
        ttpl->at("values").at("output_file") = path;
    }
}

void mk_task_template_enable_event(
        mk_task_template_t *ttpl, const char *type) noexcept {
    if (ttpl != nullptr && type != nullptr) {
        ttpl->at("values").at("enabled_events").push_back(type);
    }
}

void mk_task_template_enable_all_events(mk_task_template_t *ttpl) noexcept {
    mk_task_template_enable_event("ALL");
}

void mk_task_template_destroy(const mk_task_template_t *ttpl) noexcept {
    delete ttpl; // handles nullptr
}

struct mk_task_s {
	std::condition_variable cond;
	std::deque<std::unique_ptr<const mk_event_t>> deque;
	int flags = 0;
#define F_START (1 << 0)
#define F_RUN (1 << 1)
#define F_INTERRUPT (1 << 2)
	std::mutex mutex;
	mk::SharedPtr<mk::Reactor> reactor = mk::Reactor::make();
	mk_task_template ttpl;
};

// We implement the most tricky code parts here and defer the less tricky
// code parts to static functions available at the bottom of this file.
static void mk_task_main_unlocked(mk_tast_t *task) noexcept;

mk_task_t *mk_task_create(const mk_task_template_t *ttpl) noexcept {
	if (ttpl == nullptr) {
		return nullptr;
	}
	auto task = new mk_task_t{};
	task->ttpl = *ttpl; // makes a copy
	return ttpl;
}

void mk_task_start(mk_task_t *task) noexcept {
    if (task != nullptr) {
        std::unique_lock<std::mutex> _{task->mutex};
        if ((task->flags & F_START) != 0) {
            return; // Prevent racing on starting a task
        }
        task->flags |= F_START | F_RUN;
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
					// okay to call unlocked - must be ALL
                    task->cond.notify_all();
                });
    }
}

void mk_task_interrupt(mk_task_t *task) noexcept {
    if (task != nullptr) {
        std::unique_lock<std::mutex> _{task->mutex};
        if ((task->flags & (F_START | F_RUN)) != 0) {
			// TODO(bassosimone): ideally it would simplify the implementation
			// if we could (and we can) move this flag inside of the reactor.
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
	task->cond.notify_all(); // okay to call unlocked - must be ALL
}

void mk_task_destroy(mk_task_t *task) noexcept {
    if (task != nullptr) {
        {
            std::unique_lock<std::mutex> lock{task->mutex};
            if ((taks->flags & F_START) != 0) {
				// If we've been started, wait until we are stopped.
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
