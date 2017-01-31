/*
 * This is an integration test that is intended to be run directly agianst
 * the tpm2-abrmd. It creates transient objects (RSA keys), loads them, then
 * saves the context and flushes it. This is intended to exercise the special
 * handling of the TPM2_FlushContext function that is required in the daemon.
 */
#include <glib.h>

#include "common.h"
#include "tpm2-struct-init.h"

int
main (int   argc,
      char *argv[])
{
    TSS2_TCTI_CONTEXT *tcti_context;
    TSS2_SYS_CONTEXT  *sapi_context;
    TPM_HANDLE         primary_handle, out_handle;
    TPMS_CONTEXT       context     = { 0 };
    TPM2B_PRIVATE      out_private = TPM2B_PRIVATE_STATIC_INIT;
    TPM2B_PUBLIC       out_public  = { 0 };
    TSS2_RC            rc;

    rc = tcti_context_init (&tcti_context);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to initialize TCTI context");
    rc = sapi_context_init (&sapi_context, tcti_context);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to initialize SAPI context");

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
}
