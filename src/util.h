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

#define prop_str(val) val ? "set" : "clear"
/*
#define TPM_CC_FROM_TPMA_CC(attrs) (attrs.val & 0x0000ffff)
#define TPMA_CC_RESERVED(attrs)    (attrs.val & 0x003f0000)
#define TPMA_CC_NV(attrs)          (attrs.val & 0x00400000)
#define TPMA_CC_EXTENSIVE(attrs)   (attrs.val & 0x00800000)
#define TPMA_CC_FLUSHED(attrs)     (attrs.val & 0x01000000)
#define TPMA_CC_CHANDLES(attrs)    (attrs.val & 0x02000000)
#define TPMA_CC_RHANDLES(attrs)    (attrs.val & 0x10000000)
#define TPMA_CC_V(attrs)           (attrs.val & 0x20000000)
#define TPMA_CC_RES(attrs)         (attrs.val & 0xc0000000)
*/

ssize_t     write_all                       (gint const        fd,
                                             void const       *buf,
                                             size_t const      size);
ssize_t     read_till_block                 (gint const        fd,
                                             guint8          **buf,
                                             size_t           *size);
void        process_control_code            (ControlCode       code);
uint8_t*    read_tpm_command_header_from_fd (int               fd,
                                             uint8_t          *buf,
                                             size_t            buf_size);
uint8_t*    read_tpm_command_body_from_fd   (int               fd,
                                             uint8_t          *buf,
                                             size_t            body_size);
uint8_t*    read_tpm_command_from_fd        (int               fd,
                                             UINT32           *command_size);

void        g_debug_bytes                   (uint8_t const    *byte_array,
                                             size_t            array_size,
                                             size_t            width,
                                             size_t            indent);

#endif /* UTIL_H */
