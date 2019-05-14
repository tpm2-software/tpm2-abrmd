/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include "util.h"
#include "logging.h"

static const char *env_str_all = "all";
static const char *env_str_foo = "foo";

char* __real_getenv (const char *name);
char*
__wrap_getenv (const char *name)
{
    if (strcmp (name, "G_MESSAGES_DEBUG")) {
        return __real_getenv (name);
    }
    return mock_type (char*);
}

static void
logging_get_enabled_log_levels_default_test (void **state)
{
    int levels;
    UNUSED_PARAM(state);

    will_return (__wrap_getenv, NULL);
    levels = get_enabled_log_levels ();
    assert_int_equal (levels, LOG_LEVEL_DEFAULT);
}

static void
logging_get_enabled_log_levels_all_test (void **state)
{
    int levels;
    UNUSED_PARAM(state);

    will_return (__wrap_getenv, env_str_all);
    levels = get_enabled_log_levels ();
    assert_int_equal (levels, LOG_LEVEL_ALL);
}

static void
logging_get_enabled_log_levels_foo_test (void **state)
{
    int levels;
    UNUSED_PARAM(state);

    will_return (__wrap_getenv, env_str_foo);
    levels = get_enabled_log_levels ();
    assert_int_equal (levels, LOG_LEVEL_DEFAULT);
}

static void
logging_set_logger_foo_test (void **state)
{
    UNUSED_PARAM(state);
    assert_int_equal (set_logger ("foo"), -1);
}

static void
logging_set_logger_stdout_test (void **state)
{
    UNUSED_PARAM(state);
    assert_int_equal (set_logger ("stdout"), 0);
}

static void
logging_set_logger_syslog_test (void **state)
{
    UNUSED_PARAM(state);
    will_return (__wrap_getenv, NULL);
    assert_int_equal (set_logger ("syslog"), 0);
}

void
__wrap_syslog (int priority,
               const char *format,
               ...)
{
    UNUSED_PARAM(priority);
    UNUSED_PARAM(format);
    return;
}

static void
logging_syslog_log_handler_fatal_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_FLAG_FATAL,
                        "foo",
                        NULL);
}

static void
logging_syslog_log_handler_error_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_LEVEL_ERROR,
                        "foo",
                        NULL);
}

static void
logging_syslog_log_handler_critical_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_LEVEL_CRITICAL,
                        "foo",
                        NULL);
}

static void
logging_syslog_log_handler_warning_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_LEVEL_WARNING,
                        "foo",
                        NULL);
}

static void
logging_syslog_log_handler_message_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_LEVEL_MESSAGE,
                        "foo",
                        NULL);
}

static void
logging_syslog_log_handler_info_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_LEVEL_INFO,
                        "foo",
                        NULL);
}

static void
logging_syslog_log_handler_debug_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_LEVEL_DEBUG,
                        "foo",
                        NULL);
}

static void
logging_syslog_log_handler_default_test (void **state)
{
    UNUSED_PARAM(state);
    syslog_log_handler ("domain",
                        G_LOG_LEVEL_DEBUG|G_LOG_LEVEL_MESSAGE,
                        "foo",
                        NULL);
}
int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (logging_get_enabled_log_levels_default_test),
        cmocka_unit_test (logging_get_enabled_log_levels_all_test),
        cmocka_unit_test (logging_get_enabled_log_levels_foo_test),
        cmocka_unit_test (logging_set_logger_foo_test),
        cmocka_unit_test (logging_set_logger_stdout_test),
        cmocka_unit_test (logging_set_logger_syslog_test),
        cmocka_unit_test (logging_syslog_log_handler_fatal_test),
        cmocka_unit_test (logging_syslog_log_handler_error_test),
        cmocka_unit_test (logging_syslog_log_handler_critical_test),
        cmocka_unit_test (logging_syslog_log_handler_warning_test),
        cmocka_unit_test (logging_syslog_log_handler_message_test),
        cmocka_unit_test (logging_syslog_log_handler_info_test),
        cmocka_unit_test (logging_syslog_log_handler_debug_test),
        cmocka_unit_test (logging_syslog_log_handler_default_test),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
