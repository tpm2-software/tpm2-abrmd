/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <tss2/tss2_sys.h>

#include "tss2-tcti-tabrmd.h"

/*
 * This is a test program that exercises the TCTI cancel command.
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
    Tss2_Tcti_Finalize (tcti_context);
    Tss2_Tcti_Finalize (tcti_context);
    return 0;
}
