#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "tpm2-header.h"

/**
 * This is a wrapper around g_debug to dump a binary buffer in a human
 * readable format. Since g_debug appends a new line to each string that
 * it builds we dump a single line at a time. Each line is indented by
 * 'indent' spaces. The 'width' parameter determines how many bytes are
 * output on each line.
 */
void
g_debug_bytes (uint8_t const *byte_array,
               size_t         array_size,
               size_t         width,
               size_t         indent)
{
    int byte_ctr;
    int indent_ctr;
    size_t line_length = indent + width * 3 + 1;
    uint8_t line [line_length];
    uint8_t *line_position = line;

    line [line_length - 1] = '\0';
    for (byte_ctr = 0; byte_ctr < array_size; ++byte_ctr) {
        /* index into line where next byte is written */
        line_position = line + indent + (byte_ctr % width) * 3;
        /* detect the beginning of a line, pad indent spaces */
        if (byte_ctr % width == 0)
            for (indent_ctr = 0; indent_ctr < indent; ++indent_ctr)
                line [indent_ctr] = ' ';
        sprintf (line_position, "%02x", byte_array [byte_ctr]);
        /**
         *  If we're not width bytes into the array AND we're not at the end
         *  of the byte array: print a space. This is padding between the
         *  current byte and the next.
         */
        if (byte_ctr % width != width - 1 && byte_ctr != array_size - 1) {
            sprintf (line_position + 2, " ");
        } else {
            g_debug (line);
        }
    }
}
/** Write as many of the size bytes from buf to fd as possible.
 */
ssize_t
write_all (const gint    fd,
           const void   *buf,
           const size_t  size)
{
    ssize_t written = 0;
    size_t written_total = 0;

    do {
        g_debug ("writing 0x%x bytes starting at 0x%x to fd %d",
                 size - written_total,
                 buf + written_total,
                 fd);
        written = write (fd,
                         buf  + written_total,
                         size - written_total);
        switch (written) {
        case -1:
            g_warning ("failed to write to fd %d: %s", fd, strerror (errno));
            return written;
        case  0:
            return written_total;
        default:
            g_debug ("wrote %d bytes to fd %d", written, fd);
        }
        written_total += written;
    } while (written_total < size);
    g_debug ("returning %d", written_total);

    return written_total;
}
/** Read as many bytes as possible from fd into a newly allocated buffer up to
 * UTIL_BUF_MAX or a call to read that would block.
 * THE fd PARAMETER MUST BE AN FD WITH O_NONBLOCK SET.
 * We return -1 for an unrecoverable error or when the fd has been closed.
 * The rest of the time we return the number of bytes read.
 */
ssize_t
read_till_block (const gint   fd,
                 guint8     **buf,
                 size_t      *size)
{
    guint8  *local_buf = NULL, *tmp_buf;
    ssize_t bytes_read  = 0;
    size_t  bytes_total = 0;

    g_debug ("reading till EAGAIN on fd %d", fd);
    do {
        g_debug ("reallocing buf at 0x%x to %d bytes", local_buf, bytes_total + UTIL_BUF_SIZE);
        tmp_buf = realloc (local_buf, bytes_total + UTIL_BUF_SIZE);
        if (tmp_buf == NULL) {
            g_warning ("realloc of %d bytes failed: %s",
                       bytes_total + UTIL_BUF_SIZE, strerror (errno));
            goto err_out;
        }
        g_debug ("reallocated buf at 0x%x to %d bytes",
                 local_buf, bytes_total + UTIL_BUF_SIZE);
        local_buf = tmp_buf;

        bytes_read = read (fd, (void*)(local_buf + bytes_total), UTIL_BUF_SIZE);
        switch (bytes_read) {
        case -1:
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                g_debug ("would block on fd %d", fd);
                goto out;
            } else {
                g_warning ("unexpected error reading from fd %d: errno %d: %s",
                           fd, errno, strerror (errno));
                goto err_out;
            }
        case 0:
            g_info ("read returned 0 bytes -> fd %d closed", fd);
            goto err_out;
        default:
            g_debug ("read %d bytes from fd %d", bytes_read, fd);
            bytes_total += bytes_read;
            break;
        }
    } while (bytes_total < UTIL_BUF_MAX);
out:
    *size = bytes_total;
    *buf = local_buf;

    return bytes_total;
err_out:
    if (local_buf != NULL)
        free (local_buf);
    return -1;
}

void
process_control_message (ControlMessage *msg)
{
    g_debug ("Got a ControlMessage with code: 0x%x", msg->code);
    switch (msg->code) {
    case CHECK_CANCEL:
        pthread_testcancel ();
        break;
    default:
        break;
    }
}
/**
 * Read a TPM command header from the parameter 'fd'. Command headers are
 * a fixed size so the buf_size is largely a formality: it's just a way to
 * be sure the caller understands that the buffer must be at least
 * TPM_COMMAND_HEADER_SIZE bytes.
 */
uint8_t*
read_tpm_command_header_from_fd (int fd, uint8_t *buf, size_t buf_size)
{
    int num_read = 0;

    g_debug ("read_tpm_command_header_from_fd");
    if (buf_size < TPM_COMMAND_HEADER_SIZE) {
        g_error ("buffer size too small for tpm command header");
        return NULL;
    }

    num_read = read (fd, buf, TPM_COMMAND_HEADER_SIZE);
    switch (num_read) {
    case TPM_COMMAND_HEADER_SIZE:
        g_debug ("  read 0x%x bytes", num_read);
        g_debug_bytes (buf, num_read, 16, 4);
        return buf;
    case -1:
        g_warning ("error reading from fd %d: %s", fd, strerror (errno));
        return NULL;
    case 0:
        g_warning ("EOF trying to read tpm command header from fd: %d", fd);
        return NULL;
    default:
        g_warning ("read %d bytes on fd %d, expecting %d",
                   num_read, fd, TPM_COMMAND_HEADER_SIZE);
        return NULL;
    }
}
/**
 * Read the body of a TPM command buffer from the provided file descriptor
 * 'fd'. The command body will be written to the buffer provided in 'buf'.
 * The caller specifies the size of the command body in the 'body_size'
 * parameter.
 */
uint8_t*
read_tpm_command_body_from_fd (int fd, uint8_t *buf, size_t body_size)
{
    int num_read = 0;

    g_debug ("read_tpm_command_body_from_fd");
    num_read = read (fd, buf, body_size);
    if (num_read == body_size) {
        g_debug ("  read 0x%x bytes as expected", num_read);
        g_debug_bytes (buf, body_size, 8, 4);
        return buf;
    }
    switch (num_read) {
    case -1:
        g_warning ("  error reading from fd %d: %s", fd, strerror (errno));
        return NULL;
    case 0:
        g_warning ("  EOF reading TPM command body from fd: %d", fd);
        return NULL;
    default:
        g_warning ("  read %d bytes on fd %d, expecting %d",
                   num_read, fd, body_size);
        return NULL;
    }
}
/**
 * Read a TPM command from the provided file descriptor. The size of the
 * command is returned in the 'command_size' parameter. The return value
 * is a pointer to the newly allocated buffer holding the command.
 * The caller must deallocate the command buffer.
 */
uint8_t*
read_tpm_command_from_fd (int fd, UINT32 *command_size)
{
    size_t header_size = TPM_COMMAND_HEADER_SIZE;
    uint8_t header[TPM_COMMAND_HEADER_SIZE] = { 0, };
    uint8_t *tpm_command = NULL, *tmp = NULL;

    g_debug ("read_tpm_commad_from_fd");
    if (read_tpm_command_header_from_fd (fd, header, header_size) == NULL)
        return NULL;
    *command_size = get_command_size (header);
    g_debug ("  reading command_size: 0x%x", *command_size);
    if (*command_size > UTIL_BUF_MAX) {
        g_warning ("received TPM command larger than %d", UTIL_BUF_MAX);
        return NULL;
    }
    tpm_command = g_malloc0 (*command_size);
    if (tpm_command == NULL) {
        g_error ("g_malloc0 failed to allocate buffer for TPM command");
        return NULL;
    }
    memcpy (tpm_command, header, header_size);
    tmp = read_tpm_command_body_from_fd (fd,
                                         tpm_command + header_size,
                                         *command_size - header_size);
    if (tmp == NULL) {
        g_free (tpm_command);
        return NULL;
    }

    return tpm_command;
}
/* pretty print */
void
g_debug_tpma_cc (TPMA_CC tpma_cc)
{
    g_debug ("TPMA_CC: 0x%08" PRIx32, tpma_cc.val);
    g_debug ("  commandIndex: 0x%" PRIx16, tpma_cc.val & TPMA_CC_COMMANDINDEX);
    g_debug ("  reserved1:    0x%" PRIx8, (tpma_cc.val & TPMA_CC_RESERVED1) >> 16);
    g_debug ("  nv:           %s", prop_str (tpma_cc.val & TPMA_CC_NV));
    g_debug ("  extensive:    %s", prop_str (tpma_cc.val & TPMA_CC_EXTENSIVE));
    g_debug ("  flushed:      %s", prop_str (tpma_cc.val & TPMA_CC_FLUSHED));
    g_debug ("  cHandles:     0x%" PRIx8, (tpma_cc.val & TPMA_CC_CHANDLES) >> 25);
    g_debug ("  rHandle:      %s", prop_str (tpma_cc.val & TPMA_CC_RHANDLE));
    g_debug ("  V:            %s", prop_str (tpma_cc.val & TPMA_CC_V));
    g_debug ("  Res:          0x%" PRIx8, (tpma_cc.val & TPMA_CC_RES) >> 30);
}
