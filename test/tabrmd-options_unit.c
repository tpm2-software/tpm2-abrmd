/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tabrmd-options.h"
#include "util.h"

GOptionContext*
__wrap_g_option_context_new (const gchar *parameter_string)
{
    UNUSED_PARAM (parameter_string);

    g_warning ("%s", __func__);
    return mock_type (GOptionContext*);
}
void
__wrap_g_option_context_add_main_entries (
    GOptionContext *context,
    const GOptionEntry *entries,
    const gchar *translation_domain)
{
    UNUSED_PARAM (context);
    UNUSED_PARAM (entries);
    UNUSED_PARAM (translation_domain);

    char *long_name = mock_type (char*);
    size_t i;

    for (i = 0; entries [i].long_name != NULL; ++i) {
        if (strcmp (long_name, entries [i].long_name) == 0) {
            if (strcmp (long_name, "max-connections") == 0 ||
                strcmp (long_name, "max-sessions") == 0 ||
                strcmp (long_name, "max-transients") == 0)
            {
                *(guint*)entries [i].arg_data = mock_type (guint);
            }
            if (strcmp (long_name, "tcti") == 0) {
                *(char**)entries [i].arg_data = mock_type (char*);
            }
        }
    }

    g_warning ("%s", __func__);
    return;
}

gboolean
__wrap_g_option_context_parse (GOptionContext *context,
                               gint *argc,
                               gchar ***argv,
                               GError **error)
{
    UNUSED_PARAM (context);
    UNUSED_PARAM (argc);
    UNUSED_PARAM (argv);

    g_warning ("%s", __func__);
    *error = mock_type (GError*);
    return mock_type (gboolean);
}
static void
tcti_conf_parse_opts_parse_fail (void **state)
{
    UNUSED_PARAM (state);
    tabrmd_options_t options = TABRMD_OPTIONS_INIT_DEFAULT;
    GOptionContext *ctx = NULL;
    int argc = 0;
    char **argv = NULL;
    GError error = { .message = "foo", };

    will_return (__wrap_g_option_context_new, ctx);
    will_return (__wrap_g_option_context_add_main_entries, "foo");
    will_return (__wrap_g_option_context_parse, &error);
    will_return (__wrap_g_option_context_parse, FALSE);
    assert_false (parse_opts (argc, argv, &options));
}
gint
__wrap_set_logger (gchar *name)
{
    UNUSED_PARAM (name);
    return mock_type (gint);
}
static void
tcti_conf_parse_opts_logger_fail (void **state)
{
    UNUSED_PARAM (state);
    tabrmd_options_t options = TABRMD_OPTIONS_INIT_DEFAULT;
    GOptionContext *ctx = NULL;
    int argc = 0;
    char **argv = NULL;
    GError error = { .message = "foo", };

    will_return (__wrap_g_option_context_new, ctx);
    will_return (__wrap_g_option_context_add_main_entries, "foo");
    will_return (__wrap_g_option_context_parse, &error);
    will_return (__wrap_g_option_context_parse, TRUE);
    will_return (__wrap_set_logger, -1);
    assert_false (parse_opts (argc, argv, &options));
}
static void
tcti_conf_parse_opts_max_connections_fail (void **state)
{
    UNUSED_PARAM (state);
    tabrmd_options_t options = TABRMD_OPTIONS_INIT_DEFAULT;
    GOptionContext *ctx = NULL;
    int argc = 0;
    char **argv = NULL;
    GError error = { .message = "foo", };

    will_return (__wrap_g_option_context_new, ctx);
    will_return (__wrap_g_option_context_add_main_entries, "max-connections");
    will_return (__wrap_g_option_context_add_main_entries, 0);
    will_return (__wrap_g_option_context_parse, &error);
    will_return (__wrap_g_option_context_parse, TRUE);
    will_return (__wrap_set_logger, 0);
    assert_false (parse_opts (argc, argv, &options));
}
static void
tcti_conf_parse_opts_max_sessions_fail (void **state)
{
    UNUSED_PARAM (state);
    tabrmd_options_t options = TABRMD_OPTIONS_INIT_DEFAULT;
    GOptionContext *ctx = NULL;
    int argc = 0;
    char **argv = NULL;
    GError error = { .message = "foo", };

    will_return (__wrap_g_option_context_new, ctx);
    will_return (__wrap_g_option_context_add_main_entries, "max-sessions");
    will_return (__wrap_g_option_context_add_main_entries, 0);
    will_return (__wrap_g_option_context_parse, &error);
    will_return (__wrap_g_option_context_parse, TRUE);
    will_return (__wrap_set_logger, 0);
    assert_false (parse_opts (argc, argv, &options));
}
static void
tcti_conf_parse_opts_max_transient_fail (void **state)
{
    UNUSED_PARAM (state);
    tabrmd_options_t options = TABRMD_OPTIONS_INIT_DEFAULT;
    GOptionContext *ctx = NULL;
    int argc = 0;
    char **argv = NULL;
    GError error = { .message = "foo", };

    will_return (__wrap_g_option_context_new, ctx);
    will_return (__wrap_g_option_context_add_main_entries, "max-transients");
    will_return (__wrap_g_option_context_add_main_entries, 0);
    will_return (__wrap_g_option_context_parse, &error);
    will_return (__wrap_g_option_context_parse, TRUE);
    will_return (__wrap_set_logger, 0);
    assert_false (parse_opts (argc, argv, &options));
}
void
__wrap_g_option_context_free (GOptionContext *context)
{
    UNUSED_PARAM (context);

    g_warning ("%s", __func__);
    return;
}
static void
tcti_conf_parse_opts_success (void **state)
{
    UNUSED_PARAM (state);
    tabrmd_options_t options = TABRMD_OPTIONS_INIT_DEFAULT;
    GOptionContext *ctx = NULL;
    int argc = 0;
    char **argv = NULL;
    GError error = { .message = "foo", };

    will_return (__wrap_g_option_context_new, ctx);
    will_return (__wrap_g_option_context_add_main_entries, "tcti");
    will_return (__wrap_g_option_context_add_main_entries, "foo");
    will_return (__wrap_g_option_context_parse, &error);
    will_return (__wrap_g_option_context_parse, TRUE);
    will_return (__wrap_set_logger, 0);
    assert_true (parse_opts (argc, argv, &options));
}

int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (tcti_conf_parse_opts_parse_fail),
        cmocka_unit_test (tcti_conf_parse_opts_logger_fail),
        cmocka_unit_test (tcti_conf_parse_opts_max_connections_fail),
        cmocka_unit_test (tcti_conf_parse_opts_max_sessions_fail),
        cmocka_unit_test (tcti_conf_parse_opts_max_transient_fail),
        cmocka_unit_test (tcti_conf_parse_opts_success),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
