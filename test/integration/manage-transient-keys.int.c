/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
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
