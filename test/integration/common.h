/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
/*
 * These are common functions used by the integration tests.
 */
#include <inttypes.h>
#include <tss2/tss2_sys.h>
#include <tss2/tss2_tcti.h>

/* Current ABI version */
#define SUPPORTED_ABI_VERSION \
{ \
    .tssCreator = 1, \
    .tssFamily = 2, \
    .tssLevel = 1, \
    .tssVersion = 108, \
}

/*
 * This macro is useful as a wrapper around SAPI functions to automatically
 * retry function calls when the RC is TPM2_RC_RETRY.
 */
#define TSS2_RETRY_EXP(expression)                         \
    ({                                                     \
        TSS2_RC __result = 0;                              \
        do {                                               \
            __result = (expression);                       \
        } while ((__result & 0x0000ffff) == TPM2_RC_RETRY); \
        __result;                                          \
    })
#define PRIxHANDLE "08" PRIx32

TSS2_RC
create_primary (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM2_HANDLE       *handle
    );

TSS2_RC
create_key (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM2_HANDLE        parent_handle,
    TPM2B_PRIVATE    *out_private,
    TPM2B_PUBLIC     *out_public
    );

TSS2_RC
load_key (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM2_HANDLE        parent_handle,
    TPM2_HANDLE       *out_handle,
    TPM2B_PRIVATE    *in_private,
    TPM2B_PUBLIC     *in_public
    );

TSS2_RC
save_context (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM2_HANDLE        handle,
    TPMS_CONTEXT     *context
    );

TSS2_RC
flush_context (
    TSS2_SYS_CONTEXT *sapi_context,
    TPM2_HANDLE        handle
    );

TSS2_RC
start_auth_session (
    TSS2_SYS_CONTEXT      *sapi_context,
    TPMI_SH_AUTH_SESSION  *session_handle
    );

void
prettyprint_context (
    TPMS_CONTEXT *context
    );

void
clean_up_all (
    TSS2_SYS_CONTEXT *sapi_context
    );
