/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>

#include "tss2-tcti-tabrmd.h"
#include "tpm2-struct-init.h"
#include "common.h"
#include "tabrmd-defaults.h"
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
