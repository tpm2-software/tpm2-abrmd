#include <glib.h>
#include <inttypes.h>

#include "tcti-tabrmd.h"
#include "tpm2-struct-init.h"
#include "common.h"

#define NUM_KEYS 5
int
main (int   argc,
      char *argv[])
{
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    TSS2_SYS_CONTEXT  *sapi_context = NULL;
    TPM_HANDLE         parent_handle, out_handle;
    TPM2B_PRIVATE      out_private = TPM2B_PRIVATE_STATIC_INIT;
    TPM2B_PUBLIC       out_public  = { 0 };
    TSS2_RC rc;

    rc = tcti_context_init (&tcti_context);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to initialize TCTI context");
    rc = sapi_context_init (&sapi_context, tcti_context);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to initialize SAPI context");

    rc = create_primary (sapi_context, &parent_handle);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to create primary key: 0x%" PRIx32, rc);
    g_print ("primary handle: 0x%" PRIx32 "\n", parent_handle);
    guint i;
    for (i = 0; i < NUM_KEYS; ++i) {
        rc = create_key (sapi_context,
                         parent_handle,
                         &out_private,
                         &out_public);
        if (rc != TSS2_RC_SUCCESS)
            g_error ("Failed to create_key: 0x%" PRIx32, rc);
        rc = load_key (sapi_context,
                       parent_handle,
                       &out_handle,
                       &out_private,
                       &out_public);
        if (rc != TSS2_RC_SUCCESS)
            g_error ("Failed to create_key: 0x%" PRIx32, rc);
        out_public.t.size = 0;
    }

    rc = tss2_tcti_tabrmd_dump_trans_state (tcti_context);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to dump transient object state");

    tss2_tcti_finalize (tcti_context);
    free (tcti_context);

    return rc;
}
