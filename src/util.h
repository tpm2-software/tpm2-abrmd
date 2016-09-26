#ifndef UTIL_H
#define UTIL_H

#include <glib.h>
#include <sys/types.h>
#include <tpm20.h>

#include "control-message.h"

/* allocate read blocks in BUF_SIZE increments */
#define UTIL_BUF_SIZE 1024
/* stop allocating at BUF_MAX */
#define UTIL_BUF_MAX  8*UTIL_BUF_SIZE

ssize_t     write_all                       (gint const        fd,
                                             void const       *buf,
                                             size_t const      size);
ssize_t     read_till_block                 (gint const        fd,
                                             guint8          **buf,
                                             size_t           *size);
void        process_control_message         (ControlMessage   *msg);
uint8_t*    read_tpm_command_header_from_fd (int               fd,
                                             uint8_t          *buf,
                                             size_t            buf_size);
uint8_t*    read_tpm_command_body_from_fd   (int               fd,
                                             uint8_t          *buf,
                                             size_t            body_size);
uint8_t*    read_tpm_command_from_fd        (int               fd,
                                             UINT32           *command_size);


#endif /* UTIL_H */
