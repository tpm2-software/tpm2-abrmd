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
CreatePasswordTestNV (TSS2_SYS_CONTEXT *sapi_context, TPMI_RH_NV_INDEX nvIndex, char * password)
{
    TSS2_RC rval;
    int i;
    TPMS_AUTH_COMMAND sessionData;
    TPMS_AUTH_RESPONSE sessionDataOut;
    TSS2_SYS_CMD_AUTHS sessionsData;
    TSS2_SYS_RSP_AUTHS sessionsDataOut;
    TPM2B_NV_PUBLIC publicInfo;
    TPM2B_AUTH  nvAuth;

    TPMS_AUTH_COMMAND *sessionDataArray[1];
    TPMS_AUTH_RESPONSE *sessionDataOutArray[1];

    sessionDataArray[0] = &sessionData;
    sessionDataOutArray[0] = &sessionDataOut;

    sessionsDataOut.rspAuths = &sessionDataOutArray[0];
    sessionsData.cmdAuths = &sessionDataArray[0];

    sessionData.sessionHandle = TPM_RS_PW;

    // Init nonce.
    sessionData.nonce.t.size = 0;

    // init hmac
    sessionData.hmac.t.size = 0;

    // Init session attributes
    *( (UINT8 *)((void *)&sessionData.sessionAttributes ) ) = 0;

    sessionsDataOut.rspAuthsCount = 1;

    sessionsData.cmdAuthsCount = 1;
    sessionsData.cmdAuths[0] = &sessionData;

    nvAuth.t.size = strlen( password );
    for( i = 0; i < nvAuth.t.size; i++ ) {
        nvAuth.t.buffer[i] = password[i];
    }

    publicInfo.t.size = sizeof( TPMI_RH_NV_INDEX ) +
	sizeof( TPMI_ALG_HASH ) + sizeof( TPMA_NV ) + sizeof( UINT16) +
	sizeof( UINT16 );
    publicInfo.t.nvPublic.nvIndex = nvIndex;
    publicInfo.t.nvPublic.nameAlg = TPM_ALG_SHA1;

    // First zero out attributes.
    *(UINT32 *)&( publicInfo.t.nvPublic.attributes ) = 0;

    // Now set the attributes.
    publicInfo.t.nvPublic.attributes.TPMA_NV_AUTHREAD = 1;
    publicInfo.t.nvPublic.attributes.TPMA_NV_AUTHWRITE = 1;
    publicInfo.t.nvPublic.attributes.TPMA_NV_PLATFORMCREATE = 1;
    publicInfo.t.nvPublic.attributes.TPMA_NV_ORDERLY = 1;
    publicInfo.t.nvPublic.authPolicy.t.size = 0;
    publicInfo.t.nvPublic.dataSize = 32;

    rval = Tss2_Sys_NV_DefineSpace( sapi_context, TPM_RH_PLATFORM,
            &sessionsData, &nvAuth, &publicInfo, &sessionsDataOut );
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
    int i;

    // Authorization structure for command.
    TPMS_AUTH_COMMAND sessionData;

    // Authorization structure for response.
    TPMS_AUTH_RESPONSE sessionDataOut;

    // Create and init authorization area for command:
    // only 1 authorization area.
    TPMS_AUTH_COMMAND *sessionDataArray[1] = { &sessionData };

    // Create authorization area for response:
    // only 1 authorization area.
    TPMS_AUTH_RESPONSE *sessionDataOutArray[1] = { &sessionDataOut };

    // Authorization array for command (only has one auth structure).
    TSS2_SYS_CMD_AUTHS sessionsData = { 1, &sessionDataArray[0] };

    // Authorization array for response (only has one auth structure).
    TSS2_SYS_RSP_AUTHS sessionsDataOut = { 1, &sessionDataOutArray[0] };
    TPM2B_MAX_NV_BUFFER nvWriteData;

    char password[] = "test password";
	
    // Create an NV index that will use password
    // authorizations the password will be
    // "test password".
    if (CreatePasswordTestNV( sapi_context,  TPM20_INDEX_PASSWORD_TEST, password ) != 1) {
	// Error message already printed in CreatePasswordTestNV()
	return 0;
    }

    //
    // Initialize the command authorization area.
    //

    // Init sessionHandle, nonce, session
    // attributes, and hmac (password).
    sessionData.sessionHandle = TPM_RS_PW;
    // Set zero sized nonce.
    sessionData.nonce.t.size = 0;
    // sessionAttributes is a bit field.  To initialize
    // it to 0, cast to a pointer to UINT8 and
    // write 0 to that pointer.
    *( (UINT8 *)&sessionData.sessionAttributes ) = 0;

    // Init password (HMAC field in authorization structure).
    sessionData.hmac.t.size = strlen( password );
    memcpy( &( sessionData.hmac.t.buffer[0] ),
            &( password[0] ), sessionData.hmac.t.size );

    // Initialize write data.
    nvWriteData.t.size = 4;
    for( i = 0; i < nvWriteData.t.size; i++ ){
        nvWriteData.t.buffer[i] = 0xff - i;
    }

    // Attempt write with the correct password.
    // It should pass.
    rval = Tss2_Sys_NV_Write( sapi_context,
            TPM20_INDEX_PASSWORD_TEST,
            TPM20_INDEX_PASSWORD_TEST,
            &sessionsData, &nvWriteData, 0,
            &sessionsDataOut );
    // Check that the function passed as
    // expected.  Otherwise, exit.
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("Failed to write in NV with correct password. RC = 0x%x", rval);
	return 0;
    }

    // Alter the password so it's incorrect.
    sessionData.hmac.t.buffer[4] = 0xff;
    rval = Tss2_Sys_NV_Write( sapi_context,
            TPM20_INDEX_PASSWORD_TEST,
            TPM20_INDEX_PASSWORD_TEST,
            &sessionsData, &nvWriteData, 0,
            &sessionsDataOut );
    // Check that the function failed as expected,
    // since password was incorrect.  If wrong
    // response code received, exit.
    if (rval != (TPM_RC_S + TPM_RC_1 + TPM_RC_AUTH_FAIL)) {
	g_warning("Unexpected error while writing NV with incorrect password. RC = 0x%x", rval);
	return 0;
    }

    // Change hmac to null one, since null auth is
    // used to undefine the index.
    sessionData.hmac.t.size = 0;

    // Now undefine the index.
    rval = Tss2_Sys_NV_UndefineSpace( sapi_context, TPM_RH_PLATFORM,
            TPM20_INDEX_PASSWORD_TEST, &sessionsData, 0 );
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("Failed to undefine NV index. RC = 0x%x", rval);
	return 0;
    }
    
  return 1;
}
