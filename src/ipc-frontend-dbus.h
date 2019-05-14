/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef IPC_FRONTEND_DBUS_H
#define IPC_FRONTEND_DBUS_H

#include <glib-object.h>
#include <gio/gio.h>

#include "connection-manager.h"
#include "ipc-frontend.h"
#include "random.h"
#include "tabrmd-generated.h"

G_BEGIN_DECLS

#define IPC_FRONTEND_DBUS_NAME_DEFAULT "com.intel.tss2.Tabrmd"
#define IPC_FRONTEND_DBUS_TYPE_DEFAULT G_BUS_TYPE_SYSTEM

typedef struct _IpcFrontendDbusClass {
   IpcFrontendClass     parent;
} IpcFrontendDbusClass;

typedef struct _IpcFrontendDbus
{
    IpcFrontend        parent_instance;
    /* data set by GObject properties */
    gchar             *bus_name;
    GBusType           bus_type;
    /* private data */
    gboolean           dbus_name_acquired;
    guint              dbus_name_owner_id;
    guint              max_transient_objects;
    ConnectionManager *connection_manager;
    GDBusProxy        *dbus_daemon_proxy;
    Random            *random;
    TctiTabrmd        *skeleton;
} IpcFrontendDbus;

#define TYPE_IPC_FRONTEND_DBUS             (ipc_frontend_dbus_get_type       ())
#define IPC_FRONTEND_DBUS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_IPC_FRONTEND_DBUS, IpcFrontendDbus))
#define IPC_FRONTEND_DBUS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_IPC_FRONTEND_DBUS, IpcFrontendDbusClass))
#define IS_IPC_FRONTEND_DBUS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_IPC_FRONTEND_DBUS))
#define IS_IPC_FRONTEND_DBUS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_IPC_FRONTEND_DBUS))
#define IPC_FRONTEND_DBUS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_IPC_FRONTEND_DBUS, IpcFrontendDbusClass))

GType            ipc_frontend_dbus_get_type   (void);
IpcFrontendDbus* ipc_frontend_dbus_new        (GBusType           bus_type,
                                               gchar const       *bus_name,
                                               ConnectionManager *connection_manager,
                                               guint              max_trans,
                                               Random            *random);
void             ipc_frontend_dbus_connect    (IpcFrontendDbus   *self,
                                               GMutex            *init_mutex);
void             ipc_frontend_dbus_disconnect (IpcFrontendDbus   *self);

G_END_DECLS
#endif /* IPC_FRONTEND_DBUS_H */
