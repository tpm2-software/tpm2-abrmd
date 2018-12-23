/* SPDX-License-Identifier: BSD-2 */
/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TABRMD_TCTI_UTIL_H
#define TABRMD_TCTI_UTIL_H

#include <tss2/tss2_tcti.h>

TSS2_RC
tcti_util_discover_info (const char *filename,
                         const TSS2_TCTI_INFO **info,
                         void **tcti_dl_handle);
TSS2_RC
tcti_util_dynamic_init (const TSS2_TCTI_INFO *info,
                        const char *conf,
                        TSS2_TCTI_CONTEXT **context);

#endif /* TABRMD_TCTI_UTIL_H */
