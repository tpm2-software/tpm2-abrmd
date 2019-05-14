/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>

#include "tabrmd.h"
#include "tss2-tcti-tabrmd.h"
#include "common.h"

#define NUM_KEYS 5
#define ENV_NUM_KEYS "TABRMD_TEST_NUM_KEYS"

/*
 * This is a test program that exercises the TCTI cancel command.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    TSS2_RC            rc = TSS2_RC_SUCCESS;
    uint8_t            cmd_buffer [] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00,
        0x01, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x40
    };
    size_t cmd_buffer_size = sizeof (cmd_buffer);

    rc = Tss2_Sys_GetTctiContext (sapi_context, &tcti_context);
    if (rc != TSS2_RC_SUCCESS || tcti_context == NULL) {
        g_critical ("Tss2_Sys_GetTctiContext failed: 0x%" PRIx32, rc);
        return 1;
    }
    g_info ("sending command buffer");
    rc = Tss2_Tcti_Transmit (tcti_context, cmd_buffer_size, cmd_buffer);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("failed to send command");
        return 1;
    }
    g_info ("invoking tss2_tcti_tabrmd_cancel");
    rc = Tss2_Tcti_Cancel (tcti_context);
    if (rc == TSS2_RESMGR_RC_NOT_IMPLEMENTED) {
        g_warning ("Tss2_Tcti_Cancel failed as expected: 0x%" PRIx32,
                   rc);
        return 0;
    } else {
        g_critical ("Tss2_Tcti_Cancel returned unexpected rc: 0x%"
                    PRIx32, rc);
        return 1;
    }
}
