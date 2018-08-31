/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
#ifndef TPM2_HEADER_H
#define TPM2_HEADER_H

#include <sys/types.h>
#include <tss2/tss2_tcti.h>

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
