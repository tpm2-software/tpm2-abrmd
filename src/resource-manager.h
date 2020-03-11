/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <tss2/tss2_tpm2_types.h>

#include "tpm2.h"
#include "connection-manager.h"
#include "message-queue.h"
#include "session-list.h"
#include "sink-interface.h"
#include "thread.h"

G_BEGIN_DECLS

typedef struct _ResourceManagerClass {
    ThreadClass      parent;
} ResourceManagerClass;

typedef struct _ResourceManager {
    Thread            parent_instance;
    Tpm2 *tpm2;
    MessageQueue     *in_queue;
    Sink             *sink;
    SessionList      *session_list;
} ResourceManager;

#define TYPE_RESOURCE_MANAGER              (resource_manager_get_type ())
#define RESOURCE_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_RESOURCE_MANAGER, ResourceManager))
#define RESOURCE_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_RESOURCE_MANAGER, ResourceManagerClass))
#define IS_RESOURCE_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_RESOURCE_MANAGER))
#define IS_RESOURCE_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_RESOURCE_MANAGER))
#define RESOURCE_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_RESOURCE_MANAGER, ResourceManagerClass))

GType                 resource_manager_get_type       (void);
ResourceManager*      resource_manager_new            (Tpm2 *tpm2,
                                                       SessionList  *session_list);
void                  resource_manager_process_tpm2_command (ResourceManager   *resmgr,
                                                             Tpm2Command       *command);
void                  resource_manager_flushsave_context (gpointer              entry,
                                                          gpointer              resmgr);
TSS2_RC               resource_manager_load_handles    (ResourceManager *resmgr,
                                                        Tpm2Command     *command,
                                                        GSList         **slist);
TSS2_RC               resource_manager_virt_to_phys      (ResourceManager *resmgr,
                                                          Tpm2Command     *command,
                                                          HandleMapEntry  *entry,
                                                          guint8           handle_number);
void                  resource_manager_enqueue           (Sink            *sink,
                                                          GObject         *obj);
void                  resource_manager_remove_connection (ResourceManager *resource_manager,
                                                          Connection      *connection);
TSS2_RC               get_cap_post_process (Tpm2Response *resp);
G_END_DECLS
#endif /* RESOURCE_MANAGER_H */
