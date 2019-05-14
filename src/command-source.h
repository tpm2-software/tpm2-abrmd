/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef COMMAND_SOURCE_H
#define COMMAND_SOURCE_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>

#include "command-attrs.h"
#include "connection-manager.h"
#include "sink-interface.h"
#include "thread.h"

G_BEGIN_DECLS

/* Maximum buffer size for client data. Connections that send a single
 * command larger than this size will be closed.
 */
#define BUF_MAX 4096

typedef struct _CommandSourceClass {
    ThreadClass       parent;
} CommandSourceClass;

typedef struct _CommandSource {
    Thread             parent_instance;
    ConnectionManager *connection_manager;
    CommandAttrs      *command_attrs;
    GMainContext      *main_context;
    GMainLoop         *main_loop;
    GHashTable        *istream_to_source_data_map;
    Sink              *sink;
} CommandSource;

#define TYPE_COMMAND_SOURCE              (command_source_get_type   ())
#define COMMAND_SOURCE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_COMMAND_SOURCE, CommandSource))
#define COMMAND_SOURCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_COMMAND_SOURCE, CommandSourceClass))
#define IS_COMMAND_SOURCE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_COMMAND_SOURCE))
#define IS_COMMAND_SOURCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_COMMAND_SOURCE))
#define COMMAND_SOURCE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_COMMAND_SOURCE, CommandSourceClass))

GType           command_source_get_type          (void);
CommandSource*  command_source_new               (ConnectionManager  *connection_manager,
                                                  CommandAttrs       *command_attrs);
gint            command_source_on_new_connection (ConnectionManager  *connection_manager,
                                                  Connection         *connection,
                                                  CommandSource      *command_source);
/*
 * The following are private functions. They are exposed here for unit
 * testing. Do not call these from anywhere else.
 */
gboolean        command_source_on_input_ready    (GInputStream       *socket,
                                                  gpointer            user_data);
/*
 * Instances of this structure are used to track GSources and their
 * GCancellable objects that have been registered with the
 * GMainContext/Loop. We keep these structures in a GHashTable
 * (socket_to_source_data_map) keyed on the client's GSocket.
 * - When we're notified of a new connection we create a new GSource to
 *   receive a callback when the GSocket associated with the connection is in
 *   the G_IO_IN state.
 * - When we receive a callback for a GSocket that has the G_IO_IN condition
 *   and the peer has closed their connection we use this data (passed to the
 *   callback when the GSource is created and the callback registered) to find
 *   and remove the same structure from the hash table (and free it). This way
 *   when the CommandSource is destroyed we won't have stale GSources hanging
 *   around.
 * - When the CommandSource is destroyed all of the GSources registered with
 *   the GMainContext/Loop must be canceled and freed (see dispose function).
 */
typedef struct {
    CommandSource *self;
    GCancellable  *cancellable;
    GSource       *source;
} source_data_t;


G_END_DECLS
#endif /* COMMAND_SOURCE_H */
