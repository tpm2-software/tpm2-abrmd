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
/*
 * This is a test program intended to create a number of TCTI connections
 * with the tabrmd. The goal is to hit the upper bound on the number of
 * concurrent connections and get the TCTI to return an error.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tabrmd.h"
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
