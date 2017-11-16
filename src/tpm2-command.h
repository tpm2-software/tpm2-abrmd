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

#define TPM2_COMMAND_MAX_HANDLES     3

GType                 tpm2_command_get_type        (void);
Tpm2Command*          tpm2_command_new             (Connection      *connection,
                                                    guint8           *buffer,
                                                    size_t            size,
                                                    TPMA_CC           attrs);
TPMA_CC               tpm2_command_get_attributes  (Tpm2Command      *command);
TPMA_SESSION          tpm2_command_get_auth_attrs  (Tpm2Command      *command,
                                                    size_t            auth_offset);
TPM2_HANDLE            tpm2_command_get_auth_handle (Tpm2Command      *command,
                                                    size_t            offset);
guint8*               tpm2_command_get_buffer      (Tpm2Command      *command);
TPM2_CC                tpm2_command_get_code        (Tpm2Command      *command);
guint8                tpm2_command_get_handle_count (Tpm2Command     *command);
TPM2_HANDLE            tpm2_command_get_handle      (Tpm2Command      *command,
                                                    guint8            handle_number);
gboolean              tpm2_command_get_handles     (Tpm2Command      *command,
                                                    TPM2_HANDLE        handles[],
                                                    size_t           *count);
gboolean              tpm2_command_set_handle      (Tpm2Command      *command,
                                                    TPM2_HANDLE        handle,
                                                    guint8            handle_number);
gboolean              tpm2_command_set_handles     (Tpm2Command      *command,
                                                    TPM2_HANDLE        handles[],
                                                    guint8            count);
TSS2_RC                tpm2_command_get_flush_handle (Tpm2Command     *command,
                                                     TPM2_HANDLE      *handle);
guint32               tpm2_command_get_size        (Tpm2Command      *command);
TPMI_ST_COMMAND_TAG   tpm2_command_get_tag         (Tpm2Command      *command);
Connection*           tpm2_command_get_connection  (Tpm2Command      *command);
TPM2_CAP               tpm2_command_get_cap         (Tpm2Command      *command);
UINT32                tpm2_command_get_prop        (Tpm2Command      *command);
UINT32                tpm2_command_get_prop_count  (Tpm2Command      *command);
gboolean              tpm2_command_has_auths       (Tpm2Command      *command);
UINT32                tpm2_command_get_auths_size  (Tpm2Command      *command);
gboolean              tpm2_command_foreach_auth    (Tpm2Command      *command,
                                                    GFunc             func,
                                                    gpointer          user_data);

G_END_DECLS

#endif /* TPM2_COMMAND_H */
