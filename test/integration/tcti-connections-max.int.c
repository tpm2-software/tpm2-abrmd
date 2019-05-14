/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
/*
 * This is a test program intended to create a number of TCTI connections
 * with the tabrmd. The goal is to hit the upper bound on the number of
 * concurrent connections and get the TCTI to return an error.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tabrmd-defaults.h"
#include "test-options.h"
#include "context-util.h"
#include "tss2-tcti-tabrmd.h"

#define TCTI_COUNT_MAX 100

int
main ()
{
    TSS2_TCTI_CONTEXT *tcti_context[TCTI_COUNT_MAX] = { NULL };
    uint8_t            tcti_count = TABRMD_CONNECTIONS_MAX_DEFAULT, i;
    test_opts_t test_opts = TEST_OPTS_DEFAULT_INIT;

    if (tcti_count > TCTI_COUNT_MAX) {
        printf ("Cannot create more than %" PRIx8 " connections\n",
                TCTI_COUNT_MAX);
        exit (1);
    }

    get_test_opts_from_env (&test_opts);
    for (i = 0; i < tcti_count; ++i) {
        printf ("creating TCTI %" PRIu8 "\n", i);
        tcti_context [i] = tcti_init_from_opts (&test_opts);

        if (tcti_context [i] == NULL) {
            if (i == TABRMD_CONNECTIONS_MAX_DEFAULT - 1) {
                g_info ("Failed to initialize TCTI after default connection max, success");
                exit (1);
            } else {
                g_warning ("failed to initialize TCTI connection number %" PRIu8
                           " with tabrmd", i);
                exit (2);
            }
        }
    }
}
