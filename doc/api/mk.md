# Measurement Kit API

This document specifies Measurement Kit API. It is organized such that we
can easily extract the API header from this document using `awk`, using the
following one-line, to guarantee that docs and API are consistent.

```
awk '/^```C\+\+$/{ emit=1; next }/^```$/{ emit=0 } emit'
```

## Introduction

There are three Measurement Kit APIs that you can use:

1. the C++11 API in the `mk::engine` namespace, designed to be
consumed by [SWIG](https://github.com/swig/swig) to automatically
produce bindings in other languages;

2. the C++11 API in the `mk::cxx11` namespace, designed to be
consumed by clients written in C++11 and above;

3. the ANSI C API prefixed by `mk_`, designed to be consumed
by languages using the [Foreign Function Interface](
https://en.wikipedia.org/wiki/Foreign_function_interface) aka
FFI API.

Pick the most appropriate API for your use case.

All these APIs describe Measurement Kit as en engine that can run
specific _tasks_ (e.g. a network measurement, OONI orchestration) that
emit _events_ as part of their lifecycle (e.g. logs, results).

You will configure tasks using specific option strings. You will
process events by accessing their type and event-specific variables ("keys")
that have specific names. Alternatively, you can configure a task
using a single JSON and process events as a JSON.

The strings containing names for options, for event keys, etc. are
specified as part of this API.

We use semantic versioning. We will update `MK_API_MAJOR` and/or `MK_API_MINOR`
accordingly whenever there are changes _either_ in any of the three
above APIs _or_ in the accompanying strings.

## Conventions

Whenever a function _may_ return `nullptr`, it can return `nullptr`. So
make sure you code with that use case in your mind.

The `mk::engine` and `mk_` APIs do not `throw`. Whenver a fatal error
occurs (e.g. cannot create a thread), they call `std::abort`.

The `mk::cxx11` API throws exceptions derived from `std::exception`.

## Index

The remainder of this file documents the `measurement_kit/mk.h` API
header using a sort of literate programming. We will cover all the
symbols included into the API and explain how to use them. The file
can roughly be divided into the following sections:

- [Prelude](#prelude)
- [Generally used macros](#generally-used-macros)
    - [MK_PUBLIC](#mk_public)
    - [MK_API_MAJOR](#mk_get_api_major-macro)
    - [MK_API_MINOR](#mk_get_api_minor-macro)
- [namespace mk](#namespace-mk)
    - [namespace mk::engine](#namespace-mkengine)
        - [class engine::Event](#class-engineevent)
        - [class engine::Task](#class-enginetask)
    - [namespace mk::cxx11](#namespace-mkcxx11)
        - [class cxx11::Event](#class-cxx11event)
        - [class cxx11::Task](#class-cxx11task)
- [ANSI C API](#ansi-c-api)
    - [MK_NOEXCEPT](#mk_noexcept)
    - [MK_BOOL](#mk_bool)
    - [MK_FALSE](#mk_false)
    - [MK_TRUE](#mk_true)
    - [mk_get_api_major](#mk_get_api_major)
    - [mk_get_api_minor](#mk_get_api_minor)
    - [mk_event type](#mk_event-type)
    - [mk_task type](#mk_task-type)
- [Strings](#strings)
    - [MK_ENUM_VERBOSITY_LEVELS](#mk_enum_verbosity_levels)
    - [MK_ENUM_EVENTS](#mk_enum_events)
    - [MK_ENUM_TASKS](#mk_enum_tasks)
    - [MK_ENUM_STRING_OPTIONS](#mk_enum_string_options)
    - [MK_ENUM_INT_OPTIONS](#mk_enum_int_options)
    - [MK_ENUM_DOUBLE_OPTIONS](#mk_enum_double_options)
    - [MK_ENUM_FAILURES](#mk_enum_failures)
- [Conclusion](#conclusion)

## Prelude

```C++
/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_MK_H
#define MEASUREMENT_KIT_MK_H

/*
 * Measurement Kit API.
 *
 * WARNING! Automatically generated file; DO NOT EDIT!
 *
 * Automatically generated from documentation: [doc/api/mk.md](
 *    https://github.com/measurement-kit/measurement-kit/blob/master/doc/api/mk.md)
 */

```

## Generally used macros

### MK_PUBLIC

Because we want the API to be available from a Windows DLL, we need to
define `MK_PUBLIC`, a macro that allows us to export only specific symbols
to DLLs and Unix shared libraries.

```C++
#if defined(_WIN32) && defined(MK_BUILDING_DLL)
#define MK_PUBLIC __declspec(dllexport)
#elif defined(_WIN32) && defined(MK_USING_DLL)
#define MK_PUBLIC __declspec(dllimport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define MK_PUBLIC __attribute__((visibility("default")))
#else
#define MK_PUBLIC /* Nothing */
#endif

```

### MK_API_MAJOR

`MK_API_MAJOR` contains Measurement Kit the major API version number.

```C++
#define MK_API_MAJOR 1

```

### MK_API_MINOR

`MK_API_MINOR` contains Measurement Kit the minor API version number.

```C++
#define MK_API_MINOR 0

```

## namespace mk

C++11 code lives in the `mk` namespace. This is only visible to C++11
compilers, SWIG, and modern-enough versions of Visual Studio.

```C++
#if ((defined __cplusplus && __cplusplus >= 201103L) ||                        \
     (defined SWIG) ||                                                         \
     (defined _MSC_VER && _MSC_VER >= 1600))

```

We need to include C++11 headers we use.

```C++
#include <memory>    // for std::unique_ptr

#ifndef SWIG
#include <stdexcept> // for std::runtime_error
#include <string>    // for std::string
#endif // SWIG

```

All the C++11 code is in `namespace mk`.

```C++
namespace mk {
```

### namespace mk::engine

The code designed to be consumed by SWIG is in the `mk::engine` namespace.

```C++
namespace engine {

```

#### class engine::Event

`Event` is an event emitted by running a `Task`. Internally, `Event` is a JSON
object that maps string keys to JSON objects of the following types:

- `null`
- `string`
- `int`
- `double`
- `object`
- `list`

You can get the `Event` type using `get_type`. Knowing the type, you know the
JSON structure as documented at the bottom of this specification.

```C++
class Event {
  public:
    MK_PUBLIC const char *get_type() const noexcept;

```

To get a serialization of the whole JSON, just use the
`as_serialized_json` method.

```C++
    MK_PUBLIC const char *as_serialized_json() const noexcept;

```

Alternatively, you can programmatically query whether specific keys of
specific types exists, and get their value. Accessing the value of `object`
and `list` keys will return you their JSON serialization. You can check
whether a key of the specific type exists. If you don't check and either
the key does not exist or is of the wrong type, the code will return
`0`, `0.0` or `nullptr`, depending on the return value type.

```C++
    MK_PUBLIC bool has_null_entry(const char *key) const noexcept;

    MK_PUBLIC bool has_string_entry(const char *key) const noexcept;

    MK_PUBLIC const char *get_string_entry(const char *key) const noexcept;

    MK_PUBLIC bool has_int_entry(const char *key) const noexcept;

    MK_PUBLIC int get_int_entry(const char *key) const noexcept;

    MK_PUBLIC bool has_double_entry(const char *key) const noexcept;

    MK_PUBLIC double get_double_entry(const char *key) const noexcept;

    MK_PUBLIC bool has_object_entry(const char *key) const noexcept;

    MK_PUBLIC const char *get_serialized_object_entry(
            const char *key) const noexcept;

    MK_PUBLIC bool has_list_entry(const char *key) const noexcept;

    MK_PUBLIC const char *get_serialized_list_entry(
            const char *key) const noexcept;

```

In general, you can get away with the above programmatic API, because all
JSONs returned by Measurement Kit are flat, except for the `"result"` event,
whose structure is defined in the [TheTorProject/ooni-spec](
https://github.com/TheTorProject/ooni-spec) repository and is quited
nested, so that with the above API you can only get the top-level keys.

This ends our description of the `Event` type. We just need to define some
extra fiels for technical reasons unrelated with this API.

```C++
    MK_PUBLIC ~Event() noexcept;

  private:
    Event();

    class Unwrapper;
    friend class Unwrapper;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

```

#### class engine::Task

A `Task` is an operation that Measurement Kit can perform. Even though
you could `start` several `Task`s concurrently, _internally only a single
task will be run at a time_. Subsequently `start`ed `Tasks` will be put
into a queue and will wait to become active.

The `Task` class is thread safe.

Once a `Task` has been started, attempting to change its configuration by
setting an extra log file, adding options, etc. is a programming error that
will lead to `std::abort()` being called by Measurement Kit.

You can create a `Task` by type name. Task names are defined below. Also,
once a `Task` is created, you can query its type name. If you create a
`Task` with an unrecognized name, the `Task` will fail when you `start` it,
and the `"end"` event will be emitted with a suitable error.

```C++
class Task {
  public:
    MK_PUBLIC explicit Task(const char *type) noexcept;

    MK_PUBLIC const char *get_type() const noexcept;

```

You can add annotations to a task. Such annotations will be added to the
`Task` result (or "report") and may be useful to you, or to others, in
processing the results afterwards. Measurement Kit will let you add all
the annotations you want. The type of the annotation will be loosely
preserved in the final report (i.e. strings will still be strings and
numeric values will still be numeric values).

```C++
    MK_PUBLIC void add_string_annotation(
            const char *name, const char *value) noexcept;

    MK_PUBLIC void add_int_annotation(
            const char *name, int value) noexcept;

    MK_PUBLIC void add_double_annotation(
            const char *name, double value) noexcept;

```

Some `Task`s, like OONI's Web Connectivity, accept one or more inputs, while
other tasks do not. You can always add input, either directly or specifing
a file to read from, to any task. If the `Task` does not use any input, by
default such input will be ignored by the `Task` (this behavior can though
be configured using specific options).

```C++
    MK_PUBLIC void add_input(const char *input) noexcept;

    MK_PUBLIC void add_input_file(const char *path) noexcept;

```

Measurement Kit tasks emit logs. These logs are ignored unless you specify
a log file or you declare you are interested to get `"log"` events. In
such case, you can control what level of verbosity you want. By default,
only `"warning"` messages are emitted. However, you can control the level
of verbosity you want by passing the proper string (see below) to the
`set_verbosity` method. If a verbosity level is not recognized, the `Task`
will fail right after you attempt to `Start` it. To avoid confusion,
verbosity levels are always in lower case.

```C++
    MK_PUBLIC void set_verbosity(const char *verbosity) noexcept;

    MK_PUBLIC void set_log_file(const char *path) noexcept;

```

The behavior of `Task`s can be greatly influenced by setting options (see
below for all the available options). Options can either be strings,
integers, or doubles. If you pass to an option the wrong value, the `Task`
may not fail immediately, rather it may fall later, when it discovers that
the option value is not acceptable. To avoid confusion, option names are
always in lower case.

```C++
    MK_PUBLIC void set_string_option(
            const char *name, const char *value) noexcept;

    MK_PUBLIC void set_int_option(
            const char *name, int value) noexcept;

    MK_PUBLIC void set_double_option(
            const char *name, double value) noexcept;

```

You can alternatively pass Measurement Kit a JSON that contains all
the options you would like to set, with `set_options`. This method
will return false if the JSON does not parse, and true otherwise. The
options will be actually processed when the `Task` starts, hence, as
stated above, you may notice _later_ that specific options have wrong
values.

```C++
    MK_PUBLIC bool set_options(const char *serialized_json) noexcept;

```

By default (but this can be changed with options), a `Task` will submit
the results of measurements to the OONI collector. Optionally, you can
also specify that you want such result (also called the "report") to be
saved into a specific file. You can also get the report (or reports, if the
task iterates over multiple inputs) if you enable the `"result"` event.

```C++
    MK_PUBLIC void set_output_file(const char *path) noexcept;

```

As stated, when running a `Task` will emit `Events`. By default, only the
`"end"` event &mdash; which signals that a `Task` is done &mdash; is enabled,
but with `enable_event` you can enable more events. All the available events
are described below. Event names are always in lower case. You can also
`enable_all_events`, if you prefer.

```C++
    MK_PUBLIC void enable_event(const char *type) noexcept;

    MK_PUBLIC void enable_all_events() noexcept;

```

When a `Task` is configured, you can start it using `start`. This will queue
the `Task` until the internal runner thread is ready to service it. Do not
attempt to further modify the events using any of the above methods once
a `Task` has been `start`ed. Doing that will cause `std::abort()` to be called.

```C++
    MK_PUBLIC void start() noexcept;

```

The `is_running` method returns `true` since a task `start` method has been
called until the task has finished running.

```C++
    MK_PUBLIC bool is_running() const noexcept;

```

You can interrupt a `Task` at any time using the `interrupt` method. This will
do what you expect: if the `Task` was queued, cause it to fail when it is
run; interrupt the `Task` if it is running; do nothing is the `Task` has
already terminated. In the event in which the `Task` is using blocking I/O,
Measurement Kit will notify the blocking I/O engine that the task should
be aborted and that should result in the `Task` being aborted after one-two
seconds of delay, which are caused by internal timeout settings.

```C++
    MK_PUBLIC void interrupt() noexcept;

```

Finally, while a `Task` is running, you can receive its events by
calling `wait_for_next_event()`. This is a blocking call that will return
only when the next enabled `Event` occurs. If you do not enable any
event, you will still receive the `"end"` `Event` with this method, since
the `"end"` `Event` is always enabled. Calling this method before you
`start()` a `Task` or after a `Task` is complete will return `nullptr`.
Note that _you own_ the `Event *` returned by `wait_for_next_event()` and
it is your responsibility to `delete` it when done.

```C++
    MK_PUBLIC Event *wait_for_next_event() noexcept;

```

This ends our discussion of `engine::Task`. All what remains are internal
fields required to actually implement a `Task` properly.

```C++
  private:
    class Unwrapper;
    friend class Unwrapper;

    class Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace engine

```

### namespace mk::cxx11

The code designed to be used by C++11 and above clients is in the
`mk::cx11` namespace. This code is actually an inline implementation
that adds just a tiny wrapper around `mk::engine` to make the C++11
experience more pleasant and robust. Specifically, we will thrown
`std::runtime_error`s when we detect a `nullptr` pointer.

```C++
#ifndef SWIG
namespace cxx11 {

template <typename T> T *panic_if_nullptr(T *t) {
    if (t == nullptr) {
        throw std::runtime_error("null_pointer");
    }
    return t;
}

```

#### class cx11::Event

```C++
class Event {
  public:
    Event(engine::Event *p) : evp_{panic_if_nullptr(p)} {}

    std::string get_type() const {
        return std::string{panic_if_nullptr(evp_->get_type())};
    }

    std::string as_serialized_json() const {
        return std::string{panic_if_nullptr(evp_->as_serialized_json())};
    }

    bool has_null_entry(const std::string &key) const noexcept {
        return evp_->has_null_entry(key.data());
    }

    bool has_string_entry(const std::string &key) const noexcept {
        return evp_->has_string_entry(key.data());
    }

    std::string get_string_entry(const std::string &key) const {
        return std::string{
                panic_if_nullptr(evp_->get_string_entry(key.data()))};
    }

    bool has_int_entry(const std::string &key) const noexcept {
        return evp_->has_int_entry(key.data());
    }

    int get_int_entry(const std::string &key) const noexcept {
        return evp_->get_int_entry(key.data());
    }

    bool has_double_entry(const std::string &key) const noexcept {
        return evp_->has_double_entry(key.data());
    }

    double get_double_entry(const std::string &key) const noexcept {
        return evp_->get_double_entry(key.data());
    }

    bool has_object_entry(const std::string &key) const noexcept {
        return evp_->has_object_entry(key.data());
    }

    std::string get_serialized_object_entry(const std::string &key) const {
        return std::string{panic_if_nullptr(
                evp_->get_serialized_object_entry(key.data()))};
    }

    bool has_list_entry(const std::string &key) const noexcept {
        return evp_->has_list_entry(key.data());
    }

    std::string get_serialized_list_entry(const std::string &key) const {
        return std::string{panic_if_nullptr(
                evp_->get_serialized_list_entry(key.data()))};
    }

  private:
    std::unique_ptr<engine::Event> evp_;
};

```

#### class cx11::Task

```C++
class Task {
  public:
    explicit Task(const std::string &type) noexcept : task_{type.data()} {}

    std::string get_type() const {
        return std::string{panic_if_nullptr(task_.get_type())};
    }

    void add_string_annotation(
            const std::string &name, const std::string &value) noexcept {
        task_.add_string_annotation(name.data(), value.data());
    }

    void add_int_annotation(const std::string &name, int value) noexcept {
        task_.add_int_annotation(name.data(), value);
    }

    void add_double_annotation(const std::string &name, double value) noexcept {
        task_.add_double_annotation(name.data(), value);
    }

    void add_input(const std::string &input) noexcept {
        task_.add_input(input.data());
    }

    void add_input_file(const std::string &path) noexcept {
        task_.add_input_file(path.data());
    }

    void set_verbosity(const std::string &verbosity) noexcept {
        task_.set_verbosity(verbosity.data());
    }

    void set_log_file(const std::string &path) noexcept {
        task_.set_log_file(path.data());
    }

    void set_string_option(
            const std::string &name, const std::string &value) noexcept {
        task_.set_string_option(name.data(), value.data());
    }

    void set_int_option(const std::string &name, int value) noexcept {
        task_.set_int_option(name.data(), value);
    }

    void set_double_option(const std::string &name, double value) noexcept {
        task_.set_double_option(name.data(), value);
    }

    bool set_options(const std::string &serialized_json) noexcept {
        return task_.set_options(serialized_json.data());
    }

    void set_options_or_throw(const std::string &serialized_json) {
        if (!set_options(serialized_json)) {
            throw std::runtime_error("invalid_json");
        }
    }

    void set_output_file(const std::string &path) noexcept {
        task_.set_output_file(path.data());
    }

    void enable_event(const std::string &type) noexcept {
        task_.enable_event(type.data());
    }

    void enable_all_events() noexcept { task_.enable_all_events(); }

    void start() noexcept { task_.start(); }

    bool is_running() const noexcept { return task_.is_running(); }

    void interrupt() noexcept { task_.interrupt(); }

    Event wait_for_next_event() {
        return Event{panic_if_nullptr(task_.wait_for_next_event())};
    }

  private:
    engine::Task task_;
};

} // namespace cxx11
#endif // SWIG 
} // namespace mk
#endif /* C++11 code */

```

## ANSI C API

All the remainder of the API is written in ANSI C. SWIG should not see it,
because that will only make the bindings unnecessarily larger.

```C++
#ifndef SWIG

```

### MK_NOEXCEPT

We need to tell a C++ compiler processing this file that this is a C API
and that functions in this API do not `throw`.

```C++
#if defined(__cplusplus) && __cplusplus >= 201103L
#define MK_NOEXCEPT noexcept
#elif defined(__cplusplus)
#define MK_NOEXCEPT throw()
#else
#define MK_NOEXCEPT /* Nothing */
#endif

#ifdef __cplusplus
extern "C" {
#endif

```

### MK_BOOL

We define a type called `MK_BOOL` to make it clear when we're using `int`
as a real `int` and when we're using it with boolean semantics.

```C++
#define MK_BOOL int

```

### MK_FALSE

```C++
#define MK_FALSE 0

```

### MK_TRUE

```C++
#define MK_TRUE 1

```

### mk_get_api_major

`mk_get_api_major` return the `MK_API_MAJOR` version.

```C++
MK_PUBLIC int mk_get_api_major(void) MK_NOEXCEPT;

```

### mk_get_api_minor

`mk_get_api_minor` return the `MK_API_MINOR` version.

```C++
MK_PUBLIC int mk_get_api_minor(void) MK_NOEXCEPT;

```

### mk_event type

We start by wrapping the `Event` type.

```C++
typedef struct mk_event_s mk_event_t;

MK_PUBLIC const char *mk_event_get_type(mk_event_t *event) MK_NOEXCEPT;

MK_PUBLIC const char *mk_event_as_serialized_json(
        mk_event_t *event) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_null_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_string_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC const char *mk_event_get_string_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_int_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC int mk_event_get_int_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_double_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC double mk_event_get_double_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_list_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC const char *mk_event_get_list_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_object_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC const char *mk_event_get_object_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC void mk_event_destroy(mk_event_t *event) MK_NOEXCEPT;

```

### mk_task type

Then we wrap the `Task` type.

```C++
typedef struct mk_task_s mk_task_t;

MK_PUBLIC mk_task_t *mk_task_create(const char *type) MK_NOEXCEPT;

MK_PUBLIC char *mk_task_get_type(mk_task_t *task) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_string_annotation(
        mk_task_t *task, const char *key, const char *value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_int_annotation(
        mk_task_t *task, const char *key, int value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_double_annotation(
        mk_task_t *task, const char *key, double value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_input(
        mk_task_t *task, const char *input) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_input_file(
        mk_task_t *task, const char *path) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_verbosity(
        mk_task_t *task, const char *verbosity) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_log_file(
        mk_task_t *task, const char *path) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_string_option(
        mk_task_t *task, const char *key, const char *value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_int_option(
        mk_task_t *task, const char *key, int value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_double_option(
        mk_task_t *task, const char *key, double value) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_task_set_options(
        mk_task_t *task, const char *serialized_json) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_output_file(
        mk_task_t *task, const char *path) MK_NOEXCEPT;

MK_PUBLIC void mk_task_enable_event(
        mk_task_t *task, const char *type) MK_NOEXCEPT;

MK_PUBLIC void mk_task_enable_all_events(mk_task_t *task) MK_NOEXCEPT;

MK_PUBLIC void mk_task_start(mk_task_t *task) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_task_is_running(mk_task_t *task) MK_NOEXCEPT;

MK_PUBLIC void mk_task_interrupt(mk_task_t *task) MK_NOEXCEPT;

MK_PUBLIC mk_event_t *mk_task_wait_for_next_event(mk_task_t *task) MK_NOEXCEPT;

MK_PUBLIC void mk_task_destroy(mk_task_t *task) MK_NOEXCEPT;

#ifdef __cplusplus
} // __cplusplus
#endif
#endif /* !SWIG */

```

## Strings

### MK_ENUM_VERBOSITY_LEVELS

The possible verbosity level strings are:

```C++
#define MK_ENUM_VERBOSITY_LEVELS(XX)                                           \
    XX(quiet)                                                                  \
    XX(warning)                                                                \
    XX(info)                                                                   \
    XX(debug)                                                                  \
    XX(debug2)

```

- `"quiet"`: does not emit any log messages.

- `"warning"`: only emits log messages

- `"info"`: also emits info messages

- `"debug"`: also emits debug messages

- `"debug2"`: also emits debug2 messages

### MK_ENUM_EVENTS

The possible event types are listed below. We also have indicated
the number of times an event should be emitted. Of course, since
all events but the "end" event are not enabled, they will not be
emitted unless they are enabled.

```C++
#define MK_ENUM_EVENT_TYPES(XX)                                                \
    XX(queued)                                                                 \
    XX(started)                                                                \
    XX(log)                                                                    \
    XX(configured)                                                             \
    XX(progress)                                                               \
    XX(performance)                                                            \
    XX(measurement_error)                                                      \
    XX(report_submission_error)                                                \
    XX(result)                                                                 \
    XX(end)

```

- `"queued"`: emitted once when the task has been queued. The corresponding
   JSON is empty.

- `"started"`: emitted once when the task has been started. The corresponding
   JSON is empty.

- `"log"`: emitted whenever the task has generated a log line. The
   corresponding JSON is like:

```JSON
{
  "verbosity": "warning",
  "message": "the actual log line"
}
```

- `"configured"`: emitted once, for tasks that need configuration (i.e. to
  discover what test helper and collector servers to use), when this
  information becomes available during the task lifecycle. The corresponding
  JSON is like:

```JSON
{
  "probe_asn": "AS0",
  "probe_cc": "ZZ",
  "probe_ip": "10.0.0.1",
  "report_id": "report-id-string"
}

```

- `"progress"`: emitted whenever the task has made some progress. The
  corresponding JSON is like:

```JSON
{
  "percentage": 0.7,
  "eta": 17,
  "message": "what I am doing know, human readable",
}
```

- `"performance"`: emitted by test that measure speed, while they are measuring
  the download or upload speed, to allow apps to programmatically update
  their UI with the currently measured speed. The JSON is like:

```JSON
{
  "direction": "download",
  "elapsed_seconds": 11.1117,
  "num_streams": 1,
  "speed_kbit_s": 17.117
}
```

- `"measurement_error"`: emitted when a measurement fail, so that the app
   can perhaps update its UI accordingly. The JSON is described below. Note
   that, especially for censorship events, there can possibly be several
   events like this emitted. While it is true that you can parse the
   `"result"` event and get the failure from there, this event is meant
   to simplify the life of app developers by being able to easily
   update their UI without looking into the whole `"result"`.

```JSON
{
  "input": "current-input",
  "failure": "a failure string, see below"
}
```

- `"report_submission_error"`: emitted when a report cannot be submitted, so
  that perhaps the app can manage to submit the report later. The JSON
  of this event is described below. The full report is included into the
  event, such that one can set it aside and use another task later to
  try to resubmit the specific measurement that failed to submit.

```JSON
{
  "failure": "a failure string, see below",
  "report": {
  }
}
```

- `"result"`: emitted whenever the task generates a result (hence possibly
  emitted more than once for tasks that cycle over an input list). The
  corresponding JSON is the result JSON emitted by the task, according to
  the specification at github.com/TheTorProject/ooni-spec. Since the JSON
  structure of this event is quite complex, it is unlikely that one can
  parse it successfully using the programmatical API and usage of a JSON
  parser to deserialize and process the event is recommended.

- `"end"`: emitted just once when the task terminates. The corresponding
  JSON is described below. Note that here the failure indicates an hard
  failure that prevented the whole task to be run, as opposed to a finer
  grained failure that can cause a measurement to fail.

```JSON
{
  "downloaded_kbites": 10.5,
  "uploaded_kbites": 5.6,
  "failure": "a failure string, see below"
}
```

Note that more event types can be emitted and, especially while we are
converging towards v1.0 of the API, _will_ be emitted. You are advised to
change your code, though, because these legacy event types will most
likely be removed in future versions of Measurement Kit.

### MK_ENUM_TASKS

The possible task types are listed below.

```C++
#define MK_ENUM_TASK_TYPES(XX)                                                 \
    XX(dash)                                                                   \
    XX(captive_portal)                                                         \
    XX(dns_injection)                                                          \
    XX(facebook_messenger)                                                     \
    XX(http_header_field_manipulation)                                         \
    XX(http_invalid_request_line)                                              \
    XX(meek_fronted_requests)                                                  \
    XX(multi_ndt)                                                              \
    XX(ndt)                                                                    \
    XX(tcp_connect)                                                            \
    XX(telegram)                                                               \
    XX(web_connectivity)                                                       \
    XX(whatsapp)                                                               \
                                                                               \
    XX(opos_register)                                                          \
    XX(opos_update)                                                            \
    XX(opos_list_tasks)                                                        \
    XX(opos_get_task)                                                          \
    XX(opos_accept_task)                                                       \
    XX(opos_reject_task)                                                       \
    XX(opos_task_done)                                                         \
                                                                               \
    XX(find_probe_location)

```

TODO: add the description of the above strings (should be obvious).

### MK_ENUM_STRING_OPTIONS

Here we describe the available string options.

```C++
#define MK_ENUM_STRING_OPTIONS(XX)                                             \
    XX(bouncer_base_url)                                                       \
    XX(collector_base_url)                                                     \
    XX(dns_nameserver)                                                         \
    XX(geoip_ans_path)                                                         \
    XX(geoip_country_path)

```

### MK_ENUM_INT_OPTIONS

Here we describe the available int options.

```C++
#define MK_ENUM_INT_OPTIONS(XX)                                                \
    XX(ignore_open_report_error)                                               \
    XX(ignore_write_entry_error)                                               \
    XX(no_bouncer)                                                             \
    XX(no_collector)                                                           \
    XX(no_file_report)                                                         \
    XX(parallelism)

```

### MK_ENUM_DOUBLE_OPTIONS

Here we describe the available double options.

```C++
#define MK_ENUM_DOUBLE_OPTIONS(XX)                                             \
    XX(max_runtime)

```

### MK_ENUM_FAILURES

Here we list all the possible failure strings.

```C++
#define MK_ENUM_FAILURES(XX)                                                   \
    XX(no_error)                                                               \
    XX(value_error)                                                            \
    XX(eof_error)                                                              \
    XX(connection_reset_error)

```

## Conclusion

```C++
#endif /* MEASUREMENT_KIT_MK_H */
```
