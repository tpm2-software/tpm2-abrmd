/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */

/*
 * This is a port of PasswordTest() from TSS's test/tpmclient/tpmclient
 */

#include <glib.h>
#include <inttypes.h>
#include <string.h>

#include "tabrmd.h"
#include "tss2-tcti-tabrmd.h"
#include "common.h"

#define TPM20_INDEX_PASSWORD_TEST       0x01500020

/*
 * Helper function to define NV index with password authorization
 */
int
CreatePasswordTestNV (TSS2_SYS_CONTEXT   *sapi_context,
                      TPMI_RH_NV_INDEX    nvIndex,
                      char               *password)
{
    TSS2_RC rval;
    int i;
    TPM2B_NV_PUBLIC publicInfo = {
        .nvPublic = {
            .attributes = TPMA_NV_AUTHREAD | TPMA_NV_AUTHWRITE | \
                TPMA_NV_ORDERLY,
            .dataSize = strlen (password),
            .nameAlg = TPM2_ALG_SHA1,
            .nvIndex = nvIndex,
        }
    };
    TPM2B_AUTH  nvAuth;
    TSS2L_SYS_AUTH_COMMAND cmd_auths = {
        .count = 1,
        .auths = {{
            .sessionHandle = TPM2_RS_PW,
        }}
    };
    TSS2L_SYS_AUTH_RESPONSE rsp_auths;

    nvAuth.size = strlen (password);
    for (i = 0; i < nvAuth.size; i++) {
        nvAuth.buffer[i] = password[i];
    }

    rval = Tss2_Sys_NV_DefineSpace (sapi_context,
                                    TPM2_RH_OWNER,
                                    &cmd_auths,
                                    &nvAuth,
                                    &publicInfo,
                                    &rsp_auths);
    if (rval != TSS2_RC_SUCCESS) {
        g_warning("Failed to define NV space with password authorization. RC = 0x%x", rval);
        return 0;
    }

    return 1;
}

int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{

    TSS2_RC rval;
    int i, ret;
    TPM2B_MAX_NV_BUFFER nvWriteData;
#define PASSWORD "test password"
    TSS2L_SYS_AUTH_COMMAND cmd_auths = {
        .count = 1,
        .auths = {{
            .sessionHandle = TPM2_RS_PW,
            .hmac.buffer = {PASSWORD},
            .hmac.size = strlen(PASSWORD),
        }}
    };

    TSS2L_SYS_AUTH_RESPONSE rsp_auths;

    /*
     * Create an NV index that will use password
     * authorizations the password will be
     * "test password".
     */
    ret = CreatePasswordTestNV (sapi_context,
                                TPM20_INDEX_PASSWORD_TEST,
                                PASSWORD);
    if (ret != 1) {
	    /* Error message already printed in CreatePasswordTestNV() */
        return 1;
    }

    /* Initialize write data. */
    nvWriteData.size = 4;
    for (i = 0; i < nvWriteData.size; i++) {
        nvWriteData.buffer[i] = 0xff - i;
    }

    /* Attempt write with the correct password. It should pass. */
    rval = TSS2_RETRY_EXP (Tss2_Sys_NV_Write (sapi_context,
                              TPM20_INDEX_PASSWORD_TEST,
                              TPM20_INDEX_PASSWORD_TEST,
                              &cmd_auths,
                              &nvWriteData,
                              0,
                              &rsp_auths));
    /* Check that the function passed as expected.  Otherwise, exit. */
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("Failed to write in NV with correct password. RC = 0x%x", rval);
	return 1;
    }

    /* Alter the password so it's incorrect. */
    cmd_auths.auths[0].hmac.buffer[4] = 0xff;
    rval = Tss2_Sys_NV_Write (sapi_context,
                              TPM20_INDEX_PASSWORD_TEST,
                              TPM20_INDEX_PASSWORD_TEST,
                              &cmd_auths,
                              &nvWriteData,
                              0,
                              &rsp_auths);
    /*
     * Check that the function failed as expected,
     * since password was incorrect.  If wrong
     * response code received, exit.
     */
    if (rval != (TPM2_RC_S + TPM2_RC_1 + TPM2_RC_AUTH_FAIL)) {
	g_warning("Unexpected error while writing NV with incorrect password. RC = 0x%x", rval);
	return 1;
    }

    /*
     * Change hmac to null one, since null auth is used to undefine the index.
     */
    cmd_auths.auths[0].hmac.size = 0;

    /* Now undefine the index. */
    rval = Tss2_Sys_NV_UndefineSpace (sapi_context,
                                      TPM2_RH_OWNER,
                                      TPM20_INDEX_PASSWORD_TEST,
                                      &cmd_auths,
                                      0);
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("Failed to undefine NV index. RC = 0x%x", rval);
	return 1;
    }

    return 0;
}
