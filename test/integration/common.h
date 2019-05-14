/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
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
get_context_gap_max (
    TSS2_SYS_CONTEXT *sys_context,
    UINT32 *value);

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
