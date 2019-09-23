/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <assert.h>
#include <glib.h>
#include <inttypes.h>
#include <stddef.h>

#include <tss2/tss2_mu.h>
#include <tss2/tss2_tpm2_types.h>

#include "tpm2-header.h"

/**
 * Extract the 'tag' field from the tpm command header. This is a
 * TPMI_ST_COMMAND_TAG, which is a fancy word for UINT16
 */
TPMI_ST_COMMAND_TAG
get_command_tag (uint8_t *command_header)
{
    return be16toh (*(TPMI_ST_COMMAND_TAG*)command_header);
}
/**
 * Get the commandSize field from the tpm command buffer supplied in the
 * 'command_header' parameter. We assume that the command buffer is a valid TPM
 * command (or response) and so it must be at least COMMAND_HEADER_SIZE
 * bytes long.
 * NOTE: The TPM2 architecture spec states in section 18.2.2 that the
 *       commandSize field from the header is the total size of the
 *       command. This includes the header itself.
 */
UINT32
get_command_size (uint8_t *command_header)
{
    return be32toh (*(UINT32*)(command_header + sizeof (TPMI_ST_COMMAND_TAG)));
}
/**
 * Extract the commandCode from a tpm command buffer. It is the 3rd field
 * in the header so we do some pointer math to get the offset.
 */
TPM2_CC
get_command_code (uint8_t *command_header)
{
    return be32toh (*(TPM2_CC*)(command_header + sizeof (TPMI_ST_COMMAND_TAG) + sizeof (UINT32)));
}
/**
 * Get the 'tag' field from a TPM response buffer. This is the first field
 * in the header.
 */
TPM2_ST
get_response_tag (uint8_t *response_header)
{
    return be16toh (*(TPM2_ST*)response_header);
}
/*
 * Set the 'tag' field in a response buffer.
 */
void
set_response_tag (uint8_t *response_header,
                  TPM2_ST   tag)
{
    *(TPM2_ST*)response_header = htobe16 (tag);
}
/**
 * Get the 'responseSize' field from a TPM response header.
 */
UINT32
get_response_size (uint8_t *response_header)
{
    return be32toh (*(UINT32*)(response_header + sizeof (TPM2_ST)));
}
/*
 * Set the 'size' field in a response buffer.
 */
void
set_response_size (uint8_t *response_header,
                   UINT32   size)
{
    *(UINT32*)(response_header + sizeof (TPM2_ST)) = htobe32 (size);
}
/**
 * Get the responseCode field from the TPM response buffer supplied in the
 * 'response_header' parameter. We assume that the response buffer is a valid TPM
 * response header so it must be at least RESPONSE_HEADER_SIZE bytes long.
 */
TSS2_RC
get_response_code (uint8_t *response_header)
{
    return be32toh (*(TSS2_RC*)(response_header + sizeof (TPM2_ST) + sizeof (UINT32)));
}
/*
 * Set the responseCode field in a TPM response buffer.
 */
void
set_response_code (uint8_t *response_header,
                   TSS2_RC  rc)
{
    *(TSS2_RC*)(response_header + sizeof (TPM2_ST) + sizeof (UINT32)) = \
        htobe32 (rc);
}
/*
 * create response object, populate with SessionEntry member
 * 'context_client'
 */
TSS2_RC
tpm2_header_init (uint8_t *buf,
                  size_t buf_size,
                  TPM2_ST tag,
                  UINT32 size,
                  TSS2_RC code)
{
    size_t offset = 0;
    TSS2_RC rc;

    g_assert (buf_size >= TPM_HEADER_SIZE);
    rc = Tss2_MU_TPM2_ST_Marshal (tag, buf, buf_size, &offset);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: failed to write TPM2_ST tag to response header: 0x%"
                   PRIx32, __func__, rc);
        return rc;
    }
    rc = Tss2_MU_UINT32_Marshal (size, buf, buf_size, &offset);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: failed to write UINT32 size to response header: 0x%"
                   PRIx32, __func__, rc);
        return rc;
    }
    rc = Tss2_MU_UINT32_Marshal (code, buf, buf_size, &offset);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: failed to write UINT32 responseCode to response header: 0x%"
                   PRIx32, __func__, rc);
    }

    return rc;
}
