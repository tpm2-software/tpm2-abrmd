/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TPM2_HEADER_H
#define TPM2_HEADER_H

#include <sys/types.h>
#include <tss2/tss2_tcti.h>

#if defined(__FreeBSD__)
#include <sys/endian.h>
#else
#include <endian.h>
#endif

/* A convenience macro to get us the size of the TPM header. */
#define TPM_HEADER_SIZE (UINT32)(sizeof (TPM2_ST) + sizeof (UINT32) + sizeof (TPM2_CC))

/*
 * A generic tpm header structure, could be command or response.
 * NOTE: Do not expect sizeof (tpm_header_t) to get your the size of the
 * header. The compiler pads this structure. Use the macro above.
 */
typedef struct {
    TPM2_ST   tag;
    UINT32   size;
    UINT32   code;
} tpm_header_t;

TPMI_ST_COMMAND_TAG    get_command_tag        (uint8_t      *command_header);
UINT32                 get_command_size       (uint8_t      *command_header);
TPM2_CC                 get_command_code       (uint8_t      *command_header);
TPM2_ST                 get_response_tag       (uint8_t      *response_header);
void                   set_response_tag       (uint8_t      *response_header,
                                               TPM2_ST        tag);
UINT32                 get_response_size      (uint8_t      *response_header);
void                   set_response_size      (uint8_t      *response_header,
                                               UINT32        size);
TSS2_RC                get_response_code      (uint8_t      *response_header);
void                   set_response_code      (uint8_t      *response_header,
                                               TSS2_RC       rc);
TSS2_RC                tpm2_header_init       (uint8_t      *buf,
                                               size_t        buf_size,
                                               TPM2_ST       tag,
                                               UINT32        size,
                                               TSS2_RC       code);

#endif /* TPM2_HEADER_H */
