/*
 * These are common functions used by the integration tests.
 */
#include <glib.h>

#include "common.h"

/*
 * Instantiate a TCTI context for communication with the tabrmd.
 */
TSS2_TCTI_CONTEXT*
tcti_context_init (TSS2_RC *rc)
{
    TSS2_TCTI_CONTEXT *tcti_context;
    size_t context_size;

    *rc = tss2_tcti_tabrmd_init (NULL, &context_size);
    if (*rc != TSS2_RC_SUCCESS)
        g_error ("failed to get size of tcti context");
    g_debug ("tcti size is: %d", context_size);
    tcti_context = calloc (1, context_size);
    if (tcti_context == NULL)
        g_error ("failed to allocate memory for tcti context");
    g_debug ("context structure allocated successfully");
    *rc = tss2_tcti_tabrmd_init (tcti_context, NULL);
    if (*rc != TSS2_RC_SUCCESS)
        g_error ("failed to initialize tcti context. RC: 0x%" PRIx32, *rc);

    return tcti_context;
}
/*
 * Initialize a SAPI context using the TCTI context provided by the caller.
 * This function allocates memory for the SAPI context and returns it to the
 * caller. This memory must be freed by the caller.
 */
TSS2_SYS_CONTEXT*
sapi_context_init (TSS2_TCTI_CONTEXT *tcti_context,
                   TSS2_RC *rc)
{
    TSS2_SYS_CONTEXT *sapi_context;
    size_t size;
    TSS2_ABI_VERSION abi_version = {
        .tssCreator = TSSWG_INTEROP,
        .tssFamily  = TSS_SAPI_FIRST_FAMILY,
        .tssLevel   = TSS_SAPI_FIRST_LEVEL,
        .tssVersion = TSS_SAPI_FIRST_VERSION,
    };

    size = Tss2_Sys_GetContextSize (0);
    sapi_context = (TSS2_SYS_CONTEXT*)calloc (1, size);
    if (sapi_context == NULL) {
        g_warning ("Failed to allocate 0x%" PRIxMAX " bytes for the SAPI "
                   "context", size);
        return NULL;
    }
    *rc = Tss2_Sys_Initialize (sapi_context, size, tcti_context, &abi_version);
    if (*rc != TSS2_RC_SUCCESS) {
        g_warning ("Failed to initialize SAPI context: 0x%" PRIx32, *rc);
        free (sapi_context);
        return NULL;
    }
    return sapi_context;
}
