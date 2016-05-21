#ifndef UTIL_H
#define UTIL_H

#include <glib.h>
#include <sys/types.h>

ssize_t
write_all (const gint    fd,
           const void   *buf,
           const size_t  size);

#endif /* UTIL_H */
