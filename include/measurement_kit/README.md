# Measurement Kit API

This directory contains Measurement Kit API. It is composed of the
following C/C++ header files (in alphabetic order):

- [cxx.hpp](cxx.hpp): C++14 wrapper around [swig.hpp](swig.hpp) designed
to be used directly by applications written in C++14;

- [ffi.h](ffi.h): ANSI C API for interacting with MK using [Foreign
Function Interface](https://en.wikipedia.org/wiki/Foreign_function_interface);

- [nettests.hpp](nettests.hpp): C++14 API exposed by old versions of MK, to
be reimplemented using [cxx.hhp](cxx.hpp), provided for backward compatibility
and guaranteed to be available until September, 2018;

- [swig.hpp](swig.hpp): C++14 wrapper around [ffi.h](ffi.h) suitable to
be passed to [SWIG](https://github.com/swig/swig) to automatically generate
wrappers in several programming languages;

- [version.h](version.h): header exposing MK version macros.

Of all these header files, the only file exporting functions is [ffi.h](ffi.h)
since all the others just define inline wrappers. This allows to hide that
MK is written in C++ and allows linking with pre-C++-11 codebases.

The rest of this document describes [ffi.h](ffi.h), which is the most
fundamental API exposed by MK. This document is organized such that we
can automatically extract [ffi.h](ffi.h) from this document using the
`awk` tool. We also have regression tests to make sure that this document
is consistent with the header committed on the repository. To generate
[ffi.h](ffi.h) from this document, run:

```
./script/gen/ffi_h.sh
```

## Introduction

[ffi.h](ffi.h) describes Measurement Kit as en engine that can run
specific _tasks_ (e.g. a network measurement, OONI orchestration) that
emit _events_ as part of their lifecycle (e.g. logs, results).

You will _configure_ tasks using specific option strings. You will
process events by accessing their type and event-specific variables ("keys")
that have specific names. Alternatively, you can configure a task
in a single function call using JSON and, likewise, you can get
a JSON serialization of an event.

The strings containing names for options, for event keys, etc. are
specified as part of this API.

We use semantic versioning. We will update our major and/or minor
version numbers whenever there are changes in [ffi.h](ffi.h) exported
functions and/or accompanying strings.

## Conventions

Whenever a function _may_ return `NULL`, it can return `NULL`. So
make sure you code with that use case in your mind.

Whenever a function can take a `NULL` argument, it will handle it
gracefully, typically returning `NULL` back.

## Index

The remainder of this file documents the `measurement_kit/ffi.h` API
header using a sort of literate programming. We will cover all the
symbols included into the API and explain how to use them.

```C
/* Part of Measurement Kit <https://measurement-kit.github.io/>.
   Measurement Kit is free software under the BSD license. See AUTHORS
   and LICENSE for more information on the copying conditions. */
#ifndef MEASUREMENT_KIT_FFI_H
#define MEASUREMENT_KIT_FFI_H

/*
 * Measurement Kit Foreign Function Interface (FFI) API.
 *
 * WARNING! Automatically generated from `include/measurement_kit/README.md`
 * using `./script/gen/ffi_h.sh`; DO NOT EDIT!
 */

```

The following symbols are discussed

- [MK_PUBLIC](#mk_public)
- [MK_BOOL](#mk_bool)
- [MK_NOEXCEPT](#mk_noexcept)
- [mk_version](#mk_version)
- [mk_event](#mk_event)
- [mk_task](#mk_task)
- [MK_ENUM_VERBOSITY_LEVELS](#mk_enum_verbosity_levels)
- [MK_ENUM_EVENTS](#mk_enum_events)
- [MK_ENUM_TASKS](#mk_enum_tasks)
- [MK_ENUM_STRING_OPTIONS](#mk_enum_string_options)
- [MK_ENUM_INT_OPTIONS](#mk_enum_int_options)
- [MK_ENUM_DOUBLE_OPTIONS](#mk_enum_double_options)
- [MK_ENUM_FAILURES](#mk_enum_failures)

## MK_PUBLIC

Because we want the API to be available from a Windows DLL, we need to
define `MK_PUBLIC`, a macro that allows us to export only specific symbols
to DLLs and Unix shared libraries.

```C
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

## MK_BOOL

We define a type called `MK_BOOL` to make it clear when we're using `int`
with boolean semantics.

```C++
#define MK_BOOL int

```

We will use `0` to indicate `false` and nonzero to indicate `true`.

## MK_NOEXCEPT

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

## mk_version

`mk_version_major` returns MK's major version number.

```C++
MK_PUBLIC unsigned long mk_version_major(void) MK_NOEXCEPT;

```

`mk_version_minor` returns MK's minor version number.

```C++
MK_PUBLIC unsigned long mk_version_minor(void) MK_NOEXCEPT;

```

These two functions are available to allow FFI bindings writers to
programmatically access MK version.

## mk_event

`mk_event_t` is an event emitted by a `mk_task_t`. It is an opaque type.

```C
typedef struct mk_event_s mk_event_t;
```

Internally, `mk_event_t` is a JSON object that maps string keys to JSON
objects of the following types:

- `null`
- `string`
- `int`
- `double`
- `object`
- `list`

You can get the `mk_event_t` type using `mk_event_get_type`.

```C
MK_PUBLIC const char *mk_event_get_type(mk_event_t *event) MK_NOEXCEPT;

```

Knowing the type, which is one of the strings documented at the bottom of
this specification, you know the JSON structure, which is also documented at
the bottom of this specification.

To get a serialization of the event as JSON, use the
`mk_event_as_serialized_json` function.

```C
MK_PUBLIC const char *mk_event_as_serialized_json(
        mk_event_t *event) MK_NOEXCEPT;

```

The returned JSON will be like:

```JSON
{
  "type": "event-type",
  "value": {
  }
}
```

where `type` is the event type string and `value` is an object structured
as explained at the bottom of this specification.

Alternatively, you can programmatically query whether specific keys of
specific types exists, and get their value. Accessing the value of `object`
and `list` keys will return you their JSON serialization. You can check
whether a key of the specific type exists. If you don't check and either
the key does not exist or is of the wrong type, the code will return
`0`, `0.0` or `nullptr`, depending on the return value type.

```C
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

MK_PUBLIC const char *mk_event_get_serialized_list_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_object_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC const char *mk_event_get_serialized_object_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

```

The expected keys for each type are also described at the bottom of
this specification.

In general, you can get away with the above programmatic API, because all
JSONs returned by Measurement Kit are flat, except for the `"result"` event,
whose structure is defined in the [TheTorProject/ooni-spec](
https://github.com/TheTorProject/ooni-spec) repository and is quited
nested, so that with the above API you can only get the top-level keys.

When you will get a `mk_event_t` pointer, you will always _own_ it, so,
destroy it when you are done using:

```C
MK_PUBLIC void mk_event_destroy(mk_event_t *event) MK_NOEXCEPT;

```

## mk_task

A `mk_task_t` is an operation that Measurement Kit can perform. Even though
you could start several tasks concurrently, _internally only a single
task will be run at a time_. Subsequently started tasks will have to wait.

Certain task operations are thread safe, others are not. Specifically, once
you have started a task, attempting to change its configuration is not supported
and `abort()` will be called if you attempt doing that. See below the
documentation of `mk_task_start()` for more information on this topic.

You can create a task by type name. Task names are defined below. Also,
once a `mk_task_t` is created, you can query its type name. If you create a
task with an unrecognized name, the task will fail when you start it.

```C
typedef struct mk_task_s mk_task_t;

MK_PUBLIC mk_task_t *mk_task_create(const char *type) MK_NOEXCEPT;

MK_PUBLIC char *mk_task_get_type(mk_task_t *task) MK_NOEXCEPT;

```

You can add annotations to a task. Such annotations will be added to the
task result (or "report") and may be useful to you, or to others, in
processing the results afterwards. Measurement Kit will let you add all
the annotations you want. The type of the annotation will be loosely
preserved in the final report (i.e. strings will still be strings and
numeric values will still be numeric values).

```C
MK_PUBLIC void mk_task_add_string_annotation(
        mk_task_t *task, const char *key, const char *value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_int_annotation(
        mk_task_t *task, const char *key, int value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_double_annotation(
        mk_task_t *task, const char *key, double value) MK_NOEXCEPT;

```

Some tasks, like OONI's Web Connectivity, accept one, or more, inputs, while
other tasks don't. You can always add input, either directly or specifing
a file to read from, to any task. If a task does not use any input, by
default such input will be ignored by the task (this behavior can though
be configured using specific options).

```C
MK_PUBLIC void mk_task_add_input(
        mk_task_t *task, const char *input) MK_NOEXCEPT;

MK_PUBLIC void mk_task_add_input_file(
        mk_task_t *task, const char *path) MK_NOEXCEPT;

```

Measurement Kit tasks emit logs. These logs are ignored unless you specify
a log file or you declare you are interested to get `"LOG"` events. In
such case, you can control what level of verbosity you want. By default,
only `"WARNING"` log events are emitted. However, you can control the level
of verbosity you want by passing the proper string (see below) to the
`mk_task_set_verbosity` function. If a verbosity level is not recognized, the
task will fail right after you attempt to start it.

```C
MK_PUBLIC void mk_task_set_verbosity(
        mk_task_t *task, const char *verbosity) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_log_file(
        mk_task_t *task, const char *path) MK_NOEXCEPT;

```

The behavior of tasks can be greatly influenced by setting options (see
below for all the available options). Options can either be strings,
integers, or doubles. If you pass to an option the wrong value, the task
may not fail immediately, rather it may fall later, when it discovers that
the option value is not acceptable.

```C
MK_PUBLIC void mk_task_set_string_option(
        mk_task_t *task, const char *key, const char *value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_int_option(
        mk_task_t *task, const char *key, int value) MK_NOEXCEPT;

MK_PUBLIC void mk_task_set_double_option(
        mk_task_t *task, const char *key, double value) MK_NOEXCEPT;

```

You can alternatively pass Measurement Kit a JSON that contains all
the options you would like to set, with `mk_task_set_options`. This function
will return false if the JSON does not parse, and true otherwise. The
options will be actually processed when the task starts, hence, as
stated above, you may notice _later_ that specific options have wrong
values.

```C
MK_PUBLIC MK_BOOL mk_task_set_options(
        mk_task_t *task, const char *serialized_json) MK_NOEXCEPT;

```

By default (but this can be changed with options), a task will submit
the results of measurements to the OONI collector. Optionally, you can
also specify that you want such result (also called the "report") to be
saved into a specific file. You can also get the report (or reports, if the
task iterates over multiple inputs) if you enable the `"RESULT"` event.

```C
MK_PUBLIC void mk_task_set_output_file(
        mk_task_t *task, const char *path) MK_NOEXCEPT;

```

As stated, when running a task will emit events. By default, only the
`"TERMINATE"` event &mdash; which signals that a task is done &mdash; is
enabled, but with `mk_task_enable_event` you can enable more events. All the
available events are described below. You can also `mk_task_enable_all_events`,
if you prefer.

```C
MK_PUBLIC void mk_task_enable_event(
        mk_task_t *task, const char *type) MK_NOEXCEPT;

MK_PUBLIC void mk_task_enable_all_events(mk_task_t *task) MK_NOEXCEPT;

```

When a task is configured, you can start it using `mk_task_start`. This will
queue the task until the internal runner thread is ready to service it. Do not
attempt to further configure the task using any of the above functions once
a task has been started. Doing that will cause `abort()` to be called.

```C
MK_PUBLIC void mk_task_start(mk_task_t *task) MK_NOEXCEPT;

```

The `mk_task_is_running` function returns `true` since a task `start` method
has been called until the task has finished running.

```C
MK_PUBLIC MK_BOOL mk_task_is_running(mk_task_t *task) MK_NOEXCEPT;

```

You can interrupt a task at any time using the `mk_task_interrupt` function.
This will do what you expect: if the task was queued, cause it to fail
when it is run; interrupt the task if it is running; do nothing is the task
has already terminated. In the event in which the task is using blocking I/O,
Measurement Kit will notify the blocking I/O engine that the task should
be aborted and that should result in the task being aborted after one-two
seconds of delay, which are caused by internal timeout settings.

```C++
MK_PUBLIC void mk_task_interrupt(mk_task_t *task) MK_NOEXCEPT;

```

Finally, while a task is running, you can receive its events by
calling `mk_task_wait_for_next_event()`. This is a blocking call that will
return only when the next enabled event occurs. If you do not enable any
event, you will still receive the `"TERMINATE"` event, since this event is
always enabled. Calling this function before you start a task or after a task
is terminated will return `NULL`. Note that _you own_ the event returned by
`mk_task_wait_for_next_event()` and it is your responsibility to call
`mk_event_destroy()` when done with it.

```C
MK_PUBLIC mk_event_t *mk_task_wait_for_next_event(mk_task_t *task) MK_NOEXCEPT;

```

Speaking of ownership, you also own the task, and you should dispose of it
when done using `mk_task_destroy`:

```C
MK_PUBLIC void mk_task_destroy(mk_task_t *task) MK_NOEXCEPT;
```

Calling `mk_task_destroy` while a task is running will interrupt the task and
wait for the task thread to join before freeing resources and returning.

## MK_ENUM_VERBOSITY_LEVELS

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

## MK_ENUM_EVENTS

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

## MK_ENUM_TASKS

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

## MK_ENUM_STRING_OPTIONS

Here we describe the available string options.

```C++
#define MK_ENUM_STRING_OPTIONS(XX)                                             \
    XX(bouncer_base_url)                                                       \
    XX(collector_base_url)                                                     \
    XX(dns_nameserver)                                                         \
    XX(geoip_ans_path)                                                         \
    XX(geoip_country_path)

```

## MK_ENUM_INT_OPTIONS

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

## MK_ENUM_DOUBLE_OPTIONS

Here we describe the available double options.

```C++
#define MK_ENUM_DOUBLE_OPTIONS(XX)                                             \
    XX(max_runtime)

```

## MK_ENUM_FAILURES

Here we list all the possible failure strings.

```C++
#define MK_ENUM_FAILURES(XX)                                                   \
    XX(no_error)                                                               \
    XX(value_error)                                                            \
    XX(eof_error)                                                              \
    XX(connection_reset_error)

```

This concludes our description of [ffi](ffi.h).

```C++
#endif /* MEASUREMENT_KIT_MK_H */
```
