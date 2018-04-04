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
#include <pthread.h>

#include "connection.h"
#include "sink-interface.h"
#include "response-sink.h"
#include "control-message.h"
#include "tpm2-response.h"
#include "util.h"

#define RESPONSE_SINK_TIMEOUT 1e6

static void response_sink_sink_interface_init   (gpointer g_iface);

G_DEFINE_TYPE_WITH_CODE (
    ResponseSink,
    response_sink,
    TYPE_THREAD,
    G_IMPLEMENT_INTERFACE (TYPE_SINK,
                           response_sink_sink_interface_init)
    );

enum {
    PROP_0,
    PROP_IN_QUEUE,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * enqueue function to implement Sink interface.
 */
void
response_sink_enqueue (Sink            *self,
                       GObject         *obj)
{
    ResponseSink *sink = RESPONSE_SINK (self);

    g_debug ("response_sink_enqueue:");
    if (sink == NULL)
        g_error ("  passed NULL sink");
    if (obj == NULL)
        g_error ("  passed NULL object");
    message_queue_enqueue (sink->in_queue, obj);
}
/**
 * GObject property setter.
 */
static void
response_sink_set_property (GObject        *object,
                               guint           property_id,
                               GValue const   *value,
                               GParamSpec     *pspec)
{
    ResponseSink *self = RESPONSE_SINK (object);

    g_debug ("response_sink_set_property");
    switch (property_id) {
    case PROP_IN_QUEUE:
        g_debug ("  setting PROP_IN_QUEUE");
        self->in_queue = g_value_get_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * GObject property getter.
 */
static void
response_sink_get_property (GObject     *object,
                               guint        property_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
    ResponseSink *self = RESPONSE_SINK (object);

    g_debug ("response_sink_get_property: 0x%" PRIxPTR, (uintptr_t)self);
    switch (property_id) {
    case PROP_IN_QUEUE:
        g_value_set_object (value, self->in_queue);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * Bring down the ResponseSink as gracefully as we can.
 */
static void
response_sink_dispose (GObject *obj)
{
    ResponseSink *sink = RESPONSE_SINK (obj);
    Thread *thread = THREAD (obj);

    g_debug ("%s: 0x%" PRIxPTR, __func__, (uintptr_t)obj);
    if (sink == NULL)
        g_error ("%s: passed NULL pointer", __func__);
    if (thread->thread_id != 0)
        g_error ("%s: thread running, cancel first", __func__);
    g_clear_object (&sink->in_queue);
    G_OBJECT_CLASS (response_sink_parent_class)->dispose (obj);
}
void* response_sink_thread (void *data);
static void
response_sink_init (ResponseSink *response)
{
    UNUSED_PARAM(response);
    /* noop */
}
static void
response_sink_unblock (Thread *self)
{
    ResponseSink *sink = RESPONSE_SINK (self);
    ControlMessage *msg;

    if (sink == NULL)
        g_error ("response_sink_cancel passed NULL sink");
    msg = control_message_new (CHECK_CANCEL);
    g_debug ("response_sink_cancel enqueuing ControlMessage: 0x%" PRIxPTR,
             (uintptr_t)msg);
    message_queue_enqueue (sink->in_queue, G_OBJECT (msg));
    g_object_unref (msg);
}
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set dispose, property get/set.
 * - Install properties.
 */
static void
response_sink_class_init (ResponseSinkClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ThreadClass  *thread_class = THREAD_CLASS (klass);

    if (response_sink_parent_class == NULL)
        response_sink_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose = response_sink_dispose;
    object_class->get_property = response_sink_get_property;
    object_class->set_property = response_sink_set_property;
    thread_class->thread_run     = response_sink_thread;
    thread_class->thread_unblock = response_sink_unblock;

    obj_properties [PROP_IN_QUEUE] =
        g_param_spec_object ("in-queue",
                             "MessageQueue",
                             "Input MessageQueue.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate code to register functions with the SinkInterface.
 */
static void
response_sink_sink_interface_init (gpointer g_iface)
{
    SinkInterface *sink_interface = (SinkInterface*)g_iface;
    sink_interface->enqueue = response_sink_enqueue;
}
/**
 */
ResponseSink*
response_sink_new ()
{
    MessageQueue *in_queue = message_queue_new ();
    return RESPONSE_SINK (g_object_new (TYPE_RESPONSE_SINK,
                                           "in-queue", in_queue,
                                           NULL));
}

ssize_t
response_sink_process_response (Tpm2Response *response)
{
    ssize_t      written = 0;
    guint32      size    = tpm2_response_get_size (response);
    guint8      *buffer  = tpm2_response_get_buffer (response);
    Connection  *connection = tpm2_response_get_connection (response);
    GIOStream   *iostream = connection_get_iostream (connection);
    GOutputStream *ostream = g_io_stream_get_output_stream (iostream);

    g_debug ("response_sink_thread got response: 0x%" PRIxPTR " size %d",
             (uintptr_t)response, size);
    g_debug ("  writing 0x%x bytes", size);
    g_debug_bytes (buffer, size, 16, 4);
    written = write_all (ostream, buffer, size);
    g_object_unref (connection);

    return written;
}

void*
response_sink_thread (void *data)
{
    ResponseSink *sink = RESPONSE_SINK (data);
    GObject *obj;

    do {
        g_debug ("response_sink_thread blocking on input queue: 0x%" PRIxPTR,
                 (uintptr_t)sink->in_queue);
        obj = message_queue_dequeue (sink->in_queue);
        g_debug ("response_sink_thread got obj: 0x%" PRIxPTR, (uintptr_t)obj);
        if (IS_CONTROL_MESSAGE (obj)) {
            g_object_unref (obj);
            break;
        }
        if (IS_TPM2_RESPONSE (obj)) {
            response_sink_process_response (TPM2_RESPONSE (obj));
            g_object_unref (obj);
        }
    } while (TRUE);

    return NULL;
}
