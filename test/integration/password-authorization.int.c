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

/*
 * This is a port of PasswordTest() from TSS's test/tpmclient/tpmclient
 */

#include <glib.h>
#include <inttypes.h>

#include "tabrmd.h"
#include "tcti-tabrmd.h"
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
    TPM2B_NV_PUBLIC publicInfo;
    TPM2B_AUTH  nvAuth;

    TPMS_AUTH_COMMAND auth_command = {
        .sessionHandle = TPM2_RS_PW,
    };
    TPMS_AUTH_COMMAND *auth_command_array[1] = { &auth_command, };
    TSS2_SYS_CMD_AUTHS cmd_auths = {
        .cmdAuthsCount = 1,
        .cmdAuths      = auth_command_array,
    };
    TPMS_AUTH_RESPONSE  auth_response = { 0 };
    TPMS_AUTH_RESPONSE *auth_response_array[1] = { &auth_response };
    TSS2_SYS_RSP_AUTHS  rsp_auths = {
        .rspAuths      = auth_response_array,
        .rspAuthsCount = 1
    };

    nvAuth.size = strlen (password);
    for (i = 0; i < nvAuth.size; i++) {
        nvAuth.buffer[i] = password[i];
    }

    publicInfo.size = sizeof (TPMI_RH_NV_INDEX) + sizeof (TPMI_ALG_HASH) +
        sizeof (TPMA_NV) + sizeof (UINT16) + sizeof (UINT16);
    publicInfo.nvPublic.nvIndex = nvIndex;
    publicInfo.nvPublic.nameAlg = TPM2_ALG_SHA1;

    // First zero out attributes.
    *(UINT32 *)&( publicInfo.nvPublic.attributes ) = 0;

    // Now set the attributes.
    publicInfo.nvPublic.attributes.TPMA_NV_AUTHREAD = 1;
    publicInfo.nvPublic.attributes.TPMA_NV_AUTHWRITE = 1;
    publicInfo.nvPublic.attributes.TPMA_NV_ORDERLY = 1;
    publicInfo.nvPublic.authPolicy.size = 0;
    publicInfo.nvPublic.dataSize = 32;

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

    char password[] = "test password";

    TPMS_AUTH_COMMAND auth_command = {
        .sessionHandle = TPM2_RS_PW,
    };
    TPMS_AUTH_COMMAND *auth_command_array[1] = { &auth_command, };
    TSS2_SYS_CMD_AUTHS cmd_auths = {
        .cmdAuthsCount = 1,
        .cmdAuths      = auth_command_array,
    };
    TPMS_AUTH_RESPONSE  auth_response = { 0 };
    TPMS_AUTH_RESPONSE *auth_response_array[1] = { &auth_response };
    TSS2_SYS_RSP_AUTHS  rsp_auths = {
        .rspAuths      = auth_response_array,
        .rspAuthsCount = 1
    };

    /*
     * Create an NV index that will use password
     * authorizations the password will be
     * "test password".
     */
    ret = CreatePasswordTestNV (sapi_context,
                                TPM20_INDEX_PASSWORD_TEST,
                                password);
    if (ret != 1) {
	// Error message already printed in CreatePasswordTestNV()
        return 1;
    }

    // Init password (HMAC field in authorization structure).
    auth_command.hmac.size = strlen (password);
    memcpy (auth_command.hmac.buffer, password, strlen (password));

    // Initialize write data.
    nvWriteData.size = 4;
    for (i = 0; i < nvWriteData.size; i++) {
        nvWriteData.buffer[i] = 0xff - i;
    }

    // Attempt write with the correct password. It should pass.
    rval = TSS2_RETRY_EXP (Tss2_Sys_NV_Write (sapi_context,
                              TPM20_INDEX_PASSWORD_TEST,
                              TPM20_INDEX_PASSWORD_TEST,
                              &cmd_auths,
                              &nvWriteData,
                              0,
                              &rsp_auths));
    // Check that the function passed as expected.  Otherwise, exit.
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("Failed to write in NV with correct password. RC = 0x%x", rval);
	return 1;
    }

    // Alter the password so it's incorrect.
    auth_command.hmac.buffer[4] = 0xff;
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

    // Change hmac to null one, since null auth is used to undefine the index.
    auth_command.hmac.size = 0;

    // Now undefine the index.
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
