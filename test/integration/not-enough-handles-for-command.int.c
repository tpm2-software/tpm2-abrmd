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
    /*
     * Command buffer for a call to TPM2_CC_PolicyNV. It is however missing
     * the third handle from the handle area.
     */
    uint8_t cmd_buf [] = {
        0x80, 0x02, /* TPM2_ST_SESSIONS */
        0x00, 0x00, 0x00, 0x12, /* command buffer size */
        0x00, 0x00, 0x01, 0x49, /* command code: 0x149 / TPM2_CC_PolicyNV */
        0x01, 0x02, 0x03, 0x04, /* first handle */
        0xf0, 0xe0, 0xd0, 0xc0  /* second handle */
    };
    size_t buf_size = sizeof (cmd_buf);

    rc = Tss2_Sys_GetTctiContext (sapi_context, &tcti_context);
    if (rc != TSS2_RC_SUCCESS || tcti_context == NULL) {
        g_error ("Error getting TCTI context via Tss2_Sys_GetTctiContext: 0x%"
                 PRIx32, rc);
    }

    rc = Tss2_Tcti_Transmit (tcti_context, buf_size, cmd_buf);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Error transmitting cmd_buf: 0x%" PRIx32, rc);
    }

    rc = Tss2_Tcti_Receive (tcti_context,
                            &buf_size,
                            cmd_buf,
                            TSS2_TCTI_TIMEOUT_BLOCK);
    g_warning ("just received RC 0x%" PRIx32, rc);
    return rc;
}
