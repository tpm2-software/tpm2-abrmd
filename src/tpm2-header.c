#include <tpm20.h>

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
TPM_CC
get_command_code (uint8_t *command_header)
{
    return be32toh (*(TPM_CC*)(command_header + sizeof (TPMI_ST_COMMAND_TAG) + sizeof (UINT32)));
}
/**
 * Get the 'tag' field from a TPM response buffer. This is the first field
 * in the header.
 */
TPM_ST
get_response_tag (uint8_t *response_header)
{
    return be16toh (*(TPM_ST*)response_header);
}
/**
 * Get the 'responseSize' field from a TPM response header.
 */
UINT32
get_response_size (uint8_t *response_header)
{
    return be32toh (*(UINT32*)(response_header + sizeof (TPM_ST)));
}
/**
 * Get the responseCode field from the TPM response buffer supplied in the
 * 'response_header' parameter. We assume that the response buffer is a valid TPM
 * response header so it must be at least RESPONSE_HEADER_SIZE bytes long.
 */
TSS2_RC
get_response_code (uint8_t *response_header)
{
    return be32toh (*(TSS2_RC*)(response_header + sizeof (TPM_ST) + sizeof (UINT32)));
}

