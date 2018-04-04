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
#include <inttypes.h>
#include <glib.h>
#include <stdio.h>

#include <tss2/tss2_sys.h>

#include "common.h"
#include "tpm2-struct-init.h"
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

    /* save context */
    g_info ("Saving context for session: 0x%" PRIxHANDLE, session_handle);
    rc = Tss2_Sys_ContextSave (sapi_context, session_handle, &context);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_ContextSave failed: 0x%" PRIxHANDLE, rc);
    }
    g_info ("Successfully saved context for session: 0x%" PRIxHANDLE,
            session_handle);
    prettyprint_context (&context);

    return 0;
}
