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
