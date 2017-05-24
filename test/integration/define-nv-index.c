//**********************************************************************;
// Copyright (c) 2015, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//**********************************************************************;
#include <sapi/tpm20.h>


UINT16 CopySizedByteBuffer( TPM2B *dest, TPM2B *src )
{
    int i;
    UINT16 rval = 0;

    if( dest != 0 )
    {
        if( src == 0 )
        {
            dest->size = 0;
            rval = 0;
        }
        else
        {
            dest->size = src->size;
            for( i = 0; i < src->size; i++ )
                dest->buffer[i] = src->buffer[i];
            rval = ( sizeof( UINT16 ) + src->size );
        }
    }
    else
    {
        rval = 0;
    }

    return rval;
}

TPM_RC DefineNvIndex( TSS2_SYS_CONTEXT *sysContext,
		      TPMI_RH_PROVISION authHandle,
		      TPMI_SH_AUTH_SESSION cmdAuthSessionHandle,
		      TPM2B_AUTH *auth, TPM2B_DIGEST *authPolicy,
		      TPMI_RH_NV_INDEX nvIndex, TPMI_ALG_HASH nameAlg,
		      TPMA_NV attributes, UINT16 size  )
{
    TPM2B_NV_PUBLIC publicInfo;

    // Command and response session data structures.
    TPMS_AUTH_COMMAND cmdAuth;
    TPMS_AUTH_COMMAND *cmdAuthPtr;
    TSS2_SYS_CMD_AUTHS cmdAuthsArray;
    
    TPMS_AUTH_RESPONSE rspAuth;
    TPMS_AUTH_RESPONSE *rspAuthPtr;
    TSS2_SYS_RSP_AUTHS rspAuthsArray;

    // AUTH CMD
    cmdAuth.sessionHandle = cmdAuthSessionHandle;
    cmdAuth.nonce.t.size = 0;
    cmdAuth.sessionAttributes.val = 0x00000000;
    cmdAuth.hmac.t.size = 0;
    cmdAuthPtr = &cmdAuth;
    cmdAuthsArray.cmdAuthsCount = 1;
    cmdAuthsArray.cmdAuths = &cmdAuthPtr;

    // AUTH RSP
    rspAuthPtr = &rspAuth;
    rspAuthsArray.rspAuthsCount = 1;
    rspAuthsArray.rspAuths = &rspAuthPtr;
    
    // set TPMA_NV_ORDERLY bit
    attributes.TPMA_NV_ORDERLY = 1;

    // PublicInfo
    publicInfo.t.nvPublic.nvIndex = nvIndex;
    publicInfo.t.nvPublic.nameAlg = nameAlg;
    publicInfo.t.nvPublic.attributes = attributes;
    CopySizedByteBuffer( &publicInfo.t.nvPublic.authPolicy.b, &authPolicy->b );
    publicInfo.t.nvPublic.dataSize = size;
    publicInfo.t.size = sizeof( TPMI_RH_NV_INDEX ) +
            sizeof( TPMI_ALG_HASH ) + sizeof( TPMA_NV ) + sizeof( UINT16) +
            sizeof( UINT16 );

    // Create the index
    return Tss2_Sys_NV_DefineSpace( sysContext, authHandle,
				    &cmdAuthsArray,
				    auth,
				    &publicInfo,
				    &rspAuthsArray );
}

TPM_RC UndefineNvIndex( TSS2_SYS_CONTEXT *sysContext,
			TPMI_RH_PROVISION authHandle,
			TPMI_SH_AUTH_SESSION cmdAuthSessionHandle,
			TPMI_RH_NV_INDEX nvIndex)
{

    // Command and response session data structures.
    TPMS_AUTH_COMMAND cmdAuth;
    TPMS_AUTH_COMMAND *cmdAuthPtr;
    TSS2_SYS_CMD_AUTHS cmdAuthsArray;
    
    TPMS_AUTH_RESPONSE rspAuth;
    TPMS_AUTH_RESPONSE *rspAuthPtr;
    TSS2_SYS_RSP_AUTHS rspAuthsArray;

    // AUTH CMD
    cmdAuth.sessionHandle = cmdAuthSessionHandle;
    cmdAuth.nonce.t.size = 0;
    cmdAuth.hmac.t.size = 0;
    cmdAuthPtr = &cmdAuth;
    cmdAuthsArray.cmdAuthsCount = 1;
    cmdAuthsArray.cmdAuths = &cmdAuthPtr;

    // AUTH RSP
    rspAuthPtr = &rspAuth;
    rspAuthsArray.rspAuthsCount = 1;
    rspAuthsArray.rspAuths = &rspAuthPtr;

    return Tss2_Sys_NV_UndefineSpace( sysContext, authHandle,
				      nvIndex,
				      &cmdAuthsArray,
				      &rspAuthsArray );
    
}
