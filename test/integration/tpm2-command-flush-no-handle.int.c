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
