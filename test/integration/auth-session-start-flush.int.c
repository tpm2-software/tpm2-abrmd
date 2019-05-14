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
#include "tpm2-struct-init.h"
#include "test.h"
#define PRIxHANDLE "08" PRIx32
/*
 * This test exercises the session creation logic. We begin by creating the
 * most simple session we can. We then flush it.
  */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC               rc;
    TPMI_SH_AUTH_SESSION  session_handle = 0;
    TPMS_CONTEXT          context = TPMS_CONTEXT_ZERO_INIT;

    /* create an auth session */
    g_info ("Starting unbound, unsalted auth session");
    rc = start_auth_session (sapi_context, &session_handle);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_StartAuthSession failed: 0x%" PRIxHANDLE, rc);
    }
    g_info ("StartAuthSession for TPM_SE_POLICY success! Session handle: "
            "0x%08" PRIx32, session_handle);
    prettyprint_context (&context);

    /* flush context */
    g_info ("Flushing context for session: 0x%" PRIxHANDLE, session_handle);
    rc = Tss2_Sys_FlushContext (sapi_context, session_handle);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_FlushContext failed: 0x%" PRIx32, rc);
    }
    g_info ("Flushed context for session handle: 0x%" PRIxHANDLE " success!",
            session_handle);

    return 0;
}
