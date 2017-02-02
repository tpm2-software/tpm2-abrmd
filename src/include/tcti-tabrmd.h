#ifndef TSS2_TCTI_TABD_H
#define TSS2_TCTI_TABD_H

#include <sapi/tpm20.h>
#include <sapi/tss2_tcti.h>

TSS2_RC tss2_tcti_tabrmd_init (TSS2_TCTI_CONTEXT *context, size_t *size);
TSS2_RC tss2_tcti_tabrmd_dump_trans_state (TSS2_TCTI_CONTEXT *tcti_context);

#endif /* TSS2_TCTI_TABD_H */
