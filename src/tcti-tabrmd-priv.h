/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
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
    int                            pipe_fds[2];
    TctiTabrmd                    *proxy;
    tpm_header_t                   header;
} TSS2_TCTI_TABRMD_CONTEXT;

#endif /* TSS2TCTI_TABRMD_PRIV_H */
