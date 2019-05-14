/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef IPC_FRONTEND_H
#define IPC_FRONTEND_H

#include <glib-object.h>

#include "connection.h"

G_BEGIN_DECLS

typedef struct _IpcFrontend      IpcFrontend;
typedef struct _IpcFrontendClass IpcFrontendClass;

typedef void (*IpcFrontendConnect)    (IpcFrontend *self,
                                       GMutex      *mutex);
typedef void (*IpcFrontendDisconnect) (IpcFrontend *self);

struct _IpcFrontend {
    GObject             parent;
    GMutex             *init_mutex;
};

struct _IpcFrontendClass {
    GObjectClass          parent;
    IpcFrontendConnect    connect;
    IpcFrontendDisconnect disconnect;
};

#define TYPE_IPC_FRONTEND             (ipc_frontend_get_type       ())
#define IPC_FRONTEND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_IPC_FRONTEND, IpcFrontend))
#define IS_IPC_FRONTEND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_IPC_FRONTEND))
#define IPC_FRONTEND_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_IPC_FRONTEND, IpcFrontendClass))
#define IS_IPC_FRONTEND_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_IPC_FRONTEND))
#define IPC_FRONTEND_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_IPC_FRONTEND, IpcFrontendClass))

GType               ipc_frontend_get_type              (void);
void                ipc_frontend_connect               (IpcFrontend  *self,
                                                        GMutex       *mutex);
void                ipc_frontend_disconnect            (IpcFrontend  *self);
void                ipc_frontend_disconnected_invoke   (IpcFrontend  *self);
void                ipc_frontend_init_guard            (IpcFrontend  *self);

G_END_DECLS
#endif /* IPC_FRONTEND_H */
