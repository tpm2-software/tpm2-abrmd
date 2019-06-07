/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

#include "common.h"
#include "tss2-tcti-tabrmd.h"
#include "test.h"
#include "test-options.h"
#include "context-util.h"
#include "util.h"

/**
 * This program is a template for integration tests (ones that use the TCTI
 * and the SAPI contexts / API directly). It does nothing more than parsing
 * command line options that allow the caller (likely a script) to specify
 * which TCTI to use for the test.
 */
int
main ()
{
    TSS2_RC rc;
    TSS2_SYS_CONTEXT *sapi_context;
    int ret;
    test_opts_t opts = TEST_OPTS_DEFAULT_INIT;

    get_test_opts_from_env (&opts);
    if (sanity_check_test_opts (&opts) != 0)
        exit (1);
    sapi_context = sapi_init_from_opts (&opts);
    if (sapi_context == NULL)
        exit (1);
    rc = Tss2_Sys_Startup (sapi_context, TPM2_SU_CLEAR);
    if (rc != TSS2_RC_SUCCESS && rc != TPM2_RC_INITIALIZE)
        g_error ("TPM Startup FAILED! Response Code : 0x%x", rc);
    ret = test_invoke (sapi_context);
    sapi_teardown_full (sapi_context);
    /*
     * Use new SAPI & TCTI to clean out TPM after test. Test code may have
     * left either in an unusable state.
     */
    sapi_context = sapi_init_from_opts (&opts);
    if (sapi_context == NULL) {
        exit (1);
    }
    clean_up_all (sapi_context);
    sapi_teardown_full (sapi_context);
    return ret;
}
