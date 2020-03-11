/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TPM2_H
#define TPM2_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <tss2/tss2_sys.h>

#include "tcti.h"
#include "tpm2-response.h"

G_BEGIN_DECLS

typedef struct _Tpm2Class {
    GObjectClass      parent;
} Tpm2Class;

typedef struct _Tpm2 {
    GObject                 parent_instance;
    pthread_mutex_t         sapi_mutex;
    TSS2_SYS_CONTEXT       *sapi_context;
    Tcti                   *tcti;
    TPMS_CAPABILITY_DATA    properties_fixed;
    gboolean                initialized;
} Tpm2;

#include "tpm2-command.h"

#define TYPE_TPM2              (tpm2_get_type ())
#define TPM2(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TPM2, Tpm2))
#define TPM2_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TPM2, Tpm2Class))
#define IS_TPM2(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TPM2))
#define IS_TPM2_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TPM2))
#define TPM2_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TPM2, Tpm2Class))

GType tpm2_get_type (void);
Tpm2* tpm2_new (Tcti *tcti);
TSS2_RC tpm2_init_tpm (Tpm2 *tpm2);
void tpm2_lock (Tpm2 *tpm2);
void tpm2_unlock (Tpm2 *tpm2);
Tpm2Response* tpm2_send_command (Tpm2 *tpm2,
                                 Tpm2Command *command,
                                 TSS2_RC *rc);
TSS2_RC tpm2_get_max_response (Tpm2 *tpm2, guint32 *value);
TSS2_SYS_CONTEXT* tpm2_lock_sapi (Tpm2 *tpm2);
TSS2_RC tpm2_get_trans_object_count (Tpm2 *tpm2, uint32_t *count);
TSS2_RC tpm2_context_load (Tpm2 *tpm2,
                           TPMS_CONTEXT *context,
                           TPM2_HANDLE *handle);
TSS2_RC tpm2_context_flush (Tpm2 *tpm2, TPM2_HANDLE handle);
TSS2_RC tpm2_context_saveflush (Tpm2 *tpm2,
                                TPM2_HANDLE handle,
                                TPMS_CONTEXT *context);
TSS2_RC tpm2_context_save (Tpm2 *tpm2,
                           TPM2_HANDLE handle,
                           TPMS_CONTEXT *context);
void tpm2_flush_all_context (Tpm2 *tpm2);
TSS2_RC tpm2_send_tpm_startup (Tpm2 *tpm2);
TSS2_SYS_CONTEXT* sapi_context_init (Tcti *tcti);
TSS2_RC tpm2_flush_all_unlocked (Tpm2 *tpm2,
                                 TSS2_SYS_CONTEXT *sapi_context,
                                 TPM2_RH first,
                                 TPM2_RH last);
TSS2_RC tpm2_get_command_attrs (Tpm2 *tpm2, UINT32 *count, TPMA_CC **attrs);

G_END_DECLS

#endif
