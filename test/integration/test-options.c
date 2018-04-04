/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
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
 * Parse command line options from argv extracting test options. These are
 * returned to the caller in the provided options structure.
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
