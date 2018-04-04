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
/*
 * This is an integration test that is intended to be run directly against
 * the tpm2-abrmd. It creates transient objects (RSA keys), loads them, then
 * saves the context and flushes it. This is intended to exercise the special
 * handling of the TPM2_FlushContext function that is required in the daemon.
 */
#include <glib.h>

#include "common.h"
#include "tpm2-struct-init.h"

int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TPM2_HANDLE        primary_handle, out_handle;
    TPMS_CONTEXT       context = TPMS_CONTEXT_ZERO_INIT;
    TPM2B_PRIVATE      out_private = TPM2B_PRIVATE_STATIC_INIT;
    TPM2B_PUBLIC       out_public  = TPM2B_PUBLIC_ZERO_INIT;
    TSS2_RC            rc;

    rc = create_primary (sapi_context, &primary_handle);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to create primary key: 0x%" PRIx32, rc);

    rc = create_key (sapi_context,
                     primary_handle,
                     &out_private,
                     &out_public);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to create_key: 0x%" PRIx32, rc);
    rc = load_key (sapi_context,
                   primary_handle,
                   &out_handle,
                   &out_private,
                   &out_public);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to load_key: 0x%" PRIx32, rc);
    rc = save_context (sapi_context, out_handle, &context);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to save_context: 0x%" PRIx32, rc);
    rc = flush_context (sapi_context, out_handle);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("filed to flush_context: 0x%" PRIx32, rc);
    return 0;
}
