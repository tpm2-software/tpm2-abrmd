/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */

/*
 * This test instantiates CONNECTION_COUNT TCTI instances sending a pre-canned
 * TPM command through each. It terminates each connection before receiving
 * the result.
 *
 * This test ensures that a single process is able to instantiate and use
 * multiple connections to the tabrmd.
 *
 * NOTE: this test can't and doesn't use the main.c driver from the
 * integration test harness since we need to instantiate more than a single
 * TCTI context.
 */

#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tss2-tcti-tabrmd.h"
#include "test-options.h"

#define CONNECTION_COUNT 25

TSS2_RC
tcti_tabrmd_init (TSS2_TCTI_CONTEXT **tcti_context,
                  const char         *conf,
                  int                 retries)
{
    TSS2_RC rc;
    size_t size, i;

    rc = Tss2_Tcti_Tabrmd_Init (NULL, &size, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        fprintf (stderr, "Failed to get allocation size for tabrmd TCTI "
                 " context: 0x%" PRIx32 "\n", rc);
        return rc;
    }
    *tcti_context = calloc (1, size);
    if (*tcti_context == NULL) {
        fprintf (stderr, "Allocation for TCTI context failed: %s\n",
                 strerror (errno));
        return rc;
    }
    for (i = 0; i < (size_t) retries; ++i) {
        rc = Tss2_Tcti_Tabrmd_Init (*tcti_context, &size, conf);
        if (rc == TSS2_RC_SUCCESS) {
            break;
        } else {
            g_info ("Failed to initialize tabrmd TCTI context: 0x%" PRIx32
                    " on try %zd", rc, i);
            sleep (1);
        }
    }
    if (rc != TSS2_RC_SUCCESS) {
        fprintf (stderr, "Failed to initialize tabrmd TCTI context: "
                 "0x%" PRIx32 "\n", rc);
        free (*tcti_context);
    }
    return rc;
}

int
main (int   argc,
      char *argv[])
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *tcti_context[CONNECTION_COUNT] = { 0 };
    test_opts_t opts = TEST_OPTS_DEFAULT_INIT;
    /* This is a pre-canned TPM2 command buffer to invoke 'GetCapability' */
    uint8_t cmd_buf[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00,
        0x01, 0x7a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x0a
    };

    g_info ("Executing test: %s", argv[0]);
    if (argc > 1) {
        g_error ("Unexpected argument count %d", argc);
    }
    get_test_opts_from_env (&opts);
    if (sanity_check_test_opts (&opts) != 0) {
        g_error ("option sanity test failed");
    }
    size_t i;
    for (i = 0; i < CONNECTION_COUNT; ++i) {
        rc = tcti_tabrmd_init (&tcti_context [i],
                               opts.tcti_conf,
                               opts.tcti_retries);
        if (tcti_context [i] == NULL || rc != TSS2_RC_SUCCESS) {
            g_error ("failed to connect to TCTI: 0x%" PRIx32, rc);
        }
    }
    for (i = 0; i < CONNECTION_COUNT; ++i) {
        rc = Tss2_Tcti_Transmit (tcti_context [i], sizeof (cmd_buf), cmd_buf);
        if (rc != TSS2_RC_SUCCESS) {
            g_error ("failed to transmit TPM command");
        }
    }
    for (i = 0; i < CONNECTION_COUNT; ++i) {
        Tss2_Tcti_Finalize (tcti_context [i]);
        free (tcti_context [i]);
    }
}
