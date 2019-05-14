/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef TPM2_STRUCT_INIT
#define TPM2_STRUCT_INIT

#define TPM2B_STATIC_INIT(type) { .size = sizeof (type) - sizeof (UINT16) }
#define TPM2B_DIGEST_STATIC_INIT         TPM2B_STATIC_INIT(TPM2B_DIGEST)
#define TPM2B_NAME_STATIC_INIT           TPM2B_STATIC_INIT(TPM2B_NAME)
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
