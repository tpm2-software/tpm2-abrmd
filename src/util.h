#ifndef UTIL_H
#define UTIL_H

#include <glib.h>
#include <sys/types.h>

/* allocate read blocks in BUF_SIZE increments */
#define BUF_SIZE 1024
/* stop allocating at BUF_MAX */
#define BUF_MAX  8*BUF_SIZE

ssize_t
write_all (const gint    fd,
           const void   *buf,
           const size_t  size);
ssize_t
read_till_short (const gint   fd,
                 guint8     **buf,
                 size_t      *size);

#endif /* UTIL_H */
