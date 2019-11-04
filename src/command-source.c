/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connection-manager.h"
#include "command-source.h"
#include "source-interface.h"
#include "tpm2-command.h"
#include "tpm2-header.h"
#include "util.h"

#ifndef G_SOURCE_FUNC
#define G_SOURCE_FUNC(x) ((GSourceFunc)(void*)x)
#endif

enum {
    PROP_0,
    PROP_COMMAND_ATTRS,
    PROP_CONNECTION_MANAGER,
    PROP_SINK,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };

/**
 * Function implementing the Source interface. Adds a sink for the source
 * to pass data to.
 */
void
command_source_add_sink (Source      *self,
                         Sink        *sink)
{
    CommandSource *src = COMMAND_SOURCE (self);
    GValue value = G_VALUE_INIT;

    g_debug (__func__);
    g_value_init (&value, G_TYPE_OBJECT);
    g_value_set_object (&value, sink);
    g_object_set_property (G_OBJECT (src), "sink", &value);
    g_value_unset (&value);
}
/*
 * This function initializes the SourceInterface. This is just registering
 * a function pointer with the interface.
 */
static void
command_source_source_interface_init (gpointer g_iface)
{
    SourceInterface *source = (SourceInterface*)g_iface;
    source->add_sink = command_source_add_sink;
}
/*
 * This is a callback function used to clean up memory used by the
 * source_data_t structure. It's called by the GHashTable when removing
 * source_data_t values.
 */
static void
source_data_free (gpointer data)
{
    source_data_t *source_data = (source_data_t*)data;
    g_object_unref (source_data->cancellable);
    g_source_unref (source_data->source);
    g_free (source_data);
}
/*
 * Initialize a CommandSource instance.
 */
static void
command_source_init (CommandSource *source)
{
    source->main_context = g_main_context_new ();
    source->main_loop = g_main_loop_new (source->main_context, FALSE);
    /*
     * GHashTable mapping a GSocket to an instance of the source_data_t
     * structure. The socket is the I/O mechanism for communicating with a
     * client (from Connection object), and the source_data_t instance is
     * a collection of data that we use to manage callbacks for I/O events
     * from the main loop.
     * The hash table owns a reference to the socket (key) and it owns
     * the structure held in the value (it will be freed when removed).
     */
    source->istream_to_source_data_map =
        g_hash_table_new_full (g_direct_hash,
                               g_direct_equal,
                               g_object_unref,
                               source_data_free);
}

G_DEFINE_TYPE_WITH_CODE (
    CommandSource,
    command_source,
    TYPE_THREAD,
    G_IMPLEMENT_INTERFACE (TYPE_SOURCE,
                           command_source_source_interface_init)
    );

static void
command_source_set_property (GObject       *object,
                              guint          property_id,
                              GValue const  *value,
                              GParamSpec    *pspec)
{
    CommandSource *self = COMMAND_SOURCE (object);

    g_debug (__func__);
    switch (property_id) {
    case PROP_COMMAND_ATTRS:
        self->command_attrs = COMMAND_ATTRS (g_value_dup_object (value));
        break;
    case PROP_CONNECTION_MANAGER:
        self->connection_manager = CONNECTION_MANAGER (g_value_get_object (value));
        break;
    case PROP_SINK:
        /* be rigid initially, add flexiblity later if we need it */
        if (self->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        self->sink = SINK (g_value_get_object (value));
        g_object_ref (self->sink);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
command_source_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
    CommandSource *self = COMMAND_SOURCE (object);

    g_debug (__func__);
    switch (property_id) {
    case PROP_COMMAND_ATTRS:
        g_value_set_object (value, self->command_attrs);
        break;
    case PROP_CONNECTION_MANAGER:
        g_value_set_object (value, self->connection_manager);
        break;
    case PROP_SINK:
        g_value_set_object (value, self->sink);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/*
 * This function is invoked by the GMainLoop thread when a client GSocket has
 * data ready. This is what makes the CommandSource a source (of Tpm2Commands).
 * Here we take the GSocket, extract the command body from the it and
 * transform it to a Tpm2Command. Most of the details are handled by utility
 * functions further down the stack.
 *
 * If an error occurs while getting the command from the GSocket the connection
 * with the client will be closed and removed from the ConnectionManager.
 * Additionally the function will return FALSE and the GSource will no longer
 * monitor the GSocket for the G_IO_IN condition.
 */
gboolean
command_source_on_input_ready (GInputStream *istream,
                               gpointer      user_data)
{
    source_data_t *data = (source_data_t*)user_data;
    Connection    *connection;
    Tpm2Command   *command;
    TPMA_CC        attributes = { 0 };
    uint8_t       *buf;
    size_t         buf_size;

    g_debug (__func__);
    connection =
        connection_manager_lookup_istream (data->self->connection_manager,
                                           istream);
    if (connection == NULL) {
        g_error ("%s: failed to get connection associated with istream",
                 __func__);
    }
    buf = read_tpm_buffer_alloc (istream, &buf_size);
    if (buf == NULL) {
        goto fail_out;
    }
    attributes = command_attrs_from_cc (data->self->command_attrs,
                                        get_command_code (buf));
    command = tpm2_command_new (connection, buf, buf_size, attributes);
    if (command != NULL) {
        sink_enqueue (data->self->sink, G_OBJECT (command));
        /* the sink now owns this message */
        g_object_unref (command);
    } else {
        goto fail_out;
    }
    g_object_unref (connection);
    return G_SOURCE_CONTINUE;
fail_out:
    if (buf != NULL) {
        g_free (buf);
    }
    g_debug ("%s: removing connection from connection_manager", __func__);
    connection_manager_remove (data->self->connection_manager,
                               connection);
    ControlMessage *msg =
        control_message_new_with_object (CONNECTION_REMOVED,
                                         G_OBJECT (connection));
    sink_enqueue (data->self->sink, G_OBJECT (msg));
    g_object_unref (msg);
    g_object_unref (connection);
    /*
     * Remove data from hash table which includes the GCancellable associated
     * with the G_IN_IO condition source. Don't call the cancellable though
     * since we're letting this source die at the end of this function
     * (returning FALSE).
     */
    g_debug ("%s: removing GCancellable", __func__);
    g_hash_table_remove (data->self->istream_to_source_data_map, istream);
    return G_SOURCE_REMOVE;
}
/*
 * This is a callback function invoked by the ConnectionManager when a new
 * Connection object is added to it. It creates and sets up the GIO
 * machinery needed to monitor the Connection for I/O events.
 */
gint
command_source_on_new_connection (ConnectionManager   *connection_manager,
                                  Connection          *connection,
                                  CommandSource       *self)
{
    GIOStream *iostream;
    GPollableInputStream *istream;
    source_data_t *data;
    UNUSED_PARAM(connection_manager);

    g_info ("%s: adding new connection", __func__);
    /*
     * Take reference to socket, will be freed when the source_data_t
     * structure is freed
     */
    iostream = connection_get_iostream (connection);
    istream = G_POLLABLE_INPUT_STREAM (g_io_stream_get_input_stream (iostream));
    g_object_ref (istream);
    data = g_malloc0 (sizeof (source_data_t));
    data->cancellable = g_cancellable_new ();
    data->source = g_pollable_input_stream_create_source (istream,
                                                          data->cancellable);
    /* we ignore the ID returned since we keep a reference to the source around */
    g_source_attach (data->source, self->main_context);
    data->self = self;
    g_source_set_callback (data->source,
                           G_SOURCE_FUNC (command_source_on_input_ready),
                           data,
                           NULL);
    /*
     * To stop watching this socket for G_IO_IN condition use this GHashTable
     * to look up the GCancellable object. The hash table takes ownership of
     * the reference to the istream and the source_data_t pointer.
     */
    g_hash_table_insert (self->istream_to_source_data_map, istream, data);

    return 0;
}
/*
 * callback for iterating over objects in the member socket_to_source_data_map
 * GHashMap to kill off the GSource. This requires cancelling it, and then
 * calling the GSource destroy function. This should only happen when the
 * CommandSource object is destroyed.
 */
static void
command_source_source_cancel (gpointer key,
                              gpointer value,
                              gpointer user_data)
{
    UNUSED_PARAM(key);
    UNUSED_PARAM(user_data);
    g_debug ("%s", __func__);
    source_data_t *data = (source_data_t*)value;

    g_debug ("%s: canceling cancellable and destroying source", __func__);
    g_cancellable_cancel (data->cancellable);
}
/*
 * GObject dispose function. It's used to unref / release all GObjects held
 * by the CommandSource before chaining up to the parent.
 */
static void
command_source_dispose (GObject *object) {
    CommandSource *self = COMMAND_SOURCE (object);

    g_clear_object (&self->sink);
    g_clear_object (&self->connection_manager);
    g_clear_object (&self->command_attrs);
    /* cancel all outstanding G_IO_IN condition GSources and destroy them */
    if (self->istream_to_source_data_map != NULL) {
        g_hash_table_foreach (self->istream_to_source_data_map,
                              command_source_source_cancel,
                              NULL);
    }
    g_clear_pointer (&self->istream_to_source_data_map, g_hash_table_unref);
    if (self->main_loop != NULL && g_main_loop_is_running (self->main_loop)) {
        g_main_loop_quit (self->main_loop);
    }
    g_clear_pointer (&self->main_loop, g_main_loop_unref);
    g_clear_pointer (&self->main_context, g_main_context_unref);
    G_OBJECT_CLASS (command_source_parent_class)->dispose (object);
}

/*
 * GObject finalize function releases all resources not freed in 'dispose'
 * & chain up to parent.
 */
static void
command_source_finalize (GObject  *object)
{
    G_OBJECT_CLASS (command_source_parent_class)->finalize (object);
}
/*
 * Cause the GMainLoop to stop monitoring whatever GSources are attached to
 * it and return
 */
static void
command_source_unblock (Thread *self)
{
    CommandSource *source = COMMAND_SOURCE (self);
    g_main_loop_quit (source->main_loop);
}
/*
 * This function creates it's very own GMainLoop thread. This is used to
 * monitor client connections for incoming data (TPM2 command buffers).
 */
void*
command_source_thread (void *data)
{
    CommandSource *source;

    g_assert (data != NULL);
    source = COMMAND_SOURCE (data);
    g_assert (source->main_loop != NULL);

    if (!g_main_loop_is_running (source->main_loop)) {
        g_main_loop_run (source->main_loop);
    }

    return NULL;
}

static void
command_source_class_init (CommandSourceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ThreadClass  *thread_class = THREAD_CLASS (klass);

    g_debug ("command_source_class_init");
    if (command_source_parent_class == NULL)
        command_source_parent_class = g_type_class_peek_parent (klass);

    object_class->dispose      = command_source_dispose;
    object_class->finalize     = command_source_finalize;
    object_class->get_property = command_source_get_property;
    object_class->set_property = command_source_set_property;
    thread_class->thread_run   = command_source_thread;
    thread_class->thread_unblock = command_source_unblock;

    obj_properties [PROP_COMMAND_ATTRS] =
        g_param_spec_object ("command-attrs",
                             "CommandAttrs object",
                             "CommandAttrs instance.",
                             TYPE_COMMAND_ATTRS,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_CONNECTION_MANAGER] =
        g_param_spec_object ("connection-manager",
                             "ConnectionManager object",
                             "ConnectionManager instance.",
                             TYPE_CONNECTION_MANAGER,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SINK] =
        g_param_spec_object ("sink",
                             "Sink",
                             "Reference to a Sink object.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
CommandSource*
command_source_new (ConnectionManager    *connection_manager,
                    CommandAttrs         *command_attrs)
{
    CommandSource *source;

    if (connection_manager == NULL)
        g_error ("command_source_new passed NULL ConnectionManager");
    g_object_ref (connection_manager);
    source = COMMAND_SOURCE (g_object_new (TYPE_COMMAND_SOURCE,
                                             "command-attrs", command_attrs,
                                             "connection-manager", connection_manager,
                                             NULL));
    g_signal_connect (connection_manager,
                      "new-connection",
                      (GCallback) command_source_on_new_connection,
                      source);
    return source;
}
