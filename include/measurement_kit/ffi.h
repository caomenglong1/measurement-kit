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
 *
 * See include/measurement_kit/README.md for API documentation.
 */

#if defined(_WIN32) && defined(MK_BUILDING_DLL)
#define MK_PUBLIC __declspec(dllexport)
#elif defined(_WIN32) && defined(MK_USING_DLL)
#define MK_PUBLIC __declspec(dllimport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define MK_PUBLIC __attribute__((visibility("default")))
#else
#define MK_PUBLIC /* Nothing */
#endif

#define MK_BOOL int

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

MK_PUBLIC unsigned long mk_version_major(void) MK_NOEXCEPT;

MK_PUBLIC unsigned long mk_version_minor(void) MK_NOEXCEPT;

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

MK_PUBLIC const char *mk_event_get_serialized_list_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC MK_BOOL mk_event_has_object_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC const char *mk_event_get_serialized_object_entry(
        mk_event_t *event, const char *key) MK_NOEXCEPT;

MK_PUBLIC void mk_event_destroy(mk_event_t *event) MK_NOEXCEPT;

typedef struct mk_task_s mk_task_t;

MK_PUBLIC mk_task_t *mk_task_create(const char *type) MK_NOEXCEPT;

MK_PUBLIC const char *mk_task_get_type(mk_task_t *task) MK_NOEXCEPT;

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

#define MK_ENUM_VERBOSITY_LEVELS(XX)                                           \
    XX(QUIET)                                                                  \
    XX(WARNING)                                                                \
    XX(INFO)                                                                   \
    XX(DEBUG)                                                                  \
    XX(DEBUG2)

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

#define MK_ENUM_STRING_OPTIONS(XX)                                             \
    XX(bouncer_base_url)                                                       \
    XX(collector_base_url)                                                     \
    XX(dns_nameserver)                                                         \
    XX(geoip_ans_path)                                                         \
    XX(geoip_country_path)

#define MK_ENUM_INT_OPTIONS(XX)                                                \
    XX(ignore_open_report_error)                                               \
    XX(ignore_write_entry_error)                                               \
    XX(no_bouncer)                                                             \
    XX(no_collector)                                                           \
    XX(no_file_report)                                                         \
    XX(parallelism)

#define MK_ENUM_DOUBLE_OPTIONS(XX)                                             \
    XX(max_runtime)

#define MK_ENUM_FAILURES(XX)                                                   \
    XX(no_error)                                                               \
    XX(value_error)                                                            \
    XX(eof_error)                                                              \
    XX(connection_reset_error)

#endif /* MEASUREMENT_KIT_FFI_H */
