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
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>

#include "tss2-tcti-tabrmd.h"
#include "tpm2-struct-init.h"
#include "common.h"
#include "tabrmd.h"

#define NUM_KEYS (TABRMD_TRANSIENT_MAX_DEFAULT + 1)
#define ENV_NUM_KEYS "TABRMD_TEST_NUM_KEYS"

/*
 * This is a test program that creates and loads a configurable number of
 * transient objects in the NULL hierarchy. The number of keys created
 * under the NULL primary key can be provided as a base 10 integer on
 * the command line. This is the only parameter the program takes.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TPM2_HANDLE        parent_handle, out_handle;
    TPM2B_PRIVATE      out_private = TPM2B_PRIVATE_STATIC_INIT;
    TPM2B_PUBLIC       out_public  = TPM2B_PUBLIC_ZERO_INIT;
    TSS2_RC            rc = TSS2_RC_SUCCESS;
    char              *env_str = NULL, *end_ptr = NULL;
    uint8_t            loops = NUM_KEYS;

    env_str = getenv (ENV_NUM_KEYS);
    if (env_str != NULL)
        loops = strtol (env_str, &end_ptr, 10);

    g_print ("%s: %d", ENV_NUM_KEYS, loops);
    rc = create_primary (sapi_context, &parent_handle);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("Failed to create primary key: 0x%" PRIx32, rc);
    g_print ("primary handle: 0x%" PRIx32 "\n", parent_handle);
    guint i;
    for (i = 0; i < loops; ++i) {
        rc = create_key (sapi_context,
                         parent_handle,
                         &out_private,
                         &out_public);
        if (rc != TSS2_RC_SUCCESS) {
            /* creation of the key won't fail, loading it will */
            g_critical ("Failed to create_key number %d: 0x%" PRIx32, i, rc);
            return -1;
        }
        rc = load_key (sapi_context,
                       parent_handle,
                       &out_handle,
                       &out_private,
                       &out_public);
        if (rc != TSS2_RC_SUCCESS) {
            g_info ("Failed to load_key number %d: 0x%" PRIx32, i, rc);
            if (rc == TSS2_RESMGR_RC_OBJECT_MEMORY &&
                i == TABRMD_TRANSIENT_MAX_DEFAULT) {
                return 0;
            }
            return rc;
        }
        out_public.size = 0;
    }
    /*
     * tpm2-abrmd running with default max-transient should have failed by now.
     * If we've made it this far the daemon isn't enforcing the upper bound
     * correctly.
     */
    return -1;
}
