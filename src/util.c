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
#define MAX_LINE_LENGTH 200
void
g_debug_bytes (uint8_t const *byte_array,
               size_t         array_size,
               size_t         width,
               size_t         indent)
{
    guint byte_ctr;
    guint indent_ctr;
    size_t line_length = indent + width * 3 + 1;
    char  line [MAX_LINE_LENGTH] = { 0 };
    char  *line_position = NULL;

    if (line_length > MAX_LINE_LENGTH) {
        g_warning ("g_debug_bytes: MAX_LINE_LENGTH exceeded");
        return;
    }

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
 * Read some data from the fd_receive.
 * Parameters:
 *   context:  tabrmd context
 *   buf:      destination buffer
 *   count:    number of bytes to read
 *     NOTE: some bytes may have already been read on past calls
 * Returns:
 *   -1:     when EOF is reached
 *   0:      if requested number of bytes received
 *   errno:  in the event of an error from the 'read' call
 * NOTE: The caller must ensure that 'buf' is large enough to hold count
 *       bytes.
 */
int
read_data (int                       fd,
           size_t                   *index,
           uint8_t                  *buf,
           size_t                    count)
{
    ssize_t num_read = 0;
    int    errno_tmp = 0;

    /*
     * Index is where we left off. The caller is asking us to read 'count'
     * bytes. So count - index is the number of bytes we need to read.
     */
    do {
        g_debug ("reading %zd bytes from fd %d, to 0x%" PRIxPTR,
                 count, fd, (uintptr_t)&buf[*index]);
        num_read = TEMP_FAILURE_RETRY (read (fd,
                                             &buf[*index],
                                             count));
        errno_tmp = errno;
        if (num_read > 0) {
            g_debug ("successfully read %zd bytes", num_read);
            g_debug_bytes (&buf[*index], num_read, 16, 4);
            /* Advance index by the number of bytes read. */
            *index += num_read;
        } else if (num_read == 0) {
            g_debug ("read produced EOF");
            return -1;
        } else { /* num_read < 0 */
            g_warning ("read on fd %d produced error: %d, %s",
                       fd, errno_tmp, strerror (errno_tmp));
            return errno_tmp;
        }
    } while (*index < count);

    return 0;
}
/*
 * This function attempts to read a TPM2 command or response into the provided
 * buffer. It specifically handles the details around reading the command /
 * response header, determining the size of the data that it needs to read and
 * keeping track of past / partial reads.
 * Returns:
 *   -1: If the underlying syscall results in an EOF
 *   0: If data is successfully read.
 *      NOTE: The index will be updated to the size of the command buffer.
 *   errno: In the event of an error from the underlying 'read' syscall.
 *   EPROTO: If buf_size is less than the size from the command buffer.
 */
int
read_tpm_buffer (int                       fd,
                 size_t                   *index,
                 uint8_t                  *buf,
                 size_t                    buf_size)
{
    ssize_t ret = 0;
    uint32_t size = 0;

    /* if the buf_size isn't at least large enough to hold the header */
    if (buf_size < TPM_HEADER_SIZE) {
        return EPROTO;
    }
    /* If we don't have the whole header yet try to get it. */
    if (*index < TPM_HEADER_SIZE) {
        ret = read_data (fd, index, buf, TPM_HEADER_SIZE - *index);
        if (ret != 0) {
            /* Pass errors up to the caller. */
            return ret;
        }
    }

    /* Once we have the header we can get the size of the whole blob. */
    size = get_command_size (buf);
    /* If size from header is size of header, there's nothing more to read. */
    if (size == TPM_HEADER_SIZE) {
        return ret;
    }
    /* Not enough space in buf to for data in the buffer (header.size). */
    if (size > buf_size) {
        return EPROTO;
    }
    /* Now that we have the header, we know the whole buffer size. Get it. */
    return read_data (fd, index, buf, size - *index);
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
int
set_flags (const int fd,
           const int flags)
{
    int local_flags, ret = 0;

    local_flags = fcntl(fd, F_GETFL, 0);
    if (!(local_flags && flags)) {
        g_debug ("connection: setting flags for fd %d to %d",
                 fd, local_flags | flags);
        ret = fcntl(fd, F_SETFL, local_flags | flags);
    }
    return ret;
}
