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
#ifndef TSS2_TABD_H
#define TSS2_TABD_H

#include <tpm20.h>

#define TABRMD_DBUS_INTERFACE                "com.intel.tss2.Tab"
#define TABRMD_DBUS_NAME                     "com.intel.tss2.Tab"
#define TABRMD_DBUS_PATH                     "/com/intel/tss2/Tab"
#define TABRMD_DBUS_METHOD_CREATE_CONNECTION "CreateConnection"
#define TABRMD_DBUS_METHOD_CANCEL            "Cancel"

#define TSS2_TABRMD_INTERNAL_ERROR ((TSS2_RC)(1 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT) + TSS2_RESMGR_ERROR_LEVEL)
#define TSS2_TABRMD_SAPI_INIT      ((TSS2_RC)(2 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT) + TSS2_RESMGR_ERROR_LEVEL)
#define TSS2_TABRMD_OUT_OF_MEMORY  ((TSS2_RC)(3 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT) + TSS2_RESMGR_ERROR_LEVEL)
#define TSS2_TABRMD_BAD_VALUE  ((TSS2_RC)(3 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT) + TSS2_RESMGR_ERROR_LEVEL)

#define TSS2_TABRMD_OBJECT_MEMORY ((TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | \
                                             TPM_RC_OBJECT_MEMORY))

TSS2_RC tss2_tcti_tabrmd_dump_trans_state (TSS2_TCTI_CONTEXT *tcti_context);

#endif /* TSS2_TABD_H */
