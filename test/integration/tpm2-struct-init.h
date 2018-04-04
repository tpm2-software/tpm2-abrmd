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
#ifndef TPM2_STRUCT_INIT
#define TPM2_STRUCT_INIT

#define TPM2B_STATIC_INIT(type) { .size = sizeof (type) - sizeof (UINT16) }
#define TPM2B_DIGEST_STATIC_INIT         TPM2B_STATIC_INIT(TPM2B_DIGEST)
#define TPM2B_NAME_STATIC_INIT           TPM2B_STATIC_INIT(TPM2B_NAME)
#define TPM2B_PUBLIC_STATIC_INIT         TPM2B_STATIC_INIT(TPM2B_PUBLIC)
#define TPM2B_PRIVATE_STATIC_INIT        TPM2B_STATIC_INIT(TPM2B_PRIVATE)

#define TPM2B_ZERO_INIT(type)   { .size = 0 }
#define TPM2B_DIGEST_ZERO_INIT           TPM2B_ZERO_INIT(TPM2B_DIGEST)
#define TPM2B_NAME_ZERO_INIT             TPM2B_ZERO_INIT(TPM2B_NAME)
#define TPM2B_PUBLIC_ZERO_INIT           TPM2B_ZERO_INIT(TPM2B_PUBLIC)
#define TPM2B_PRIVATE_ZERO_INIT          TPM2B_ZERO_INIT(TPM2B_PRIVATE)
#define TPM2B_DATA_ZERO_INIT             TPM2B_ZERO_INIT(TPM2B_DATA)
#define TPM2B_CONTEXT_DATA_ZERO_INIT     TPM2B_ZERO_INIT(TPM2B_CONTEXT_DATA)
#define TPM2B_CREATION_DATA_ZERO_INIT    TPM2B_ZERO_INIT(TPM2B_CREATION_DATA)
#define TPM2B_SENSITIVE_DATA_ZERO_INIT   TPM2B_ZERO_INIT(TPM2B_SENSITIVE_DATA)
#define TPM2B_SENSITIVE_CREATE_ZERO_INIT TPM2B_ZERO_INIT(TPM2B_SENSITIVE_CREATE)
#define TPM2B_ENCRYPTED_SECRET_ZERO_INIT TPM2B_ZERO_INIT(TPM2B_ENCRYPTED_SECRET)

#define TPMS_CONTEXT_ZERO_INIT { .sequence = 0, .savedHandle = 0, \
    .hierarchy = 0, .contextBlob = TPM2B_CONTEXT_DATA_ZERO_INIT \
}
#define TPMT_TK_CREATION_ZERO_INIT { .tag = 0, .hierarchy = 0, \
    TPM2B_DIGEST_ZERO_INIT \
}
#define TPMS_SENSITIVE_ZERO_INIT { \
    .userAuth = 0, .data = TPM2B_SENSITIVE_DATA_ZERO_INIT \
} 
#define TPMS_PCR_SELECTION_ZERO_INIT { .hash = 0, \
    .sizeofSelect = 0, .pcrSelect = 0 \
}
#define TPML_PCR_SELECTION_ZERO_INIT { .count = 0, \
    .pcrSelections = { TPMS_PCR_SELECTION_ZERO_INIT, \
    TPMS_PCR_SELECTION_ZERO_INIT, TPMS_PCR_SELECTION_ZERO_INIT } \
}

#define TPMS_CAPABILITY_DATA_ZERO_INIT { 0, { 0, { \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } }

#endif /* TPM2_STRUCT_INIT */
