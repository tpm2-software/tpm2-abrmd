/*
 * These are common functions used by the integration tests.
 */
#include <glib.h>

#include "common.h"

/*
 * Instantiate a TCTI context for communication with the tabrmd.
 */
TSS2_RC
tcti_context_init (TSS2_TCTI_CONTEXT **tcti_context)
{
    TSS2_TCTI_CONTEXT *tmp_tcti_context;
    TSS2_RC rc;
    size_t context_size;

    if (tcti_context == NULL)
        g_error ("tcti_context_init passed NULL reference");
    rc = tss2_tcti_tabrmd_init (NULL, &context_size);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to get size of tcti context");
    g_debug ("tcti size is: %d", context_size);
    tmp_tcti_context = calloc (1, context_size);
    if (tmp_tcti_context == NULL)
        g_error ("failed to allocate memory for tcti context");
    g_debug ("context structure allocated successfully");
    rc = tss2_tcti_tabrmd_init (tmp_tcti_context, NULL);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to initialize tcti context. RC: 0x%" PRIx32, rc);

    *tcti_context = tmp_tcti_context;
    return rc;
}
/*
 * Initialize a SAPI context using the TCTI context provided by the caller.
 * This function allocates memory for the SAPI context and returns it to the
 * caller. This memory must be freed by the caller.
 */
TSS2_RC
sapi_context_init (TSS2_SYS_CONTEXT  **sapi_context,
                   TSS2_TCTI_CONTEXT  *tcti_context)
{
    TSS2_SYS_CONTEXT *tmp_sapi_context;
    TSS2_RC rc;
    size_t size;
    TSS2_ABI_VERSION abi_version = {
        .tssCreator = TSSWG_INTEROP,
        .tssFamily  = TSS_SAPI_FIRST_FAMILY,
        .tssLevel   = TSS_SAPI_FIRST_LEVEL,
        .tssVersion = TSS_SAPI_FIRST_VERSION,
    };

    if (sapi_context == NULL || tcti_context == NULL)
        g_error ("sapi_context_init passed NULL reference");
    size = Tss2_Sys_GetContextSize (0);
    tmp_sapi_context = (TSS2_SYS_CONTEXT*)calloc (1, size);
    if (tmp_sapi_context == NULL) {
        g_error ("Failed to allocate 0x%" PRIxMAX " bytes for the SAPI "
                 "context", size);
    }
    rc = Tss2_Sys_Initialize (tmp_sapi_context, size, tcti_context, &abi_version);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Failed to initialize SAPI context: 0x%" PRIx32, rc);
        free (tmp_sapi_context);
        return rc;
    }

    *sapi_context = tmp_sapi_context;
    return rc;
}
