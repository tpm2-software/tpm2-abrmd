/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef UTIL_H
#define UTIL_H

#include <glib.h>
#include <gio/gio.h>
#include <inttypes.h>
#include <tss2/tss2_rc.h>
#include <tss2/tss2_tpm2_types.h>

#include "control-message.h"

/* Use to suppress "unused parameter" warnings: */
#define UNUSED_PARAM(p) ((void)(p))
/* Use to suppress "unused variable" warnings: */
#define UNUSED_VAR(p) ((void)(p))

/* Used to suppress scan-build NULL dereference warnings: */
#ifdef SCANBUILD
#define ASSERT_NON_NULL(x) assert_non_null(x); \
            if ((x) == NULL) return
#else
#define ASSERT_NON_NULL(x) assert_non_null(x)
#endif

/*
 * Substitute  for GNU TEMP_FAILURE_RETRY for environments that
 * don't have the GNU C library.
 */
#define TABRMD_ERRNO_EINTR_RETRY(exp)                   \
  ({                                                    \
    long int __result = 0;                              \
    do {                                                \
      __result = (long int)(exp);                       \
    } while ((__result == -1) && (errno == EINTR));     \
    __result;                                           \
  })

/* set the layer / component to indicate the RC comes from the RM */
#define RM_RC(rc) TSS2_RESMGR_RC_LAYER + rc

/* allocate read blocks in BUF_SIZE increments */
#define UTIL_BUF_SIZE 1024
/* stop allocating at BUF_MAX */
#define UTIL_BUF_MAX  8*UTIL_BUF_SIZE

#define prop_str(val) val ? "set" : "clear"

/*
 * Print warning message for a given response code.
 * Parameters:
 *   cmd: string name of Tss2_* function that produced the RC
 *   rc: response code from function 'cmd'
 */
#define RC_WARN(cmd, rc) \
    g_warning ("[%s:%d] %s failed: %s (RC: 0x%" PRIx32 ")", \
               __FILE__, __LINE__, cmd, Tss2_RC_Decode (rc), rc)

typedef struct {
    char *key;
    char *value;
} key_value_t;

typedef TSS2_RC (*KeyValueFunc) (const key_value_t* key_value,
                                 gpointer user_data);
/*
#define TPM2_CC_FROM_TPMA_CC(attrs) (attrs.val & 0x0000ffff)
#define TPMA_CC_RESERVED(attrs)    (attrs.val & 0x003f0000)
#define TPMA_CC_NV(attrs)          (attrs.val & 0x00400000)
#define TPMA_CC_EXTENSIVE(attrs)   (attrs.val & 0x00800000)
#define TPMA_CC_FLUSHED(attrs)     (attrs.val & 0x01000000)
#define TPMA_CC_CHANDLES(attrs)    (attrs.val & 0x02000000)
#define TPMA_CC_RHANDLES(attrs)    (attrs.val & 0x10000000)
#define TPMA_CC_V(attrs)           (attrs.val & 0x20000000)
#define TPMA_CC_RES(attrs)         (attrs.val & 0xc0000000)
*/

ssize_t     write_all                       (GOutputStream    *ostream,
                                             const uint8_t    *buf,
                                             const size_t      size);
int         read_data                       (GInputStream     *istream,
                                             size_t           *index,
                                             uint8_t          *buf,
                                             size_t            count);
int         read_tpm_buffer                 (GInputStream     *istream,
                                             size_t           *index,
                                             uint8_t          *buf,
                                             size_t            buf_size);
uint8_t*    read_tpm_buffer_alloc           (GInputStream     *istream,
                                             size_t           *buf_size);
void        g_debug_bytes                   (uint8_t const    *byte_array,
                                             size_t            array_size,
                                             size_t            width,
                                             size_t            indent);
GIOStream*  create_connection_iostream      (int              *client_fd);
int         create_socket_pair              (int              *fd_a,
                                             int              *fd_b,
                                             int               flags);
void        g_debug_tpma_cc                 (TPMA_CC           tpma_cc);
TSS2_RC     parse_key_value_string (char *kv_str,
                                    KeyValueFunc callback,
                                    gpointer user_data);

#endif /* UTIL_H */
