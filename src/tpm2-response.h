/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TPM2_RESPONSE_H
#define TPM2_RESPONSE_H

#include <glib-object.h>
#include <tss2/tss2_tpm2_types.h>

#include "connection.h"
#include "session-entry.h"

G_BEGIN_DECLS

typedef struct _Tpm2ResponseClass {
    GObjectClass    parent;
} Tpm2ResponseClass;

typedef struct _Tpm2Response {
    GObject         parent_instance;
    Connection     *connection;
    guint8         *buffer;
    size_t          buffer_size;
    TPMA_CC         attributes;
} Tpm2Response;

#define TPM_RESPONSE_HEADER_SIZE (sizeof (TPM2_ST) + sizeof (UINT32) + sizeof (TPM2_RC))

#define TYPE_TPM2_RESPONSE            (tpm2_response_get_type      ())
#define TPM2_RESPONSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TPM2_RESPONSE, Tpm2Response))
#define TPM2_RESPONSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TPM2_RESPONSE, Tpm2ResponseClass))
#define IS_TPM2_RESPONSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TPM2_RESPONSE))
#define IS_TPM2_RESPONSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TPM2_RESPONSE))
#define TPM2_RESPONSE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TPM2_RESPONSE, Tpm2ResponseClass))

GType               tpm2_response_get_type      (void);
Tpm2Response*       tpm2_response_new           (Connection      *connection,
                                                 guint8          *buffer,
                                                 size_t           buffer_size,
                                                 TPMA_CC          attributes);
Tpm2Response*       tpm2_response_new_rc        (Connection      *connection,
                                                 TSS2_RC           rc);
Tpm2Response* tpm2_response_new_context_save (Connection *connection,
                                              SessionEntry *entry);
Tpm2Response* tpm2_response_new_context_load (Connection *connection,
                                              SessionEntry *entry);
TPMA_CC             tpm2_response_get_attributes (Tpm2Response   *response);
guint8*             tpm2_response_get_buffer    (Tpm2Response    *response);
TSS2_RC              tpm2_response_get_code      (Tpm2Response    *response);
TPM2_HANDLE          tpm2_response_get_handle    (Tpm2Response    *response);
TPM2_HT              tpm2_response_get_handle_type (Tpm2Response  *response);
gboolean            tpm2_response_has_handle    (Tpm2Response    *response);
guint32             tpm2_response_get_size      (Tpm2Response    *response);
TPM2_ST              tpm2_response_get_tag       (Tpm2Response    *response);
Connection*         tpm2_response_get_connection (Tpm2Response    *response);
void                tpm2_response_set_handle    (Tpm2Response    *response,
                                                 TPM2_HANDLE       handle);

G_END_DECLS

#endif /* TPM2_RESPONSE_H */
