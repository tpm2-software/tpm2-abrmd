#ifndef TCTI_ECHO_H
#define TCTI_ECHO_H

#include <sapi/tpm20.h>
#include <sapi/tss2_tcti.h>

TSS2_RC tcti_echo_init (TSS2_TCTI_CONTEXT *context, size_t *size, size_t buf_size);

#endif /* TCTI_ECHO_H */
