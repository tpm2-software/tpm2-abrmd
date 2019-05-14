/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include <gio/gio.h>

#include "util.h"
#include "test-options.h"
#include "tss2-tcti-tabrmd.h"

GBusType
bus_type_from_str (const char *bus_type_str)
{
    if (strcmp (bus_type_str, "system") == 0) {
        return G_BUS_TYPE_SYSTEM;
    } else if (strcmp (bus_type_str, "session") == 0) {
        return G_BUS_TYPE_SESSION;
    } else {
        g_error ("Invalid bus type for %s", bus_type_str);
        return G_BUS_TYPE_NONE;
    }
}
const char*
bus_str_from_type (GBusType bus_type)
{
    switch (bus_type) {
    case G_BUS_TYPE_SESSION:
        return "session";
    case G_BUS_TYPE_SYSTEM:
        return "system";
    default:
        return NULL;
    }
}

/*
 * return 0 if sanity test passes
 * return 1 if sanity test fails
 */
int
sanity_check_test_opts (test_opts_t  *opts)
{
    UNUSED_PARAM(opts);
    return 0;
}

/*
 * Extract configuration from environment and populate the provided
 * `test_opts_t` with the relevant data.
 */
int
get_test_opts_from_env (test_opts_t          *test_opts)
{
    char *env_str;

    if (test_opts == NULL)
        return 1;
    env_str = getenv (ENV_TCTI);
    if (env_str != NULL) {
        g_debug ("%s: %s is \"%s\"", __func__, ENV_TCTI, env_str);
        test_opts->tcti_filename = env_str;
    }
    env_str = getenv (ENV_TCTI_CONF);
    if (env_str != NULL) {
        g_debug ("%s: %s is \"%s\"", __func__, ENV_TCTI_CONF, env_str);
        test_opts->tcti_conf = env_str;
    }
    env_str = getenv (ENV_TCTI_RETRIES);
    if (env_str != NULL) {
        g_debug ("%s: %s is \"%s\"", __func__, ENV_TCTI_RETRIES, env_str);
        test_opts->tcti_retries = strtoumax (env_str, NULL, 10);
    }

    return 0;
}
/*
 * Dump the contents of the test_opts_t structure to stdout.
 */
void
dump_test_opts (test_opts_t *opts)
{
    printf ("test_opts_t:\n");
    printf ("  tcti_filename: %s\n", opts->tcti_filename);
    printf ("  tcti_conf:     %s\n", opts->tcti_conf);
    printf ("  retries:       %" PRIuMAX "\n", opts->tcti_retries);
}
