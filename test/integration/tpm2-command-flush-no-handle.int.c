/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <inttypes.h>
#include <glib.h>
#include <stdio.h>

#include <tss2/tss2_sys.h>
#include <tpm2-header.h>

#include "common.h"
#include "test.h"

/*
 * This test sends the tabrmd a malformed ContextFlush command. The command
 * size is set to the size of the header which omits the handle being flushed.
 * This is designed to get the tabrmd to attempt to access the handle
 * parameter that will be outside of the buffer allocated by the
 * CommandSource.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    uint8_t cmd_buf [] = {
        0x80, 0x01, /* TPM2_ST_NO_SESSIONS */
        0x00, 0x00, 0x00, 0x0a, /* size: 10 bytes */
        0x00, 0x00, 0x01, 0x65, /* command code for ContextFlush */
    };
    size_t buf_size = sizeof (cmd_buf);

    rc = Tss2_Sys_GetTctiContext (sapi_context, &tcti_context);
    if (rc != TSS2_RC_SUCCESS || tcti_context == NULL) {
        g_error ("Error getting TCTI context via Tss2_Sys_GetTctiContext: 0x%"
                 PRIx32, rc);
    }

    rc = Tss2_Tcti_Transmit (tcti_context, sizeof (cmd_buf), cmd_buf);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Error transmitting cmd_buf: 0x%" PRIx32, rc);
    }
    rc = Tss2_Tcti_Receive (tcti_context,
                            &buf_size,
                            cmd_buf,
                            TSS2_TCTI_TIMEOUT_BLOCK);
    if (rc != TSS2_RC_SUCCESS) {
        return rc;
    }
    /* we're expecting a response buffer with just an RC */
    rc = 0x0000ffff & be32toh (cmd_buf [6]);
    if (rc != TPM2_RC_INSUFFICIENT) {
        return rc;
    }

    return 0;
}
