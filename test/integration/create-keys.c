#include <glib.h>
#include <inttypes.h>

#include "tcti-tabrmd.h"
#include "tpm2-struct-init.h"
#include "common.h"

#define NUM_KEYS 5
#define ENV_NUM_KEYS "TABRMD_TEST_NUM_KEYS"

/*
 * This is a test program that creates and loads a configurable number of
 * transient objects in the NULL hierarchy. The number of keys created
 * undert the NULL primary key can be provided as a base 10 integer on
 * the command line. This is the only parameter the program takes.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_TCTI_CONTEXT *tcti_context;
    TPM_HANDLE         parent_handle, out_handle;
    TPM2B_PRIVATE      out_private = TPM2B_PRIVATE_STATIC_INIT;
    TPM2B_PUBLIC       out_public  = { 0 };
    TSS2_RC            rc = TSS2_RC_SUCCESS;
    char              *env_str = NULL, *end_ptr = NULL;
    uint8_t            loops = NUM_KEYS;

    env_str = getenv (ENV_NUM_KEYS);
    if (env_str != NULL)
        loops = strtol (env_str, &end_ptr, 10);

    g_print ("%s: %d", ENV_NUM_KEYS, loops);
    rc = create_primary (sapi_context, &parent_handle);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to create primary key: 0x%" PRIx32, rc);
    g_print ("primary handle: 0x%" PRIx32 "\n", parent_handle);
    guint i;
    for (i = 0; i < loops; ++i) {
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

    rc = Tss2_Sys_GetTctiContext (sapi_context, &tcti_context);
    if (rc != TSS2_RC_SUCCESS && tcti_context != NULL) {
        rc = tss2_tcti_tabrmd_dump_trans_state (tcti_context);
        if (rc != TSS2_RC_SUCCESS)
            g_error ("failed to dump transient object state");
    }

    return rc;
}
