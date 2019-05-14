/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <inttypes.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <tss2/tss2_sys.h>

#include "common.h"
#include "tabrmd-defaults.h"
#include "test-options.h"
#include "context-util.h"

#define PRIxHANDLE "08" PRIx32
#define TEST_MAX_SESSIONS (TABRMD_SESSIONS_MAX_DEFAULT + 1)

/*
 * This test exercises the session tracking logic specifically around the
 * requirements mentioned in section 3.7. We've started calling
 * this "session continuation" where a saved session can be loaded by a
 * different connection.
 *
 * This test is specifically designed to cause the LRU algorithm in the
 * tabrmd to flush continued sessions associated with connections that have
 * been closed. The LRU algorithm ensures that we have a bounded number of
 * sessions in this intermediate state (continued but unclaimed) at any
 * given time.
 *
 * To accomplish this we create a fixed number (TEST_MAX_SESSIONS) of
 * connections with a single session created by each. Each session is saved
 * and the connection closed. Each session should be available for another
 * connection to load still but the number of connections created was
 * specifically chosen to be one more than the number of unowned sessions
 * allowed by the tabrmd. This causes one session to be closed summarily
 * by the daemon and so the session will not be loadable by this test.
 */

typedef struct {
    TPMI_SH_AUTH_SESSION handle;
    TPMS_CONTEXT context;
    bool load_success;
} test_data_t;
#define TEST_DATA_ZERO_INIT { 0, { 0, 0, 0, { 0, 0 } }, false }

void
create_connection_and_save_sessions (test_opts_t *opts,
                                     test_data_t data [],
                                     size_t count)
{
    TSS2_RC rc;
    TSS2_SYS_CONTEXT *sapi_context;
    size_t i;

    g_info ("Creating and saving %zu policy sessions", count);
    for (i = 0; i < count; ++i) {
        sapi_context = sapi_init_from_opts (opts);
        if (sapi_context == NULL) {
            g_error ("Failed to create SAPI context.");
        }
        g_info ("Got SAPI context: 0x%" PRIxPTR, (uintptr_t)sapi_context);
        /* create an auth session */
        g_info ("Starting unbound, unsalted auth session");
        rc = start_auth_session (sapi_context, &data [i].handle);
        if (rc != TSS2_RC_SUCCESS) {
            g_error ("Tss2_Sys_StartAuthSession failed: 0x%" PRIxHANDLE, rc);
        }
        g_info ("StartAuthSession for TPM_SE_POLICY success! Session handle: "
                "0x%08" PRIx32, data [i].handle);

        /* save context */
        g_info ("Saving context for session: 0x%" PRIxHANDLE,
                data [i].handle);
        rc = Tss2_Sys_ContextSave (sapi_context,
                                   data [i].handle,
                                   &data [i].context);
        if (rc != TSS2_RC_SUCCESS) {
            g_error ("Tss2_Sys_ContextSave failed: 0x%" PRIxHANDLE, rc);
        }
        g_info ("Successfully saved context for session: 0x%" PRIxHANDLE,
                data [i].handle);
        prettyprint_context (&data [i].context);
        g_info ("Tearing down SAPI connection 0x%" PRIxPTR,
                (uintptr_t)sapi_context);
        sapi_teardown_full (sapi_context);
    }
}
/*
 * This function attempts to load 'count' session contexts from the provided
 * array. The handles for the loaded sessions are returned in the
 * handle array. The number of contexts loaded is returned.
 */
size_t
load_sessions (TSS2_SYS_CONTEXT *sapi_context,
               test_data_t data [],
               size_t count)
{
    TSS2_RC rc;
    size_t i, success_count = 0;

    for (i = 0; i < count; ++i) {
        g_debug ("Loading saved context with handle: 0x%08" PRIx32,
                 data [i].context.savedHandle);
        rc = Tss2_Sys_ContextLoad (sapi_context,
                                   &data [i].context,
                                   &data [i].handle);
        if (rc == TSS2_RC_SUCCESS) {
            g_info ("Successfully loaded context for session: 0x%" PRIxHANDLE,
                    data [i].handle);
            data [i].load_success = true;
            ++success_count;
        } else {
            g_warning ("Tss2_Sys_ContextLoad failed: 0x%" PRIxHANDLE, rc);
        }
    }

    return success_count;
}
int
main ()
{
    TSS2_SYS_CONTEXT *sapi_context;
    test_data_t test_data [TEST_MAX_SESSIONS] =  {
        TEST_DATA_ZERO_INIT, TEST_DATA_ZERO_INIT, TEST_DATA_ZERO_INIT,
        TEST_DATA_ZERO_INIT, TEST_DATA_ZERO_INIT };
    test_opts_t opts = TEST_OPTS_DEFAULT_INIT;
    guint success_count;

    get_test_opts_from_env (&opts);
    if (sanity_check_test_opts (&opts) != 0)
        exit (1);
    /* create TEST_MAX_SESSIONS each over a unique connection to the tabrmd */
    create_connection_and_save_sessions (&opts, test_data, TEST_MAX_SESSIONS);
    /* create fresh connection for the test */
    sapi_context = sapi_init_from_opts (&opts);
    if (sapi_context == NULL) {
        g_error ("Failed to create SAPI context.");
    }
    /*
     * Of the continued sessions above, only TABRMD_MAX_SESSIONS should be
     * available to us.
     */
    success_count = load_sessions (sapi_context, test_data, TEST_MAX_SESSIONS);
    if (success_count != TABRMD_SESSIONS_MAX_DEFAULT) {
        g_critical ("Expected to load %u sessions, got %u instead",
                    TABRMD_SESSIONS_MAX_DEFAULT, success_count);
        exit (1);
    }
    /*
     * The first session we created is the oldest. The LRU should have selected
     * this one for eviction
     */
    if (test_data [0].load_success == true) {
        g_critical ("Oldest session created was loaded successfully.");
        exit (1);
    }
    sapi_teardown_full (sapi_context);

    return 0;
}
