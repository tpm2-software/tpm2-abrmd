#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#include "tcti-tabrmd-priv.h"
#include "tpm2-header.h"
#include "util.h"

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
    size_t num_read = 0;
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
        switch (num_read) {
        case -1: /* error */
            g_debug ("read produced error: %d, %s",
                     errno_tmp, strerror (errno_tmp));
            return errno_tmp;
        case 0:  /* EOF / fd closed */
            g_debug ("read produced EOF");
            return -1;
        default:
            g_debug ("successfully read %zd bytes", num_read);
            g_debug_bytes (&buf[*index], num_read, 16, 4);
            /* Advance index by the number of bytes read. */
            *index += num_read;
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
