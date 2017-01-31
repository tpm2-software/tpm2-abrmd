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
