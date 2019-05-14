/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "handle-map.h"

G_BEGIN_DECLS

typedef struct _ConnectionClass {
    GObjectClass        parent;
} ConnectionClass;

typedef struct _Connection {
    GObject             parent_instance;
    GIOStream          *iostream;
    guint64             id;
    HandleMap          *transient_handle_map;
} Connection;

#define TYPE_CONNECTION              (connection_get_type ())
#define CONNECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CONNECTION, Connection))
#define CONNECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST  ((klass), TYPE_CONNECTION, ConnectionClass))
#define IS_CONNECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CONNECTION))
#define IS_CONNECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_CONNECTION))
#define CONNECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_CONNECTION, ConnectionClass))

GType            connection_get_type     (void);
Connection*      connection_new          (GIOStream       *iostream,
                                          guint64          id,
                                          HandleMap       *transient_handle_map);
gpointer         connection_key_istream  (Connection      *session);
gpointer         connection_key_id       (Connection      *session);
GIOStream*       connection_get_iostream (Connection      *connection);
HandleMap*       connection_get_trans_map(Connection      *session);
#endif /* CONNECTION_H */
