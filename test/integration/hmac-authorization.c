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

#include <inttypes.h>

#include "tabrmd.h"
#include "tcti-tabrmd.h"
#include "common.h"

#include "tpm-handle-to-name.h"
#include "concat-sized-byte-buffer.h"
#include "tpm_hash.h"
#include "tpm_hmac.h"
#include "define-nv-index.h"

#define TPM20_INDEX_POLICY_TEST   0x01500020

int
SimpleHmacAuthorizationTest( TSS2_SYS_CONTEXT *sysContext )
{
    int i;
    UINT32 rval;
    int test_return = 1;
    
    UINT8 dataToWrite[] = { 0x00, 0x01, 0x02, 0x03,
			    0x04, 0x05, 0x06, 0x07,
			    0x08, 0x09, 0x0A, 0x0B,
			    0x0C, 0x0D, 0x0E, 0x0F };
    
    // Setup the NV index's authorization value.
    TPM2B_AUTH  nvAuthValue;  // Part 1, Section 19.6.4
    char        sharedSecret[] = "shared secret";
    nvAuthValue.t.size = strlen( sharedSecret );
    for( i = 0; i < nvAuthValue.t.size; i++ )
        nvAuthValue.t.buffer[i] = sharedSecret[i];

    // Set NV index's authorization policy
    // to zero sized policy since we won't be
    // using policy to authorize.
    TPM2B_DIGEST nvAuthPolicy;
    nvAuthPolicy.t.size = 0;

    TPMI_ALG_HASH nvNameAlg = TPM_ALG_SHA256;
	
    // Now set the NV index's attributes:
    // policyRead, authWrite, and platormCreate.
    TPMA_NV nvAttributes;
    nvAttributes.val = 0;
    nvAttributes.TPMA_NV_AUTHREAD = 1;
    nvAttributes.TPMA_NV_AUTHWRITE = 1;
    nvAttributes.TPMA_NV_PLATFORMCREATE = 1;

    // Create the NV index.
    rval = DefineNvIndex( sysContext,
			  TPM_RH_PLATFORM, TPM_RS_PW,
			  &nvAuthValue, &nvAuthPolicy,
			  TPM20_INDEX_POLICY_TEST,
			  nvNameAlg, nvAttributes,
			  sizeof(dataToWrite));

    if (rval != TSS2_RC_SUCCESS) {
	g_warning("%s(%d): DefineNvIndex failed. RC=0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	return 1;
    }
    
    TPMS_AUTH_COMMAND nvCmdAuth;
    TPMS_AUTH_COMMAND *nvCmdAuthPtr;
    TSS2_SYS_CMD_AUTHS nvCmdAuthsArray;

    TPMS_AUTH_RESPONSE nvRspAuth;
    TPMS_AUTH_RESPONSE *nvRspAuthPtr;
    TSS2_SYS_RSP_AUTHS nvRspAuthsArray;

    nvCmdAuth.sessionHandle = 0; // Will come from below

    /*
     * Part 1 - Section 19.6.3.2 Session Nonce Size
     *
     * The minimum size for nonceCaller in TPM2_StartAuthSession() is 16 octets.
     * So, we picked SAH256_DIGEST_SIZE here
     */
    nvCmdAuth.nonce.t.size =SHA256_DIGEST_SIZE;
    memset(nvCmdAuth.nonce.t.buffer, 0xa5, nvCmdAuth.nonce.t.size);
    
    nvCmdAuth.sessionAttributes.val = 0;
    nvCmdAuth.sessionAttributes.continueSession = 1;
    nvCmdAuth.hmac.t.size = 0; // Will fill in below
    nvCmdAuthPtr = &nvCmdAuth;
    nvCmdAuthsArray.cmdAuthsCount = 1;
    nvCmdAuthsArray.cmdAuths = &nvCmdAuthPtr;

    nvRspAuth.nonce.t.size = nvCmdAuth.nonce.t.size;
    nvRspAuthPtr = &nvRspAuth;
    nvRspAuthsArray.rspAuthsCount = 1;
    nvRspAuthsArray.rspAuths = &nvRspAuthPtr;

    TPM2B_ENCRYPTED_SECRET encryptedSalt;    
    encryptedSalt.t.size = 0;

    TPMT_SYM_DEF symmetric;
    symmetric.algorithm = TPM_ALG_NULL;

    TPMI_ALG_HASH sessionHashAlg = TPM_ALG_SHA256;
    
    rval = Tss2_Sys_StartAuthSession ( sysContext,
				       TPM_RH_NULL, // unsalted
				       TPM_RH_NULL, // unbounded
				       NULL, // nvCmdAuthsArray not needed!
				       &nvCmdAuth.nonce,
				       &encryptedSalt, 
				       TPM_SE_HMAC, // sessionType
				       &symmetric,
				       sessionHashAlg,
				       &nvCmdAuth.sessionHandle,
				       &nvRspAuth.nonce,
				       &nvRspAuthsArray);

    if (rval != TSS2_RC_SUCCESS) {
	g_warning("%s(%d): Tss_Sys_StartAuthSession failed. RC=0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    /*
     * Part 1 - Section 19.6 Session-Based Authorizations
     *
     * The secret values of a session are determined by the handles used when the session 
     * is started. The command for starting a session allows selection of up to two object 
     * handles. One handle indicates a TPM object that is used to encrypt a salt value that 
     * is sent when the session is started. A second handle indicates an object containing
     * a shared secret. The salt value and the shared secret are combined with a nonce 
     * provided by the caller to create the session secrets.
     */

    TPM2B_NAME name1;
    rval = TpmHandleToName( sysContext, TPM20_INDEX_POLICY_TEST, &name1 );

    TPM2B_NAME name2;
    rval = TpmHandleToName( sysContext, TPM20_INDEX_POLICY_TEST, &name2 );

    // First call prepare in order to create cpBuffer.
    // This sets the cmd to TPM_CC_NV_Write and params in the sysContext

    // Initialize NV write data.
    TPM2B_MAX_NV_BUFFER nvWriteData;
    nvWriteData.t.size = sizeof( dataToWrite );
    for( i = 0; i < nvWriteData.t.size; i++ ) {
        nvWriteData.t.buffer[i] = dataToWrite[i];
    }

    rval = Tss2_Sys_NV_Write_Prepare( sysContext,
				      TPM20_INDEX_POLICY_TEST,
				      TPM20_INDEX_POLICY_TEST,
				      &nvWriteData, 0 );

    /*******************************************
     *  Compute HMAC to go into NV Write command
     *******************************************/

    /* 
     * From Part 1 - 18.7 Command Parameter Hash (cpHash)
     *
     * cphash = H_sessionAlg ( commandCode { || Name1 {|| Name2 { || Name3 }}} { || parameters})
     */

    // Get NV Write command code
    UINT8 cmdCode[4];
    rval = Tss2_Sys_GetCommandCode( sysContext, &cmdCode );
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("%s(%d): Tss2_Sys_GetCommandCode failed. RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    UINT8 *hashInputPtr;
    TPM2B_MAX_BUFFER hashInput; // Byte stream to be hashed to create pHash
    hashInput.t.size = 0;
    hashInputPtr = &( hashInput.t.buffer[hashInput.t.size] );
    hashInputPtr[0] = cmdCode[3];
    hashInputPtr[1] = cmdCode[2];
    hashInputPtr[2] = cmdCode[1];
    hashInputPtr[3] = cmdCode[0];
    hashInput.t.size += 4;

    rval = ConcatSizedByteBuffer( &hashInput, &( name1.b ) );
    rval = ConcatSizedByteBuffer( &hashInput, &( name2.b ) );

    const uint8_t *params;
    size_t paramsSize = 0;

    Tss2_Sys_GetCpBuffer( sysContext, &paramsSize, &params );

    if( ( hashInput.t.size + paramsSize ) <= sizeof( hashInput.t.buffer ) ) {
        for( i = 0; i < paramsSize; i++ )
            hashInput.t.buffer[hashInput.t.size + i ] = params[i];
        hashInput.t.size += (UINT16)paramsSize;
    } else {
        g_warning("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
	goto cleanupNvIndex;
    }
    
    TPM2B_DIGEST cpHash;
    cpHash.t.size = sizeof(cpHash)-sizeof(UINT16);
    if( hashInput.t.size > sizeof( hashInput.t.buffer ) ) {
	g_warning("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
	goto cleanupNvIndex;
    }
    
    rval = tpm_hash( sysContext, sessionHashAlg,
		     hashInput.t.size, hashInput.t.buffer, &cpHash );
    if( rval != TPM_RC_SUCCESS ) {
	g_warning("%s(%d): cpHash computation error. RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    };
    
    /*
     * From Part 1 - Section 19.6.5 HMAC Computation
     *
     * authHMAC = HMAC ( (sessionKey || authValue),
     *                   (pHash || nonceNewer || nonceOlder
     *                   { || nonceTPM_decrypt} { || nonceTPM_encrypt}
     *                   || sessionAttributes) )
     *
     * but from Part 1 - Section 19.6.8 sessionKeyCreation
     * If both tpmKey and bind are TPM_RH_NULL, then sessionKey is set to an Empty Buffer.
     *
     * therefore, from Part 1 - Section 19.6.9 Unbound and Unsalted Session Key Generation
     *
     * authHMAC = HMAC.sessionAlg(authValue_entity.buffer,
     *                            (pHash || nonceNewer.buffer || nonceOlder*.buffer 
     *                                   || sessionAttributes))
     *
     */

    // New nonce
    memset(nvCmdAuth.nonce.t.buffer, 0x5A, nvCmdAuth.nonce.t.size);
    
    TPM2B_MAX_BUFFER hmacKey;
    TPM2B sessionAttributesByteBuffer;
    sessionAttributesByteBuffer.size = 1;
    sessionAttributesByteBuffer.buffer[0] = nvCmdAuth.sessionAttributes.val;

    hmacKey.t.size = 0;
    rval = ConcatSizedByteBuffer( (TPM2B_MAX_BUFFER *)&hmacKey, &( nvAuthValue.b ) );

    // Create 4 items buffer list
    TPM2B *bufferList[5];    
    bufferList[0] = &cpHash.b;
    bufferList[1] = &( nvCmdAuth.nonce.b ); // nonceNewer
    bufferList[2] = &( nvRspAuth.nonce.b ); // nonceOlder
    bufferList[3] = &( sessionAttributesByteBuffer );
    bufferList[4] = 0; // NULL terminated bufferList

    rval = tpm_hmac (sysContext, nvNameAlg , &hmacKey.b, &( bufferList[0] ), &nvCmdAuth.hmac );
    if( rval != TPM_RC_SUCCESS )
        return rval;

    UINT16 nvOffset = 0;
    rval = Tss2_Sys_NV_Write( sysContext,
			      TPM20_INDEX_POLICY_TEST,
			      TPM20_INDEX_POLICY_TEST,
			      &nvCmdAuthsArray,
			      &nvWriteData, nvOffset,
			      &nvRspAuthsArray );
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("%s(%d): NV Write failed! RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    /****************************************** 
     *  Check HMAC in the response for NV Write
     ******************************************/

    /*
     * authHMAC = HMAC.sessionAlg(authValue_entity.buffer,
     *                            (pHash || nonceNewer.buffer || nonceOlder*.buffer 
     *                                   || sessionAttributes))
     *
     * where pHash = rpHash
     *             = H_sessionAlg(responseCode || commandCode { || parameters }}
     *               (See Part 1 = 18.8 Response Parameter Hash (rpHash))
     */

    // responseCode
    hashInput.t.size = 0;
    hashInputPtr = &( hashInput.t.buffer[hashInput.t.size] );
    hashInputPtr[0] = (rval >> 24) & 0xFF;
    hashInputPtr[1] = (rval >> 16) & 0xFF;
    hashInputPtr[2] = (rval >>  8) & 0xFF;
    hashInputPtr[3] = (rval >>  0) & 0xFF;
    hashInput.t.size += 4;
    
    // commandCode
    hashInputPtr[4] = cmdCode[3];
    hashInputPtr[5] = cmdCode[2];
    hashInputPtr[6] = cmdCode[1];
    hashInputPtr[7] = cmdCode[0];
    hashInput.t.size += 4;

    // parameters
    paramsSize = 0;
    Tss2_Sys_GetRpBuffer( sysContext, &paramsSize, &params );
    if ( ( hashInput.t.size + paramsSize ) <= sizeof( hashInput.t.buffer ) ) {
	for( i = 0; i < paramsSize; i++ )
	    hashInput.t.buffer[hashInput.t.size + i ] = params[i];
	hashInput.t.size += (UINT16)paramsSize;
    } else {
	g_warning("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
	goto cleanupNvIndex;
    }

    TPM2B_DIGEST rpHash;
    rpHash.t.size = sizeof(rpHash)-sizeof(UINT16);
    if( hashInput.t.size > sizeof( hashInput.t.buffer ) ) {
	g_warning("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
	goto cleanupNvIndex;
    }

    rval = tpm_hash( sysContext, sessionHashAlg,
		     hashInput.t.size, hashInput.t.buffer, &rpHash );
    if( rval != TPM_RC_SUCCESS ) {
	g_warning("%s(%d): rpHash computation error. RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    TPM2B_DIGEST computedRspAuth;
    
    // Create 4 items buffer list
    bufferList[0] = &rpHash.b;
    bufferList[1] = &( nvRspAuth.nonce.b ); // nonceNewer
    bufferList[2] = &( nvCmdAuth.nonce.b ); // nonceOlder
    bufferList[3] = &( sessionAttributesByteBuffer );
    bufferList[4] = 0; // NULL terminated bufferList

    rval = tpm_hmac (sysContext, nvNameAlg , &hmacKey.b, &( bufferList[0] ), &computedRspAuth );
    if( rval != TPM_RC_SUCCESS )
        return rval;

    if ((computedRspAuth.b.size != nvRspAuth.hmac.b.size) ||
	(memcmp(computedRspAuth.b.buffer, nvRspAuth.hmac.b.buffer,
		computedRspAuth.b.size))) {

	g_warning("%s(%d): Computed Response hmac different from TPM\n",
		__FUNCTION__, __LINE__);
	goto cleanupNvIndex;
    }
    
    /******************************************
     *  Compute HMAC to go into NV Read command
     ******************************************/

    /* 
     * From Part 1 - 18.7 Command Parameter Hash (cpHash)
     *
     * cphash = H_sessionAlg ( commandCode { || Name1 {|| Name2 { || Name3 }}} { || parameters})
     */

    rval = TpmHandleToName( sysContext, TPM20_INDEX_POLICY_TEST, &name1 );
    rval = TpmHandleToName( sysContext, TPM20_INDEX_POLICY_TEST, &name2 );

    // First call prepare in order to create cpBuffer.
    TPM2B_MAX_NV_BUFFER nvReadData;
    nvReadData.t.size = nvWriteData.t.size;
    rval = Tss2_Sys_NV_Read_Prepare( sysContext,
				     TPM20_INDEX_POLICY_TEST,
				     TPM20_INDEX_POLICY_TEST,
				     nvReadData.t.size, 0 );

    // Get NV Read command code
    rval = Tss2_Sys_GetCommandCode( sysContext, &cmdCode );
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("%s(%d): Tss2_Sys_GetCommandCode failed. RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    // Add command
    hashInput.t.size = 0;
    hashInputPtr = &( hashInput.t.buffer[hashInput.t.size] );
    hashInputPtr[0] = cmdCode[3];
    hashInputPtr[1] = cmdCode[2];
    hashInputPtr[2] = cmdCode[1];
    hashInputPtr[3] = cmdCode[0];
    hashInput.t.size += 4;

    // Add names
    rval = ConcatSizedByteBuffer( &hashInput, &( name1.b ) );
    rval = ConcatSizedByteBuffer( &hashInput, &( name2.b ) );

    // Add parameters
    Tss2_Sys_GetCpBuffer( sysContext, &paramsSize, &params );

    if( ( hashInput.t.size + paramsSize ) <= sizeof( hashInput.t.buffer ) ) {
        for( i = 0; i < paramsSize; i++ )
            hashInput.t.buffer[hashInput.t.size + i ] = params[i];
        hashInput.t.size += (UINT16)paramsSize;
    } else {
        g_warning("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
	goto cleanupNvIndex;
    }
    
    cpHash.t.size = sizeof(cpHash)-sizeof(UINT16);
    if( hashInput.t.size > sizeof( hashInput.t.buffer ) ) {
	g_error("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
    } else {
        rval = tpm_hash( sysContext, sessionHashAlg,
			hashInput.t.size, hashInput.t.buffer, &cpHash );
        if( rval != TPM_RC_SUCCESS ) {
	    g_warning("%s(%d): cpHash computation error. RC = 0x%x\n",
		    __FUNCTION__, __LINE__, rval);
	    goto cleanupNvIndex;
	}
    }

    /*
     * authHMAC = HMAC.sessionAlg(authValue_entity.buffer,
     *                            (pHash || nonceNewer.buffer || nonceOlder*.buffer 
     *                                   || sessionAttributes))
     */
    
    // New nonce
    memset(nvCmdAuth.nonce.t.buffer, 0x4E, nvCmdAuth.nonce.t.size);

    // Update attribute
    nvCmdAuth.sessionAttributes.continueSession = 0;
	
    sessionAttributesByteBuffer.size = 1;
    sessionAttributesByteBuffer.buffer[0] = nvCmdAuth.sessionAttributes.val;

    hmacKey.t.size = 0;
    rval = ConcatSizedByteBuffer( (TPM2B_MAX_BUFFER *)&hmacKey, &( nvAuthValue.b ) );

    bufferList[0] = &cpHash.b;
    bufferList[1] = &( nvCmdAuth.nonce.b ); // nonceNewer
    bufferList[2] = &( nvRspAuth.nonce.b ); // nonceOlder
    bufferList[3] = &( sessionAttributesByteBuffer );
    bufferList[4] = 0; // NULL terminated bufferList

    rval = tpm_hmac (sysContext, nvNameAlg , &hmacKey.b, &( bufferList[0] ), &nvCmdAuth.hmac );
    if( rval != TPM_RC_SUCCESS ) {
	g_warning("%s(%d): HMAC caluation failed. RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    nvOffset = 0;
    rval = Tss2_Sys_NV_Read( sysContext,
			     TPM20_INDEX_POLICY_TEST,
			     TPM20_INDEX_POLICY_TEST,
			     &nvCmdAuthsArray,
			     nvReadData.t.size, nvOffset,
			     &nvReadData, &nvRspAuthsArray );
    if (rval != TSS2_RC_SUCCESS) {
	g_warning("%s(%d): NV Read failed! RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    /***************************************** 
     *  Check HMAC in the response for NV Read
     *****************************************/

        // responseCode
    hashInput.t.size = 0;
    hashInputPtr = &( hashInput.t.buffer[hashInput.t.size] );
    hashInputPtr[0] = (rval >> 24) & 0xFF;
    hashInputPtr[1] = (rval >> 16) & 0xFF;
    hashInputPtr[2] = (rval >>  8) & 0xFF;
    hashInputPtr[3] = (rval >>  0) & 0xFF;
    hashInput.t.size += 4;
    
    // commandCode
    hashInputPtr[4] = cmdCode[3];
    hashInputPtr[5] = cmdCode[2];
    hashInputPtr[6] = cmdCode[1];
    hashInputPtr[7] = cmdCode[0];
    hashInput.t.size += 4;

    // parameters
    paramsSize = 0;
    Tss2_Sys_GetRpBuffer( sysContext, &paramsSize, &params );
    if ( ( hashInput.t.size + paramsSize ) <= sizeof( hashInput.t.buffer ) ) {
	for( i = 0; i < paramsSize; i++ )
	    hashInput.t.buffer[hashInput.t.size + i ] = params[i];
	hashInput.t.size += (UINT16)paramsSize;
    } else {
	g_warning("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
	goto cleanupNvIndex;
    }

    rpHash.t.size = sizeof(rpHash)-sizeof(UINT16);
    if( hashInput.t.size > sizeof( hashInput.t.buffer ) ) {
	g_error("%s(%d): Insufficient buffer\n", __FUNCTION__, __LINE__);
    } else {
        rval = tpm_hash( sysContext, sessionHashAlg,
			 hashInput.t.size, hashInput.t.buffer, &rpHash );
        if( rval != TPM_RC_SUCCESS ) {
	    g_warning("%s(%d): rpHash computation error. RC = 0x%x\n",
		      __FUNCTION__, __LINE__, rval);
	    goto cleanupNvIndex;
	}
    }

    // Create 4 items buffer list
    bufferList[0] = &rpHash.b;
    bufferList[1] = &( nvRspAuth.nonce.b ); // nonceNewer
    bufferList[2] = &( nvCmdAuth.nonce.b ); // nonceOlder
    bufferList[3] = &( sessionAttributesByteBuffer );
    bufferList[4] = 0; // NULL terminated bufferList

    rval = tpm_hmac (sysContext, nvNameAlg , &hmacKey.b, &( bufferList[0] ), &computedRspAuth );
    if( rval != TPM_RC_SUCCESS ) {
	g_warning("%s(%d): HMAC caluation failed. RC = 0x%x\n",
		  __FUNCTION__, __LINE__, rval);
	goto cleanupNvIndex;
    }

    if ((computedRspAuth.b.size != nvRspAuth.hmac.b.size) ||
	(memcmp(computedRspAuth.b.buffer, nvRspAuth.hmac.b.buffer,
		computedRspAuth.b.size))) {

	g_warning("%s(%d): Computed Response hmac different from TPM\n",
		__FUNCTION__, __LINE__);

	goto cleanupNvIndex;
    }

    if ((nvReadData.t.size != nvWriteData.t.size) ||
	memcmp(nvReadData.t.buffer, nvWriteData.t.buffer, nvWriteData.t.size)) {

	g_warning("%s(%d): Data read back from NV different from what was written\n",
		__FUNCTION__, __LINE__);

	goto cleanupNvIndex;
    }

    test_return = 0;
    
cleanupNvIndex:

    rval = UndefineNvIndex( sysContext,
			    TPM_RH_PLATFORM, TPM_RS_PW,
			    TPM20_INDEX_POLICY_TEST );

    if (rval != TPM_RC_SUCCESS) {
	g_warning("%s(%d): NV Undefine failed. RC = 0x%x\n", __FUNCTION__, __LINE__, rval);
	return 1;
    }
    return test_return;
}

int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
  return SimpleHmacAuthorizationTest( sapi_context );
}
