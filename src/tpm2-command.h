/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TPM2_COMMAND_H
#define TPM2_COMMAND_H

#include <glib-object.h>
#include <sapi/tpm20.h>

#include "connection.h"

G_BEGIN_DECLS

typedef struct _Tpm2CommandClass {
    GObjectClass    parent;
} Tpm2CommandClass;

typedef struct _Tpm2Command {
    GObject         parent_instance;
    TPMA_CC         attributes;
    Connection     *connection;
    guint8         *buffer;
    size_t          buffer_size;
} Tpm2Command;

#include "command-attrs.h"

#define TYPE_TPM2_COMMAND            (tpm2_command_get_type      ())
#define TPM2_COMMAND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TPM2_COMMAND, Tpm2Command))
#define TPM2_COMMAND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TPM2_COMMAND, Tpm2CommandClass))
#define IS_TPM2_COMMAND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TPM2_COMMAND))
#define IS_TPM2_COMMAND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TPM2_COMMAND))
#define TPM2_COMMAND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TPM2_COMMAND, Tpm2CommandClass))

/*
 * Helper macros to extract fields from an authorization. The 'ptr' parameter
 * is a pointer to the start of the authorization.
 */
#define AUTH_HANDLE_GET(ptr) (be32toh (*(TPM_HANDLE*)ptr))
#define AUTH_NONCE_SIZE_OFFSET(ptr) (ptr + sizeof (TPM_HANDLE))
#define AUTH_NONCE_SIZE_GET(ptr) (be16toh (*(TPM_HANDLE*)AUTH_NONCE_SIZE_OFFSET (ptr)))
#define AUTH_NONCE_PTR(ptr)  (AUTH_NONCE_SIZE_OFFSET (ptr) + sizeof (UINT16))
#define AUTH_SESSION_ATTRS_OFFSET(ptr) \
    (AUTH_NONCE_PTR (ptr) + AUTH_NONCE_SIZE_GET (ptr))
#define AUTH_SESSION_ATTRS_GET(ptr) (*(TPMA_SESSION*)AUTH_SESSION_ATTRS_OFFSET (ptr))
#define AUTH_AUTH_SIZE_OFFSET(ptr) (AUTH_ATTRS_OFFSET (ptr) + sizeof (UINT8))
#define AUTH_AUTH_SIZE_GET(ptr) (be16toh (*(UINT16*)AUTH_SESSION_SIZE_OFFSET (ptr)))
#define AUTH_AUTH_OFFSET(ptr) (AUTH_AUTH_SIZE_OFFSET (ptr) + sizeof (UINT16))

GType                 tpm2_command_get_type        (void);
Tpm2Command*          tpm2_command_new             (Connection      *connection,
                                                    guint8           *buffer,
                                                    size_t            size,
                                                    TPMA_CC           attrs);
TPMA_CC               tpm2_command_get_attributes  (Tpm2Command      *command);
guint8*               tpm2_command_get_buffer      (Tpm2Command      *command);
TPM_CC                tpm2_command_get_code        (Tpm2Command      *command);
guint8                tpm2_command_get_handle_count (Tpm2Command     *command);
TPM_HANDLE            tpm2_command_get_handle      (Tpm2Command      *command,
                                                    guint8            handle_number);
gboolean              tpm2_command_get_handles     (Tpm2Command      *command,
                                                    TPM_HANDLE        handles[],
                                                    guint8            count);
gboolean              tpm2_command_set_handle      (Tpm2Command      *command,
                                                    TPM_HANDLE        handle,
                                                    guint8            handle_number);
gboolean              tpm2_command_set_handles     (Tpm2Command      *command,
                                                    TPM_HANDLE        handles[],
                                                    guint8            count);
TPM_RC                tpm2_command_get_flush_handle (Tpm2Command     *command,
                                                     TPM_HANDLE      *handle);
guint32               tpm2_command_get_size        (Tpm2Command      *command);
TPMI_ST_COMMAND_TAG   tpm2_command_get_tag         (Tpm2Command      *command);
Connection*           tpm2_command_get_connection  (Tpm2Command      *command);
TPM_CAP               tpm2_command_get_cap         (Tpm2Command      *command);
UINT32                tpm2_command_get_prop        (Tpm2Command      *command);
UINT32                tpm2_command_get_prop_count  (Tpm2Command      *command);
gboolean              tpm2_command_has_auths       (Tpm2Command      *command);
UINT32                tpm2_command_get_auths_size  (Tpm2Command      *command);
gboolean              tpm2_command_foreach_auth    (Tpm2Command      *command,
                                                    GFunc             func,
                                                    gpointer          user_data);

G_END_DECLS

#endif /* TPM2_COMMAND_H */
