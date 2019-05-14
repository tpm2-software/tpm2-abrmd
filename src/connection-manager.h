/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "connection.h"

G_BEGIN_DECLS

#define CONNECTION_MANAGER_MAX 100

typedef struct _ConnectionManagerClass {
    GObjectClass      parent;
} ConnectionManagerClass;

typedef struct _ConnectionManager {
    GObject           parent_instance;
    pthread_mutex_t   mutex;
    GHashTable       *connection_from_istream_table;
    GHashTable       *connection_from_id_table;
    guint             max_connections;
} ConnectionManager;

#define TYPE_CONNECTION_MANAGER              (connection_manager_get_type   ())
#define CONNECTION_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_CONNECTION_MANAGER, ConnectionManager))
#define CONNECTION_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_CONNECTION_MANAGER, ConnectionManagerClass))
#define IS_CONNECTION_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_CONNECTION_MANAGER))
#define IS_CONNECTION_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_CONNECTION_MANAGER))
#define CONNECTION_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_CONNECTION_MANAGER, ConnectionManagerClass))

GType          connection_manager_get_type    (void);
ConnectionManager* connection_manager_new     (guint               max_connections);
gint           connection_manager_insert      (ConnectionManager  *manager,
                                               Connection         *connection);
gint           connection_manager_remove      (ConnectionManager  *manager,
                                               Connection         *connection);
Connection*    connection_manager_lookup_istream (ConnectionManager  *manager,
                                                  GInputStream       *istream);
Connection*    connection_manager_lookup_id   (ConnectionManager  *manager,
                                               gint64              id_in);
gboolean       connection_manager_contains_id (ConnectionManager  *manager,
                                               gint64              id_in);
guint          connection_manager_size        (ConnectionManager  *manager);
gboolean       connection_manager_is_full     (ConnectionManager  *manager);

G_END_DECLS
#endif /* CONNECTION_MANAGER_H */
