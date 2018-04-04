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
    PROP_IO_STREAM,
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
    case PROP_IO_STREAM:
        self->iostream = G_IO_STREAM (g_value_dup_object (value));
        g_debug ("Connection 0x%" PRIxPTR " set socket to %" PRIxPTR,
                 (uintptr_t)self, (uintptr_t)self->iostream);
        break;
    case PROP_TRANSIENT_HANDLE_MAP:
        self->transient_handle_map = g_value_get_object (value);
        g_object_ref (self->transient_handle_map);
        g_debug ("Connection 0x%" PRIxPTR " set transient_handle_map to 0x%"
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
    case PROP_IO_STREAM:
        g_value_set_object (value, self->iostream);
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
{
    UNUSED_PARAM(connection);
    /* noop */
}

static void
connection_dispose (GObject *obj)
{
    Connection *connection = CONNECTION (obj);

    g_clear_object (&connection->iostream);
    g_object_unref (connection->transient_handle_map);

    G_OBJECT_CLASS (connection_parent_class)->dispose (obj);
}

static void
connection_class_init (ConnectionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("connection_class_init");
    if (connection_parent_class == NULL)
        connection_parent_class = g_type_class_peek_parent (klass);

    object_class->dispose      = connection_dispose;
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
    obj_properties [PROP_IO_STREAM] =
        g_param_spec_object ("iostream",
                             "GIOStream",
                             "Reference to GIOStream for exchanging data with client",
                             G_TYPE_IO_STREAM,
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
/* CreateConnection builds two pipes for communicating with client
 * applications. It's provided with an array of two integers by the caller
 * and it returns this array populated with the receiving and sending pipe fds
 * respectively.
 */
Connection*
connection_new (GIOStream  *iostream,
                guint64     id,
                HandleMap  *transient_handle_map)
{
    return CONNECTION (g_object_new (TYPE_CONNECTION,
                                     "id", id,
                                     "iostream", iostream,
                                     "transient-handle-map", transient_handle_map,
                                     NULL));
}

gpointer
connection_key_istream (Connection *connection)
{
    return g_io_stream_get_input_stream (connection->iostream);
}

GIOStream*
connection_get_iostream (Connection *connection)
{
    return connection->iostream;
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
