/*
 * Copyright (c) 2018, Intel Corporation
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
#include <dlfcn.h>
#include <glib.h>
#include <inttypes.h>
#include <sapi/tpm20.h>

#include "tabrmd.h"

TSS2_RC
tcti_util_discover_info (const char *filename,
                         const TSS2_TCTI_INFO **info)
{
    TSS2_TCTI_INFO_FUNC info_func;
    void *tcti_dl_handle;

    g_debug ("%s", __func__);
    tcti_dl_handle = dlopen (filename, RTLD_LAZY);
    if (tcti_dl_handle == NULL) {
        g_warning ("failed to dlopen file %s: %s", filename, dlerror ());
        return TSS2_RESMGR_RC_BAD_VALUE;
    }
    info_func = dlsym (tcti_dl_handle, TSS2_TCTI_INFO_SYMBOL);
    if (info_func == NULL) {
        g_warning ("Failed to get reference to symbol: %s", dlerror ());
#if !defined (DISABLE_DLCLOSE)
        dlclose (tcti_dl_handle);
#endif
        return TSS2_RESMGR_RC_BAD_VALUE;
    }
    *info = info_func ();
#if !defined (DISABLE_DLCLOSE)
    dlclose (tcti_dl_handle);
#endif
    return TSS2_RC_SUCCESS;
}
TSS2_RC
tcti_util_dynamic_init (const TSS2_TCTI_INFO *info,
                        const char *conf,
                        TSS2_TCTI_CONTEXT **context)
{
    TSS2_RC        rc       = TSS2_RC_SUCCESS;
    size_t         ctx_size;

    g_debug ("%s", __func__);
    if (info == NULL || info->init == NULL) {
        g_warning ("%s: TCTI_INFO structure or init function pointer is NULL, "
                   "cannot initialize context.", __func__);
        return TSS2_RESMGR_RC_BAD_VALUE;
    }
    rc = info->init (NULL, &ctx_size, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to get size for device TCTI contexxt structure: "
                   "0x%x", rc);
        goto out;
    }
    *context = g_malloc0 (ctx_size);
    if (*context == NULL) {
        g_warning ("failed to allocate memory");
        rc = TSS2_RESMGR_RC_INTERNAL_ERROR;
        goto out;
    }
    rc = info->init (*context, &ctx_size, conf);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to initialize device TCTI context: 0x%x", rc);
        g_free (*context);
        *context = NULL;
    }
out:
    return rc;
}
