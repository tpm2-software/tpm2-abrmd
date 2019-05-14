/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef TCTI_MOCK_H
#define TCTI_MOCK_H

#include <tss2/tss2_tcti.h>

typedef enum {
    STATE_0,
    SEND,
    RECEIVE,
    N_STATE,
} TCTI_STATE;
/*
 * Normally this structure would not be exposed in a TCTI's header. For the
 * purposes of testing we need access to this structure in order to setup
 * and verify the results of tests.
 */
typedef struct {
    TSS2_TCTI_CONTEXT_COMMON_V2 v2;
    TCTI_STATE state;
} TCTI_MOCK_CONTEXT;

TSS2_RC
Tss2_Tcti_Mock_Init (TSS2_TCTI_CONTEXT *context,
                     size_t *size,
                     const char *conf);
TSS2_TCTI_CONTEXT*
tcti_mock_init_full (void);

#endif /* TSS2_TCTI_TABD_H */
