#ifndef TPM2_HEADER_H
#define TPM2_HEADER_H

#include <sys/types.h>
#include <sapi/tpm20.h>

#define TPM_HEADER_SIZE (sizeof (TPM_ST) + sizeof (UINT32) + sizeof (TPM_CC))

TPMI_ST_COMMAND_TAG    get_command_tag        (uint8_t      *command_header);
UINT32                 get_command_size       (uint8_t      *command_header);
TPM_CC                 get_command_code       (uint8_t      *command_header);
TPM_ST                 get_response_tag       (uint8_t      *response_header);
UINT32                 get_response_size      (uint8_t      *response_header);
TSS2_RC                get_response_code      (uint8_t      *response_header);

#endif /* TPM2_HEADER_H */
