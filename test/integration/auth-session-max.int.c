/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <inttypes.h>
#include <glib.h>
#include <stdio.h>

#include <tss2/tss2_tpm2_types.h>

#include "common.h"
#include "tabrmd-defaults.h"
#include "tabrmd.h"
#include "test.h"

#define PRIxHANDLE "08" PRIx32
/*
 * This test exercises the session creation logic. We begin by creating the
 * most simple session we can. We then save it. When this test terminates
 * the RM should flush the context for us.
  */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC               rc;
    TPMI_SH_AUTH_SESSION  session_handle [TABRMD_SESSIONS_MAX_DEFAULT + 1] = { 0 };
    size_t i;

    for (i = 0; i < TABRMD_SESSIONS_MAX_DEFAULT + 1; ++i) {
        /* create an auth session */
        g_info ("Starting unbound, unsalted auth session");
        rc = start_auth_session (sapi_context, &session_handle [i]);
        if (rc == TSS2_RESMGR_RC_SESSION_MEMORY &&
            i == TABRMD_SESSIONS_MAX_DEFAULT) {
            g_info ("Tss2_Sys_StartAuthSession failed: 0x%" PRIxHANDLE
                    " trying to create session #%zd", rc, i + 1);
            return 0;
        }
        if (rc != TSS2_RC_SUCCESS) {
            g_critical ("Got unexpected response code: 0x%" PRIxHANDLE, rc);
            return 1;
        }
        g_info ("StartAuthSession for TPM_SE_POLICY success! Session handle: "
                "0x%08" PRIx32, session_handle [i]);
    }
    /*
     * this test should never get here. An error should be caught in the loop
     * above which is what we want. If execution reaches this point in the
     * function then the upper bound isn't working properly
     */
    return 1;
}
