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
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection.h"
#include "util.h"

G_DEFINE_TYPE (Connection, connection, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_ID,
    PROP_SOCKET,
    PROP_TRANSIENT_HANDLE_MAP,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };

static void
connection_set_property (GObject       *object,
                         guint          property_id,
                         const GValue  *value,
                         GParamSpec    *pspec)
{
    Connection *self = CONNECTION (object);

    g_debug ("connection_set_property");
    switch (property_id) {
    case PROP_ID:
        self->id = g_value_get_uint64 (value);
        g_debug ("Connection 0x%" PRIxPTR " set id to 0x%" PRIx64,
                 (uintptr_t)self, self->id);
        break;
    case PROP_SOCKET:
        self->socket = G_SOCKET (g_value_dup_object (value));
        self->fd = g_socket_get_fd (self->socket);
        g_debug ("Connection 0x%" PRIxPTR " set socket to %" PRIxPTR,
                 (uintptr_t)self, (uintptr_t)self->socket);
        break;
    case PROP_TRANSIENT_HANDLE_MAP:
        self->transient_handle_map = g_value_get_object (value);
        g_object_ref (self->transient_handle_map);
        g_debug ("Connection 0x%" PRIxPTR " set trans_handel_map to 0x%"
                  PRIxPTR, (uintptr_t)self,
                  (uintptr_t)self->transient_handle_map);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
connection_get_property (GObject     *object,
                         guint        property_id,
                         GValue      *value,
                         GParamSpec  *pspec)
{
    Connection *self = CONNECTION (object);

    g_debug ("connection_get_property");
    switch (property_id) {
    case PROP_ID:
        g_value_set_uint64 (value, self->id);
        break;
    case PROP_SOCKET:
        g_value_set_object (value, self->socket);
        break;
    case PROP_TRANSIENT_HANDLE_MAP:
        g_value_set_object (value, self->transient_handle_map);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
connection_init (Connection *connection)
{ /* noop */ }

static void
connection_finalize (GObject *obj)
{
    Connection *connection = CONNECTION (obj);

    g_debug ("connection_finalize: 0x%" PRIxPTR, (uintptr_t)connection);
    if (connection == NULL)
        return;
    g_clear_object (&connection->socket);
    g_object_unref (connection->transient_handle_map);
    if (connection_parent_class)
        G_OBJECT_CLASS (connection_parent_class)->finalize (obj);
}

static void
connection_class_init (ConnectionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("connection_class_init");
    if (connection_parent_class == NULL)
        connection_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = connection_finalize;
    object_class->get_property = connection_get_property;
    object_class->set_property = connection_set_property;

    obj_properties [PROP_ID] =
        g_param_spec_uint64 ("id",
                             "connection identifier",
                             "Unique identifier for the connection",
                             0,
                             UINT64_MAX,
                             0,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SOCKET] =
        g_param_spec_object ("gsocket",
                             "GSocket",
                             "Reference to GSocket for exchanging data with client",
                             G_TYPE_SOCKET,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_TRANSIENT_HANDLE_MAP] =
        g_param_spec_object ("transient-handle-map",
                             "HandleMap",
                             "HandleMap object to map handles to transient object contexts",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/* Create a pipe and return the recv and send fds. */
int
create_fd_pair (int *client_fd,
                int *server_fd,
                int  flags)
{
    int ret, fds[2] = { 0, };

    ret = socketpair (PF_LOCAL, SOCK_STREAM, 0, fds);
    if (ret == -1)
        return ret;
    *client_fd = fds[0];
    *server_fd = fds[1];
    fcntl (*client_fd, flags);
    fcntl (*server_fd, flags);
    return 0;
}
/* CreateConnection builds two pipes for communicating with client
 * applications. It's provided with an array of two integers by the caller
 * and it returns this array populated with the receiving and sending pipe fds
 * respectively.
 */
Connection*
connection_new (gint       *client_fd,
                guint64     id,
                HandleMap  *transient_handle_map)
{

    g_info ("CreateConnection");
    GError  *error;
    GObject *object;
    GSocket *socket;
    int ret, server_fd;

    g_debug ("connection_new creating pipe pairs");
    ret = create_fd_pair (client_fd, &server_fd, O_CLOEXEC);
    if (ret == -1)
        g_error ("CreateConnection failed to make fd pair %s", strerror (errno));
    /* Make the fds used by the server non-blocking, the client will have to
     * set its own flags.
     */
    socket = g_socket_new_from_fd (server_fd, &error);
    if (socket == NULL) {
        /* this is guaranteed to be non-NULL by glib but assert anyways */
        g_assert (error != NULL);
        g_warning ("Failed to create GSocket from fd %d: %s",
                   server_fd, error->message);
        close (server_fd);
        *client_fd = 0;
        return NULL;
    }
    g_socket_set_blocking (socket, TRUE);
    object = g_object_new (TYPE_CONNECTION,
                           "id", id,
                           "gsocket", socket,
                           "transient-handle-map", transient_handle_map,
                           NULL);
    g_object_unref (socket);
    return CONNECTION (object);
}

gpointer
connection_key_fd (Connection *connection)
{
    return &connection->fd;
}

GSocket*
connection_get_gsocket (Connection *connection)
{
    return connection->socket;
}

gpointer
connection_key_id (Connection *connection)
{
    return &connection->id;
}
/*
 * Return a reference to the HandleMap for transient handles to the caller.
 * We increment the reference count on this object before returning it. The
 * caller *must* decrement the reference count when they're done using the
 * object.
 */
HandleMap*
connection_get_trans_map (Connection *connection)
{
    g_object_ref (connection->transient_handle_map);
    return connection->transient_handle_map;
}
