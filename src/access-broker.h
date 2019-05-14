/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef ACCESS_BROKER_H
#define ACCESS_BROKER_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <tss2/tss2_sys.h>

#include "tcti.h"
#include "tpm2-response.h"

G_BEGIN_DECLS

typedef struct _AccessBrokerClass {
    GObjectClass      parent;
} AccessBrokerClass;

typedef struct _AccessBroker {
    GObject                 parent_instance;
    pthread_mutex_t         sapi_mutex;
    TSS2_SYS_CONTEXT       *sapi_context;
    Tcti                   *tcti;
    TPMS_CAPABILITY_DATA    properties_fixed;
    gboolean                initialized;
} AccessBroker;

#include "tpm2-command.h"

#define TYPE_ACCESS_BROKER              (access_broker_get_type ())
#define ACCESS_BROKER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_ACCESS_BROKER, AccessBroker))
#define ACCESS_BROKER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_ACCESS_BROKER, AccessBrokerClass))
#define IS_ACCESS_BROKER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_ACCESS_BROKER))
#define IS_ACCESS_BROKER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_ACCESS_BROKER))
#define ACCESS_BROKER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_ACCESS_BROKER, AccessBrokerClass))

GType              access_broker_get_type       (void);
AccessBroker*      access_broker_new            (Tcti            *tcti);
TSS2_RC            access_broker_init_tpm       (AccessBroker    *broker);
void               access_broker_lock           (AccessBroker    *broker);
void               access_broker_unlock         (AccessBroker    *broker);
Tpm2Response*      access_broker_send_command   (AccessBroker    *broker,
                                                 Tpm2Command     *command,
                                                 TSS2_RC         *rc);
TSS2_RC            access_broker_get_max_command    (AccessBroker   *broker,
                                                     guint32        *value);
TSS2_RC            access_broker_get_max_response   (AccessBroker   *broker,
                                                     guint32        *value);
TSS2_RC            access_broker_get_total_commands (AccessBroker   *broker,
                                                     guint          *value);
TSS2_SYS_CONTEXT*  access_broker_lock_sapi          (AccessBroker   *broker);
TSS2_RC            access_broker_get_trans_object_count (AccessBroker *broker,
                                                         uint32_t     *count);
TSS2_RC            access_broker_context_load           (AccessBroker *broker,
                                                         TPMS_CONTEXT *context,
                                                         TPM2_HANDLE   *handle);
TSS2_RC            access_broker_context_flush          (AccessBroker *broker,
                                                         TPM2_HANDLE    handle);
TSS2_RC            access_broker_context_saveflush      (AccessBroker *broker,
                                                         TPM2_HANDLE    handle,
                                                         TPMS_CONTEXT *context);
TSS2_RC            access_broker_context_save           (AccessBroker *broker,
                                                         TPM2_HANDLE    handle,
                                                         TPMS_CONTEXT *context);
void               access_broker_flush_all_context      (AccessBroker *broker);

G_END_DECLS

#endif
