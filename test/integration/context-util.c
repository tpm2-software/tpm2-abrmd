/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tss2/tss2_tctildr.h>

#include "context-util.h"
#include "common.h"

/*
 * Initialize a TCTI context for the tabrmd. Currently it requires no options.
 */
static TSS2_TCTI_CONTEXT*
tcti_tabrmd_init (const char *conf)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *tcti_ctx;
    size_t size;

    g_debug ("%s: with conf: \"%s\"", __func__, conf);
    rc = Tss2_Tcti_Tabrmd_Init (NULL, &size, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        fprintf (stderr, "Failed to get allocation size for tabrmd TCTI "
                 " context: 0x%" PRIx32 "\n", rc);
        return NULL;
    }
    tcti_ctx = calloc (1, size);
    if (tcti_ctx == NULL) {
        fprintf (stderr, "Allocation for TCTI context failed: %s\n",
                 strerror (errno));
        return NULL;
    }
    rc = Tss2_Tcti_Tabrmd_Init (tcti_ctx, &size, conf);
    if (rc != TSS2_RC_SUCCESS) {
        fprintf (stderr, "Failed to initialize tabrmd TCTI context: "
                 "0x%" PRIx32 "\n", rc);
        free (tcti_ctx);
        return NULL;
    }
    return tcti_ctx;
}
/*
 * Initialize a SAPI context using the TCTI context provided by the caller.
 * This function allocates memory for the SAPI context and returns it to the
 * caller. This memory must be freed by the caller.
 */
static TSS2_SYS_CONTEXT*
sapi_init_from_tcti_ctx (TSS2_TCTI_CONTEXT *tcti_ctx)
{
    TSS2_SYS_CONTEXT *sapi_ctx;
    TSS2_RC rc;
    size_t size;
    TSS2_ABI_VERSION abi_version = SUPPORTED_ABI_VERSION;

    size = Tss2_Sys_GetContextSize (0);
    sapi_ctx = (TSS2_SYS_CONTEXT*)calloc (1, size);
    if (sapi_ctx == NULL) {
        fprintf (stderr,
                 "Failed to allocate 0x%zx bytes for the SAPI context\n",
                 size);
        return NULL;
    }
    rc = Tss2_Sys_Initialize (sapi_ctx, size, tcti_ctx, &abi_version);
    if (rc != TSS2_RC_SUCCESS) {
        fprintf (stderr, "Failed to initialize SAPI context: 0x%x\n", rc);
	free (sapi_ctx);
        return NULL;
    }
    return sapi_ctx;
}
/*
 * Initialize a SAPI context to use a socket TCTI. Get configuration data from
 * the provided structure.
 */
TSS2_SYS_CONTEXT*
sapi_init_from_opts (test_opts_t *options)
{
    TSS2_TCTI_CONTEXT *tcti_ctx = NULL;
    TSS2_SYS_CONTEXT  *sapi_ctx;
    size_t i;

    for (i = 0; i < options->tcti_retries && tcti_ctx == NULL; ++i) {
        tcti_ctx = tcti_init_from_opts (options);
        if (tcti_ctx == NULL) {
            g_debug ("sapi_init_from_opts: tcti_ctx returned NULL on try: %zd", i);
            sleep (1);
        }
    }
    if (tcti_ctx == NULL)
        return NULL;
    sapi_ctx = sapi_init_from_tcti_ctx (tcti_ctx);
    if (sapi_ctx == NULL)
        return NULL;
    return sapi_ctx;
}
/*
 * Initialize a TSS2_TCTI_CONTEXT using whatever TCTI data is in the options
 * structure. This is a mechanism that allows the calling application to be
 * mostly ignorant of which TCTI they're creating / initializing.
 */
TSS2_TCTI_CONTEXT*
tcti_init_from_opts (test_opts_t *options)
{
    TSS2_TCTI_CONTEXT *tcti_ctx = NULL;
    TSS2_RC rc;

    if (options->tcti_filename != NULL) {
        rc = Tss2_TctiLdr_Initialize_Ex (options->tcti_filename,
                                         options->tcti_conf,
                                         &tcti_ctx);
        if (rc != TSS2_RC_SUCCESS || tcti_ctx == NULL)
            return NULL;
        return tcti_ctx;
    } else {
        return tcti_tabrmd_init (options->tcti_conf);
    }
}
/*
 * Teardown and free the resources associated with a SAPI context structure.
 * This includes tearing down the TCTI as well.
 */
void
sapi_teardown_full (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    TSS2_RC rc;

    rc = Tss2_Sys_GetTctiContext (sapi_context, &tcti_context);
    if (rc != TSS2_RC_SUCCESS)
        return;
    Tss2_Sys_Finalize (sapi_context);
    free (sapi_context);
    if (tcti_context) {
        Tss2_TctiLdr_Finalize (&tcti_context);
        /* if tctildr can't finalize, try to do it the old fashioned way */
        if (tcti_context != NULL) {
            Tss2_Tcti_Finalize (tcti_context);
            free (tcti_context);
        }
    }
}
