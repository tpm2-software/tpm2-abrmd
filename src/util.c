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
    guint byte_ctr;
    guint indent_ctr;
    size_t line_length = indent + width * 3 + 1;
    char  line [line_length];
    char  *line_position = line;

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
            g_debug ("%s", line);
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
        g_debug ("writing %zu bytes starting at 0x%" PRIxPTR " to fd %d",
                 size - written_total,
                 (uintptr_t)buf + written_total,
                 fd);
        written = write (fd,
                         buf  + written_total,
                         size - written_total);
        switch (written) {
        case -1:
            g_warning ("failed to write to fd %d: %s", fd, strerror (errno));
            return written;
        case  0:
            return (ssize_t)written_total;
        default:
            g_debug ("wrote %zd bytes to fd %d", written, fd);
        }
        written_total += (size_t)written;
    } while (written_total < size);
    g_debug ("returning %zu", written_total);

    return (ssize_t)written_total;
}
void
process_control_code (ControlCode code)
{
    g_debug ("Processing ControlCode: 0x%x", code);
    switch (code) {
    case CHECK_CANCEL:
        pthread_testcancel ();
        break;
    default:
        break;
    }
}
/*
 * Read a TPM command or response header from the parameter 'fd'. Headers
 * are a fixed size so the buf_size is largely a formality: it's just a way to
 * be sure the caller understands that the buffer must be at least
 * TPM_HEADER_SIZE bytes.
 *
 * On success this function returns 0.
 * On error this function returns the value of errno.
 * On a short read or EOF the function returns -1.
 * If the supplied buffer size is too small to hold the header, -2 is returned. 
 */
int
tpm_header_from_fd (int       fd,
                    uint8_t  *buf,
                    size_t    buf_size)
{
    ssize_t num_read = 0;
    int errno_tmp;

    g_debug ("tpm_header_from_fd");
    if (buf_size < TPM_HEADER_SIZE) {
        g_warning ("buffer size too small for tpm header");
        return -2;
    }

    num_read = TEMP_FAILURE_RETRY (read (fd, buf, TPM_HEADER_SIZE));
    errno_tmp = errno;
    switch (num_read) {
    case TPM_HEADER_SIZE:
        g_debug ("  read %zd bytes", num_read);
        g_debug_bytes (buf, (size_t)num_read, 16, 4);
        return 0;
    case -1:
        g_warning ("error reading from fd %d: %s", fd, strerror (errno));
        return errno_tmp;
    case 0:
        g_warning ("EOF trying to read tpm command header from fd: %d", fd);
        return -1;
    default:
        g_warning ("read %zd bytes on fd %d, expecting %" PRIu32,
                   num_read, fd, TPM_HEADER_SIZE);
        return -1;
    }
}
/*
 * Read the body of a TPM command / response buffer from the provided file
 * descriptor 'fd'. The command body will be written to the buffer provided
 * in 'buf'. The caller specifies the size of the command body in the
 * 'body_size' parameter.
 *
 * On success this function returns 0.
 * On error this function returns the value of errno.
 * On a short read or EOF the function returns -1.
 */
int
tpm_body_from_fd (int       fd,
                  uint8_t  *buf,
                  size_t    body_size)
{
    ssize_t num_read = 0;
    int errno_tmp;

    g_debug ("read_tpm_command_body_from_fd");
    num_read = TEMP_FAILURE_RETRY (read (fd, buf, body_size));
    errno_tmp = errno;
    switch (num_read) {
    case -1:
        g_warning ("  error reading from fd %d: %s", fd, strerror (errno));
        return errno_tmp;
    case 0:
        g_warning ("  EOF reading TPM command body from fd: %d", fd);
        return -1;
    default:
        if (num_read == body_size) {
            g_debug ("  read %zu bytes as expected", num_read);
            g_debug_bytes (buf, body_size, 16, 4);
            return 0;
        } else {
            g_warning ("  read %zd bytes on fd %d, expecting %zd",
                       num_read, fd, body_size);
            return -1;
        }
    }
}
/**
 * Read a TPM command from the provided file descriptor. The size of the
 * command is returned in the 'command_size' parameter. The return value
 * is a pointer to the newly allocated buffer holding the command.
 * The caller must deallocate the command buffer.
 */
uint8_t*
read_tpm_command_from_fd (int       fd,
                          UINT32   *command_size)
{
    size_t header_size = TPM_HEADER_SIZE;
    uint8_t header[TPM_HEADER_SIZE] = { 0, };
    uint8_t *tpm_command = NULL;
    int ret;

    g_debug ("read_tpm_commad_from_fd");
    ret = tpm_header_from_fd (fd, header, header_size);
    if (ret != 0) {
        return NULL;
    }
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
    ret = tpm_body_from_fd (fd,
                            &tpm_command [header_size],
                            *command_size - header_size);
    if (ret != 0) {
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
