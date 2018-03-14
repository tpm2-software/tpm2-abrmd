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
#include <inttypes.h>
#include <glib.h>
#include <stdio.h>

#include <tss2/tpm20.h>

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
