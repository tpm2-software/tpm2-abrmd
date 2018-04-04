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
#include <stdbool.h>
#include <stdlib.h>

#include "common.h"
#include "tss2-tcti-tabrmd.h"
#include "test.h"
#include "test-options.h"
#include "context-util.h"

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
