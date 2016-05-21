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
        if (written <= 0) {
            goto out;
        } else {
            g_debug ("wrote %d bytes to fd %d", written, fd);
        }
        written_total += written;
    } while (written_total < size);
out:
    return written;

}
