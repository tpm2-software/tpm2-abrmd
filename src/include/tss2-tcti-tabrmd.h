/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TSS2_TCTI_TABD_H
#define TSS2_TCTI_TABD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <tss2/tss2_tcti.h>

TSS2_RC Tss2_Tcti_Tabrmd_Init (TSS2_TCTI_CONTEXT *context,
                               size_t *size,
                               const char *conf);

#ifdef __cplusplus
}
#endif

#endif /* TSS2_TCTI_TABD_H */
