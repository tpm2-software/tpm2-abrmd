/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <tss2/tss2_tpm2_types.h>

#include "access-broker.h"
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
    AccessBroker     *access_broker;
    MessageQueue     *in_queue;
    Sink             *sink;
    SessionList      *session_list;
    GQueue           *abandoned_session_queue;
} ResourceManager;

#define TYPE_RESOURCE_MANAGER              (resource_manager_get_type ())
#define RESOURCE_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_RESOURCE_MANAGER, ResourceManager))
#define RESOURCE_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_RESOURCE_MANAGER, ResourceManagerClass))
#define IS_RESOURCE_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_RESOURCE_MANAGER))
#define IS_RESOURCE_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_RESOURCE_MANAGER))
#define RESOURCE_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_RESOURCE_MANAGER, ResourceManagerClass))

GType                 resource_manager_get_type       (void);
ResourceManager*      resource_manager_new            (AccessBroker *broker,
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

G_END_DECLS
#endif /* RESOURCE_MANAGER_H */
