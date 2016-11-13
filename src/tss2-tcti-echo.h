#ifndef TSS2_TCTI_ECHO_H
#define TSS2_TCTI_ECHO_H

#include <tpm20.h>

#define TCTI_ECHO_MAGIC 0xd511f126d4656f6d
#define TSS2_TCTI_ECHO_MIN_BUF ( 1024 )
#define TSS2_TCTI_ECHO_MAX_BUF ( 8 * TSS2_TCTI_ECHO_MIN_BUF )

TSS2_RC  tss2_tcti_echo_init (TSS2_TCTI_CONTEXT *tcti_context,
                              size_t            *ctx_size,
                              size_t             buf_size);

#endif /* TSS2_TCTI_ECHO_H */
