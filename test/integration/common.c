/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
/*
 * These are common functions used by the integration tests.
 */
#include <glib.h>
#include <stdlib.h>

#include "common.h"
#include "tss2-tcti-tabrmd.h"
#include "tpm2-struct-init.h"

TSS2_RC
get_context_gap_max (TSS2_SYS_CONTEXT *sys_context,
                     UINT32 *value)
{
    TPMI_YES_NO more_data = TPM2_NO;
    TPMS_CAPABILITY_DATA capability_data;
    TSS2_RC rc;

    g_debug ("%s: GetCapability", __func__);
    rc = Tss2_Sys_GetCapability (sys_context,
                                 NULL,
                                 TPM2_CAP_TPM_PROPERTIES,
                                 TPM2_PT_CONTEXT_GAP_MAX,
                                 1,
                                 &more_data,
                                 &capability_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("%s: GetCapability failed with RC: 0x%" PRIx32, __func__, rc);
    }
    if (capability_data.capability != TPM2_CAP_TPM_PROPERTIES) {
        g_error ("%s: capability returned is unexpected value: 0x%" PRIx32,
                 __func__, capability_data.capability);
    }
    if (capability_data.data.tpmProperties.count != 1) {
        g_error ("%s: capability returned is unexpected count: 0x%" PRIx32,
                 __func__, capability_data.data.tpmProperties.count);
    }
    if (capability_data.data.tpmProperties.tpmProperty[0].property != TPM2_PT_CONTEXT_GAP_MAX) {
        g_error ("%s: capability returned wrong property: 0x%" PRIx32,
                 __func__, capability_data.data.tpmProperties.tpmProperty[0].property);
    }
    g_debug ("%s: TPM2_PT_CONTEXT_GAP_MAX value: 0x%" PRIx32, __func__,
             capability_data.data.tpmProperties.tpmProperty[0].value);
    *value = capability_data.data.tpmProperties.tpmProperty[0].value;
    return rc;
}
/*
 */
TSS2_RC
create_primary (TSS2_SYS_CONTEXT *sapi_context,
                TPM2_HANDLE       *handle)
{
    TSS2_RC rc;
    TPM2B_SENSITIVE_CREATE in_sensitive    = TPM2B_SENSITIVE_CREATE_ZERO_INIT;
    TPM2B_PUBLIC           in_public       = TPM2B_PUBLIC_ZERO_INIT;
    TPM2B_DATA             outside_info    = TPM2B_DATA_ZERO_INIT;
    TPML_PCR_SELECTION     creation_pcr    = TPML_PCR_SELECTION_ZERO_INIT;
    TPM2B_PUBLIC           out_public      = TPM2B_PUBLIC_ZERO_INIT;
    TPM2B_CREATION_DATA    creation_data   = TPM2B_CREATION_DATA_ZERO_INIT;
    TPM2B_DIGEST           creation_digest = TPM2B_DIGEST_STATIC_INIT;
    TPMT_TK_CREATION       creation_ticket = TPMT_TK_CREATION_ZERO_INIT;
    TPM2B_NAME             name            = TPM2B_NAME_STATIC_INIT;
    /* command auth stuff */
    TSS2L_SYS_AUTH_COMMAND cmd_auths = {
        .count = 1,
        .auths = {{
            .sessionHandle = TPM2_RS_PW,
        }}
    };

    /* prepare in_sensitive */
    in_sensitive.size = in_sensitive.sensitive.userAuth.size + 2;
    /* prepare in_public TPMT_PUBLIC */
    /* TPMI_ALG_PUBLIC / publicArea */
    in_public.publicArea.type    = TPM2_ALG_RSA;
    /* TPMI_ALG_HASH / nameAlg */
    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
    /* TPMA_OBJECT / objectAttributes */
    in_public.publicArea.objectAttributes = \
        TPMA_OBJECT_FIXEDTPM            | TPMA_OBJECT_FIXEDPARENT  | \
        TPMA_OBJECT_SENSITIVEDATAORIGIN | TPMA_OBJECT_USERWITHAUTH | \
        TPMA_OBJECT_RESTRICTED          | TPMA_OBJECT_DECRYPT;
    /* TPM2B_DIGEST / authPolicy */
    in_public.publicArea.authPolicy.size = 0;
    /* TPMU_PUBLIC_PARAMS / parameters: key type is TPM2_ALG_RSA, set parameters accordingly */
    in_public.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM2_ALG_AES;
    in_public.publicArea.parameters.rsaDetail.symmetric.keyBits.aes = 128;
    in_public.publicArea.parameters.rsaDetail.symmetric.mode.aes = TPM2_ALG_CFB;
    in_public.publicArea.parameters.rsaDetail.scheme.scheme = TPM2_ALG_NULL;
    in_public.publicArea.parameters.rsaDetail.keyBits = 2048;
    in_public.publicArea.parameters.rsaDetail.exponent = 0;
    /* TPMU_PUBLIC_ID / unique */
    in_public.publicArea.unique.rsa.size = 0;

    rc = TSS2_RETRY_EXP (Tss2_Sys_CreatePrimary (
        sapi_context,
        TPM2_RH_NULL,      /* in: hierarchy */
        &cmd_auths,       /* in: in sessions / auths */
        &in_sensitive,    /* in: sensitive data? */
        &in_public,       /* in: key template */
        &outside_info,    /* in: data that will be included in the creation data */
        &creation_pcr,    /* in: PCR data used in creation data */
        handle,           /* out: handle for loaded object */
        &out_public,      /* out: public portion of created object */
        &creation_data,   /* out: TPMT_CREATION_DATA for object */
        &creation_digest, /* out: digest of creationData using nameAlg */
        &creation_ticket, /* out: ticket used to associate object and TPM */
        &name,            /* out: name of created object */
        NULL              /* out: sessions / auths returned */
    ));
    if (rc == TSS2_RC_SUCCESS) {
        g_print ("  handle returned: 0x%" PRIx32 "\n", *handle);
    } else {
        g_warning ("Tss2_Sys_CreatePrimary returned: 0x%" PRIx32, rc);
    }

    return rc;
}
/*
 */
TSS2_RC
create_key (TSS2_SYS_CONTEXT *sapi_context,
            TPM2_HANDLE        parent_handle,
            TPM2B_PRIVATE    *out_private,
            TPM2B_PUBLIC     *out_public)
{
    TSS2_RC rc;

    TPM2B_SENSITIVE_CREATE in_sensitive    = TPM2B_SENSITIVE_CREATE_ZERO_INIT;
    TPM2B_PUBLIC           in_public       = TPM2B_PUBLIC_ZERO_INIT;
    TPM2B_DATA             outside_info    = TPM2B_DATA_ZERO_INIT;
    TPML_PCR_SELECTION     creation_pcr    = TPML_PCR_SELECTION_ZERO_INIT;
    TPM2B_CREATION_DATA    creation_data   = TPM2B_CREATION_DATA_ZERO_INIT;
    TPM2B_DIGEST           creation_hash   = TPM2B_DIGEST_STATIC_INIT;
    TPMT_TK_CREATION       creation_ticket = TPMT_TK_CREATION_ZERO_INIT;
    TSS2L_SYS_AUTH_COMMAND cmd_auths = {
        .count = 1,
        .auths = {{
            .sessionHandle = TPM2_RS_PW,
        }}
    };

    g_debug ("create_key with parent_handle: 0x%" PRIx32, parent_handle);
    in_sensitive.size = in_sensitive.sensitive.userAuth.size + 2;
    in_public.publicArea.type = TPM2_ALG_RSA;
    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
    in_public.publicArea.objectAttributes = \
        TPMA_OBJECT_FIXEDTPM            | TPMA_OBJECT_FIXEDPARENT  | \
        TPMA_OBJECT_SENSITIVEDATAORIGIN | TPMA_OBJECT_USERWITHAUTH | \
        TPMA_OBJECT_DECRYPT             | TPMA_OBJECT_SIGN_ENCRYPT;
    /* TPM2B_DIGEST / authPolicy */
    in_public.publicArea.authPolicy.size = 0;
    /* TPMU_PUBLIC_PARAMS / parameters: key type is TPM2_ALG_RSA, set parameters accordingly */
    in_public.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM2_ALG_NULL;
    in_public.publicArea.parameters.rsaDetail.scheme.scheme = TPM2_ALG_NULL;
    in_public.publicArea.parameters.rsaDetail.keyBits = 2048;
    in_public.publicArea.parameters.rsaDetail.exponent = 0;
    /* TPMU_PUBLIC_ID / unique */
    in_public.publicArea.unique.rsa.size = 0;

    g_print ("Tss2_Sys_Create with parent handle: 0x%" PRIx32 "\n",
             parent_handle);
    rc = TSS2_RETRY_EXP (Tss2_Sys_Create(
        sapi_context,
        parent_handle,
        &cmd_auths,
        &in_sensitive,
        &in_public,
        &outside_info,
        &creation_pcr,
        out_private, /* private part of new key returned to caller */
        out_public,  /* public part of new key returned to caller */
        &creation_data,
        &creation_hash,
        &creation_ticket,
        NULL
    ));
    if (rc == TSS2_RC_SUCCESS) {
        g_print ("Tss2_Sys_Create returned TSS2_RC_SUCCESS\n  parent handle: "
                 "0x%" PRIx32 "\n  out_private: 0x%" PRIxPTR "\n  out_public: "
                 "0x%" PRIxPTR "\n", parent_handle, (uintptr_t)&out_private,
                 (uintptr_t)&out_public);
    } else {
        g_warning ("Tss2_Sys_Create returned: 0x%" PRIx32, rc);
    }

    return rc;
}
/*
 */
TSS2_RC
load_key (TSS2_SYS_CONTEXT *sapi_context,
          TPM2_HANDLE        parent_handle,
          TPM2_HANDLE       *out_handle,
          TPM2B_PRIVATE    *in_private,
          TPM2B_PUBLIC     *in_public)
{
    TSS2_RC            rc;
    TPM2B_NAME         name                  = TPM2B_NAME_STATIC_INIT;
    TSS2L_SYS_AUTH_COMMAND cmd_auths = {
        .count = 1,
        .auths = {{
            .sessionHandle = TPM2_RS_PW,
        }}
    };

    g_print ("Tss2_Sys_Load with parent handle: 0x%" PRIx32 "\n  in_private: "
             "0x%" PRIxPTR "\n  in_public: 0x%" PRIxPTR "\n",
             parent_handle, (uintptr_t)in_private, (uintptr_t)in_public);
    rc = Tss2_Sys_Load (sapi_context,
                        parent_handle,
                        &cmd_auths,
                        in_private,
                        in_public,
                        out_handle,
                        &name,
                        NULL);

    if (rc == TSS2_RC_SUCCESS) {
        g_print ("Tss2_Sys_Load returned TSS2_RC_SUCCESS\n  parent handle: "
                 "0x%" PRIx32 "\n  new handle: 0x%" PRIx32 "\n",
                 parent_handle, *out_handle);
    } else {
        g_warning ("Tss2_Sys_Create returned: 0x%" PRIx32, rc);
    }

    return rc;
}
TSS2_RC
undefine_nv_index (TSS2_SYS_CONTEXT *sapi_context,
                   TPM2_HANDLE        index)
{
    TSS2_RC rc;
    TSS2L_SYS_AUTH_COMMAND cmd_auths = {
        .count = 1,
        .auths = {{
            .sessionHandle = TPM2_RS_PW,
        }}
    };

    g_debug ("undefine_nv_index: sapi_context: 0x%" PRIxPTR " index: 0x%"
             PRIx32, (uintptr_t)sapi_context, index);
    if (sapi_context == NULL) {
        g_error ("undefine_nv_index passed NULL reference");
    }

    rc = Tss2_Sys_NV_UndefineSpace (sapi_context,
                                    TPM2_RH_OWNER,
                                    index,
                                    &cmd_auths,
                                    0);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Tss2_Sys_Nv_UndefineSpace: failed to undefine nv index for "
                   "index: 0x%" PRIx32 " TSS2_RC: 0x%" PRIx32, index, rc);
    }

    return rc;
}
TSS2_RC
save_context (TSS2_SYS_CONTEXT *sapi_context,
              TPM2_HANDLE        handle,
              TPMS_CONTEXT     *context)
{
    TSS2_RC rc;

    g_debug ("save_context: sapi_context: 0x%" PRIxPTR " handle: 0x%"
             PRIx32 " context: 0x%" PRIxPTR, (uintptr_t)sapi_context,
             handle, (uintptr_t)context);
    if (sapi_context == NULL || context == NULL) {
        g_error ("save_context passed NULL reference");
    }

    rc = Tss2_Sys_ContextSave (sapi_context, handle, context);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Tss2_Sys_ContextSave: failed to save context for handle: "
                   "0x%" PRIx32, handle);
    }

    return rc;
}
TSS2_RC
flush_context (TSS2_SYS_CONTEXT *sapi_context,
               TPM2_HANDLE        handle)
{
    TSS2_RC rc;

    g_debug ("flush_context: sapi_context: 0x%" PRIxPTR " handle: 0x%"
             PRIx32, (uintptr_t)sapi_context, handle);
    if (sapi_context == NULL) {
        g_error ("flush_context passed NULL reference");
    }

    rc = Tss2_Sys_FlushContext (sapi_context, handle);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Tss2_Sys_FlushContext: failed to flush context for "
                   "handle: 0x%" PRIx32 " TSS2_RC: 0x%" PRIx32, handle, rc);
    }

    return rc;
}
TSS2_RC
evict_persistent_objs (TSS2_SYS_CONTEXT *sapi_context,
                       TPM2_HANDLE        handle)
{
    TSS2_RC rc;
    TSS2L_SYS_AUTH_COMMAND cmd_auths = {
        .count = 1,
        .auths = {{
            .sessionHandle = TPM2_RS_PW,
        }}
    };

    g_debug ("evict_persistent_objs: sapi_context: 0x%" PRIxPTR
             " handle: 0x%" PRIx32, (uintptr_t)sapi_context, handle);
    if (sapi_context == NULL) {
        g_error ("evict_persistent_objs passed NULL reference");
    }

    rc = Tss2_Sys_EvictControl (sapi_context, TPM2_RH_OWNER,
                                handle, &cmd_auths,
                                handle, NULL);
    if (rc != TPM2_RC_SUCCESS) {
        g_warning ("Tss2_Sys_EvictControl: failed to evict control for "
                   "handle: 0x%" PRIx32 " TSS2_RC: 0x%" PRIx32, handle, rc);
    }

    return rc;
}
void
clean_up_all (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC rc;
    unsigned int i, j;
    TPMI_YES_NO more_data;
    TPMS_CAPABILITY_DATA capability_data;
    TPML_HANDLE *handles = &capability_data.data.handles;
    struct property_info {
        UINT32 property;
        UINT32 count;
    } properties[] = {
        {
            .property = TPM2_PERSISTENT_FIRST,
            .count = TPM2_MAX_CAP_HANDLES,
        },
        {
            .property = TPM2_TRANSIENT_FIRST,
            .count = TPM2_MAX_CAP_HANDLES,
        },
        {
            .property = TPM2_NV_INDEX_FIRST,
            .count = TPM2_MAX_CAP_HANDLES,
        },
        {
            .property = TPM2_LOADED_SESSION_FIRST,
            .count = TPM2_MAX_CAP_HANDLES,
        },
        {
            .property = TPM2_ACTIVE_SESSION_FIRST,
            .count = TPM2_MAX_CAP_HANDLES,
        }
    };

    for (i = 0; i < sizeof(properties) / sizeof(struct property_info); ++i) {
        rc = Tss2_Sys_GetCapability (sapi_context,
                                     NULL,
                                     TPM2_CAP_HANDLES,
                                     properties[i].property,
                                     properties[i].count,
                                     &more_data,
                                     &capability_data,
                                     NULL);
        if (rc != TSS2_RC_SUCCESS) {
             g_warning ("Tss2_Sys_GetCapability: failed to get capability for "
                        "handles property: 0x%" PRIx32 " count: 0x%" PRIx32
                        " TSS2_RC: 0x%" PRIx32, properties[i].property,
                        properties[i].count, rc);
             continue;
        }

        for (j = 0; j < handles->count; ++j) {
            if (properties[i].property == (UINT32) TPM2_NV_INDEX_FIRST) {
                undefine_nv_index (sapi_context, handles->handle[j]);
                continue;
            }

            /*
             * TPM2_FlushContext command may not be used to remove a persistent
             * objects from the TPM. So we always handle persistent handles
             * prior to transient handles to allow evicting them on next round.
             */
            if (properties[i].property == (UINT32) TPM2_PERSISTENT_FIRST) {
                evict_persistent_objs (sapi_context, handles->handle[j]);
                continue;
            }

            flush_context (sapi_context, handles->handle[j]);
        }
    }
}
/*
 * This function is a very simple wrapper around the TPM2_StartAuthSession
 * function. It uses the most simple / default values to create an unsalted,
 * unbound session.
 */
TSS2_RC
start_auth_session (TSS2_SYS_CONTEXT      *sapi_context,
                    TPMI_SH_AUTH_SESSION  *session_handle)
{
    TSS2_RC rc;
    TPM2B_NONCE nonce_caller = {
        .size   = TPM2_SHA256_DIGEST_SIZE,
        .buffer = {
            0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
            0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
            0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
            0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
        }
    };
    TPM2B_NONCE nonce_tpm = {
        .size   = TPM2_SHA256_DIGEST_SIZE,
        .buffer = { 0 }
    };
    TPM2B_ENCRYPTED_SECRET encrypted_salt = TPM2B_ENCRYPTED_SECRET_ZERO_INIT;
    TPMT_SYM_DEF           symmetric      = { .algorithm = TPM2_ALG_NULL };

    g_debug ("StartAuthSession for TPM_SE_POLICY (policy session)");
    rc = Tss2_Sys_StartAuthSession (sapi_context,
                                    TPM2_RH_NULL,     /* tpmKey */
                                    TPM2_RH_NULL,     /* bind */
                                    0,               /* cmdAuthsArray */
                                    &nonce_caller,   /* nonceCaller */
                                    &encrypted_salt, /* encryptedSalt */
                                    TPM2_SE_POLICY,   /* sessionType */
                                    &symmetric,      /* symmetric */
                                    TPM2_ALG_SHA256,  /* authHash */
                                    session_handle,  /* sessionHandle */
                                    &nonce_tpm,      /* nonceTPM */
                                    0                /* rspAuthsArray */
                                    );
    if (rc != TSS2_RC_SUCCESS)
        g_warning ("Tss2_Sys_StartAuthSession failed: 0x%" PRIx32, rc);

    return rc;
}
/*
 * This function dumps the fields of the TPMS_CONTEXT structure. The one
 * encrypted field (contextBlob) is dumped as an address.
 */
void
prettyprint_context (TPMS_CONTEXT *context)
{
    g_debug ("sequence:    0x%" PRIx64, context->sequence);
    g_debug ("savedHandle: 0x%" PRIxHANDLE, context->savedHandle);
    g_debug ("hierarchy:   0x%" PRIxHANDLE, context->hierarchy);
    g_debug ("contextBlob: 0x%" PRIxPTR, (uintptr_t)&context->contextBlob);
}
