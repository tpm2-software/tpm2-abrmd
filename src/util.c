#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
ssize_t
write_all (const gint    fd,
           const void   *buf,
           const size_t  size)
{
    ssize_t written = 0;
    size_t written_total = 0;

    do {
        g_debug ("writing %d bytes starting at 0x%x to fd %d",
                 size - written_total,
                 buf + written_total,
                 fd);
        written = write (fd,
                         buf  + written_total,
                         size - written_total);
        switch (written) {
        case -1: return written;
        case  0: return written_total;
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
    gint    flags = 0;

    flags = fcntl(fd, F_GETFL, 0);
    if (!(flags && O_NONBLOCK)) {
        g_debug ("read_till_eagain: setting fd %d to O_NONBLOCK", fd);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
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
    fcntl(fd, F_SETFL, flags);

    return bytes_total;
err_out:
    fcntl(fd, F_SETFL, flags);
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
