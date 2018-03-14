/*
 * Copyright (c) 2017, Intel Corporation
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
#include <glib.h>
#include <inttypes.h>

#include "tabrmd.h"
#include "tss2-tcti-tabrmd.h"
#include "common.h"

#define NUM_KEYS 5
#define ENV_NUM_KEYS "TABRMD_TEST_NUM_KEYS"

/*
 * This is a test program to exercise the TCTI set locality command.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    TSS2_RC            rc = TSS2_RC_SUCCESS;

    rc = Tss2_Sys_GetTctiContext (sapi_context, &tcti_context);
    if (rc != TSS2_RC_SUCCESS || tcti_context == NULL) {
        g_critical ("Tss2_Sys_GetTctiContext failed: 0x%" PRIx32, rc);
        return 1;
    }
    g_info ("invoking tss2_tcti_tabrmd_set_locality");
    rc = Tss2_Tcti_SetLocality (tcti_context, 1);
    if (rc == TSS2_RESMGR_RC_NOT_IMPLEMENTED) {
        g_info ("tss2_tcti_tabrmd_set_locality failed as expected: 0x%" PRIx32,
                rc);
        return 0;
    } else {
        g_critical ("tss2_tcti_tabrmd_set_locality returned unexpected rc: 0x%"
                    PRIx32, rc);
        return 1;
    }
}
