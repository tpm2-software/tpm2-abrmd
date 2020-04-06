/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TPM2_COMMAND_H
#define TPM2_COMMAND_H

#include <glib-object.h>
#include <tss2/tss2_tpm2_types.h>

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
Tpm2Command*          tpm2_command_new_context_save (TPM2_HANDLE);
Tpm2Command*          tpm2_command_new_context_load (uint8_t *buf,
                                                     size_t size);
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
