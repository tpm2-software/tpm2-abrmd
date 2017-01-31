/*
 * These are common functions used by the integration tests.
 */
#include <inttypes.h>
#include <sapi/tpm20.h>

TSS2_RC
tcti_context_init (
    TSS2_TCTI_CONTEXT **tcti_context
    );

TSS2_RC
sapi_context_init (
    TSS2_SYS_CONTEXT    **sapi_context,
    TSS2_TCTI_CONTEXT    *tcti_context
    );

TSS2_RC
create_primary (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE       *handle
    );

TSS2_RC
create_key (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE        parent_handle,
    TPM2B_PRIVATE    *out_private,
    TPM2B_PUBLIC     *out_public
    );

TSS2_RC
load_key (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM_HANDLE        parent_handle,
    TPM_HANDLE       *out_handle,
    TPM2B_PRIVATE    *in_private,
    TPM2B_PUBLIC     *in_public
    );
