#ifndef TSS2TCTI_TABRMD_PRIV_H
#define TSS2TCTI_TABRMD_PRIV_H

#include <glib.h>
#include <pthread.h>

#include <sapi/tpm20.h>

#include "tabrmd-generated.h"

#define TSS2_TCTI_TABRMD_MAGIC 0x1c8e03ff00db0f92
#define TSS2_TCTI_TABRMD_VERSION 1

#define TSS2_TCTI_TABRMD_ID(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->id
#define TSS2_TCTI_TABRMD_WATCHER_ID(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->watcher_id
#define TSS2_TCTI_TABRMD_MUTEX(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->mutex
#define TSS2_TCTI_TABRMD_PIPE_FDS(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->pipe_fds
#define TSS2_TCTI_TABRMD_PIPE_RECEIVE(context) \
    TSS2_TCTI_TABRMD_PIPE_FDS(context)[0]
#define TSS2_TCTI_TABRMD_PIPE_TRANSMIT(context) \
    TSS2_TCTI_TABRMD_PIPE_FDS(context)[1]
#define TSS2_TCTI_TABRMD_PROXY(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->proxy
#define TSS2_TCTI_TABRMD_HEADER(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->header

/*
 * A convenience macro to get us the size of the TPM header. Do not expect
 * sizeof (tpm_header_t) to get your the size of the header. The compiler pads
 * this structure.
 */
#define TPM_HEADER_SIZE (UINT32)(sizeof (TPM_ST) + sizeof (UINT32) + sizeof (UINT32))
/* a generic tpm header structure, could be command or response */
typedef struct {
    TPM_ST   tag;
    UINT32   size;
    UINT32   code;
} tpm_header_t;

/* This is our private TCTI structure. We're required by the spec to have
 * the same structure as the non-opaque area defined by the
 * TSS2_TCTI_CONTEXT_COMMON_V1 structure. Anything after this data is opaque
 * and private to our implementation. See section 7.3 of the SAPI / TCTI spec
 * for the details.
 */
typedef struct {
    TSS2_TCTI_CONTEXT_COMMON_V1    common;
    guint64                        id;
    guint                          watcher_id;
    pthread_mutex_t                mutex;
    int                            pipe_fds[2];
    TctiTabrmd                    *proxy;
    tpm_header_t                   header;
} TSS2_TCTI_TABRMD_CONTEXT;

#endif /* TSS2TCTI_TABRMD_PRIV_H */
