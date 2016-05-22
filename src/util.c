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
