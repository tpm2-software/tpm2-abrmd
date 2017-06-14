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
#ifndef TSS2_TCTI_TABD_H
#define TSS2_TCTI_TABD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gio/gio.h>
#include <sapi/tpm20.h>
#include <sapi/tss2_tcti.h>

#define TCTI_TABRMD_DBUS_INTERFACE_DEFAULT "com.intel.tss2.TctiTabrmd"
#define TCTI_TABRMD_DBUS_NAME_DEFAULT      "com.intel.tss2.Tabrmd"
#define TCTI_TABRMD_DBUS_TYPE_DEFAULT      G_BUS_TYPE_SYSTEM

TSS2_RC tss2_tcti_tabrmd_init (TSS2_TCTI_CONTEXT *context, size_t *size);
TSS2_RC tss2_tcti_tabrmd_init_full (TSS2_TCTI_CONTEXT *context,
                                    size_t            *size,
                                    GBusType           bus,
                                    const char        *name);

#ifdef __cplusplus
}
#endif

#endif /* TSS2_TCTI_TABD_H */
