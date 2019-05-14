/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <inttypes.h>
#include <glib.h>
#include <stdio.h>

#include <tss2/tss2_sys.h>

#include "common.h"
#include "test.h"

/*
  */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    uint8_t cmd_buf [] = {
        0x80, 0x01, /* TPM2_ST_NO_SESSIONS */
        0x00, 0x00, 0x20, 0x00, /* size: 8192 bytes */
        0x00, 0x00, 0x01, 0x62, /* command code for ContextSave */
        0x00, 0x00, 0x00, 0x00, /* handle not used or required */
    };

    rc = Tss2_Sys_GetTctiContext (sapi_context, &tcti_context);
    if (rc != TSS2_RC_SUCCESS || tcti_context == NULL) {
        g_error ("Error getting TCTI context via Tss2_Sys_GetTctiContext: 0x%"
                 PRIx32, rc);
    }

    rc = Tss2_Tcti_Transmit (tcti_context, sizeof (cmd_buf), cmd_buf);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Error transmitting cmd_buf: 0x%" PRIx32, rc);
    }

    return rc;
}
