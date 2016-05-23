#include <errno.h>
#include <stdlib.h>
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
 * BUF_MAX.
 * We read as much data is available allocating data in BUF_SIZE chunks up to
 * BUF_MAX. This function returns after a short read (less than the requested
 * byte count), or if a read returns 0 indicating the pipe was closed. In the
 * case of the closed pipe, 0 is returned but the caller should check the
 * returned size to see whether or not data was read before the pipe closed.
 */
ssize_t
read_till_short (const gint   fd,
                 guint8     **buf,
                 size_t      *size)
{
    ssize_t bytes_read  = 0;
    size_t  bytes_total = 0;

    g_debug ("attempting to read %d bytes from fd %d", size, fd);
    do {
        *buf = realloc (*buf, bytes_total + BUF_SIZE);
        if (*buf == NULL)
            g_error ("realloc of %d bytes failed: %s",
                     bytes_total + BUF_SIZE, strerror (errno));
        else
            g_debug ("reallocated buf to %d bytes", bytes_total + BUF_SIZE);
        bytes_read = read (fd, (void*)(*buf + bytes_total), BUF_SIZE);
        switch (bytes_read) {
        case -1:
            g_warning ("read failed on fd %d: %s", fd, strerror (errno));
            break;
        case 0:
            g_info ("read returned 0 bytes -> fd %d closed", fd);
            break;
        default:
            bytes_total += bytes_read;
            break;
        }
    } while (bytes_read == BUF_SIZE && bytes_total < BUF_MAX);
    *size = bytes_total;
    return bytes_read;
}
