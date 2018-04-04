/*
 * Copyright (c) 2017, Intel Corporation
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

#include <inttypes.h>

#include "util.h"
#include "ipc-frontend.h"

G_DEFINE_ABSTRACT_TYPE (IpcFrontend, ipc_frontend, G_TYPE_OBJECT);

enum {
    SIGNAL_0,
    SIGNAL_DISCONNECTED,
    N_SIGNALS,
};
static guint signals [N_SIGNALS] = { 0 };

static void
ipc_frontend_init (IpcFrontend *ipc_frontend)
{
    UNUSED_PARAM(ipc_frontend);
    /* noop, required by G_DEFINE_ABSTRACT_TYPE */
}
static void
ipc_frontend_class_init (IpcFrontendClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (ipc_frontend_parent_class == NULL)
        ipc_frontend_parent_class = g_type_class_peek_parent (klass);
    klass->connect = ipc_frontend_connect;
    klass->disconnect = ipc_frontend_disconnect;
    signals [SIGNAL_DISCONNECTED] =
        g_signal_new ("disconnected",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      0,
                      NULL);
}
/*
 * The init_mutex is not meant to be held for any length of time. It's only
 * used as a way for an external entity to prevent the IpcFrontend from accepting
 * new connections until the command processing machinery is setup.
 */
void
ipc_frontend_init_guard (IpcFrontend *self)
{
    if (self->init_mutex) {
        g_mutex_lock (self->init_mutex);
        g_mutex_unlock (self->init_mutex);
    }
}
/*
 * This function is boiler plate. It is used to invoke the function pointed
 * to by the 'connect' member data / function pointer in the class object.
 * Derived classes should set this pointer in their class initializer. This
 * function will use this function pointer to invoke the right function in
 * derived classes when they're being referenced through this abstract type.
 */
void
ipc_frontend_connect (IpcFrontend *self,
                      GMutex      *init_mutex)
{
    g_debug ("ipc_frontend_connect: 0x%" PRIxPTR, (uintptr_t)self);
    IPC_FRONTEND_GET_CLASS (self)->connect (self, init_mutex);
}
/*
 * This function is used to invoke the 'disconnect' function from a child
 * class. It explicitly does *not* cause the 'disconnected' event to be
 * emitted. The classes inheriting from this class are in a better position
 * to know when they're actually disconnected and thus when the signal should
 * be emitted.
 */
void
ipc_frontend_disconnect (IpcFrontend *self)
{
    g_debug ("ipc_frontend_disconnect: 0x%" PRIxPTR, (uintptr_t)self);
    IPC_FRONTEND_GET_CLASS (self)->disconnect (self);
}
void
ipc_frontend_disconnected_invoke (IpcFrontend *ipc_frontend)
{
    g_signal_emit (ipc_frontend,
                   signals [SIGNAL_DISCONNECTED],
                   0);
}
