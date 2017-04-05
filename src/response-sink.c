#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <pthread.h>

#include "connection.h"
#include "sink-interface.h"
#include "thread-interface.h"
#include "response-sink.h"
#include "control-message.h"
#include "tpm2-response.h"
#include "util.h"

#define RESPONSE_SINK_TIMEOUT 1e6

static void response_sink_thread_interface_init (gpointer g_iface);
static void response_sink_sink_interface_init   (gpointer g_iface);

G_DEFINE_TYPE_WITH_CODE (
    ResponseSink,
    response_sink,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TYPE_THREAD,
                           response_sink_thread_interface_init)
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
 * Forward declare the functions for the thread interface. We need this
 * for the type registration in _get_type.
 */
gint response_sink_cancel (Thread *self);
gint response_sink_join   (Thread *self);
gint response_sink_start  (Thread *self);
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
response_sink_finalize (GObject *obj)
{
    ResponseSink *sink = RESPONSE_SINK (obj);

    g_debug ("response_sink_finalize");
    if (sink == NULL)
        g_error ("  response_sink_free passed NULL pointer");
    if (sink->thread != 0)
        g_error ("  response_sink finalized with running thread, cancel first");
    if (sink->in_queue)
        g_object_unref (sink->in_queue);
    if (response_sink_parent_class)
        G_OBJECT_CLASS (response_sink_parent_class)->finalize (obj);
}
static void
response_sink_init (ResponseSink *response)
{ /* noop */ }
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set finalize, property get/set.
 * - Install properties.
 */
static void
response_sink_class_init (ResponseSinkClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (response_sink_parent_class == NULL)
        response_sink_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = response_sink_finalize;
    object_class->get_property = response_sink_get_property;
    object_class->set_property = response_sink_set_property;

    obj_properties [PROP_IN_QUEUE] =
        g_param_spec_object ("in-queue",
                             "MessageQueeu",
                             "Input MessageQueue.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate code to register functions with the ThreadInterface.
 */
static void
response_sink_thread_interface_init (gpointer g_iface)
{
    ThreadInterface *thread_interface = (ThreadInterface*)g_iface;
    thread_interface->cancel = response_sink_cancel;
    thread_interface->join   = response_sink_join;
    thread_interface->start  = response_sink_start;
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
    MessageQueue *in_queue = message_queue_new ("in-queue");
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
    gint         fd      = connection_send_fd (connection);

    g_debug ("response_sink_thread got response: 0x%" PRIxPTR " size %d",
             (uintptr_t)response, size);
    g_debug ("  writing 0x%x bytes", size);
    g_debug_bytes (buffer, size, 16, 4);
    written = write_all (fd, buffer, size);
    if (written <= 0)
        g_warning ("write failed (%zu) on fd %d for connection 0x%" PRIxPTR ": %s",
                   written, fd, (uintptr_t)connection, strerror (errno));
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
            ControlCode code =
                control_message_get_code (CONTROL_MESSAGE (obj));
            g_object_unref (obj);
            process_control_code (code);
        }
        if (IS_TPM2_RESPONSE (obj)) {
            response_sink_process_response (TPM2_RESPONSE (obj));
            g_object_unref (obj);
        }
    } while (TRUE);
}
/**
 * The remaining functions implement the ThreadInterface.
 */
gint
response_sink_start (Thread *self)
{
    ResponseSink *sink = RESPONSE_SINK (self);

    if (sink == NULL)
        g_error ("response_sink_start: passed NULL sink");
    if (sink->thread != 0)
        g_error ("response_sink already started");
    return pthread_create (&sink->thread, NULL, response_sink_thread, sink);
}
gint
response_sink_cancel (Thread *self)
{
    ResponseSink *sink = RESPONSE_SINK (self);
    gint ret;
    ControlMessage *msg;

    if (sink == NULL)
        g_error ("response_sink_cancel passed NULL sink");
    if (sink->thread == 0)
        g_error ("response_sink_cancel: cannot cancel thread with id 0");
    ret = pthread_cancel (sink->thread);
    msg = control_message_new (CHECK_CANCEL);
    g_debug ("response_sink_cancel enqueuing ControlMessage: 0x%" PRIxPTR,
             (uintptr_t)msg);
    message_queue_enqueue (sink->in_queue, G_OBJECT (msg));
    g_object_unref (msg);

    return ret;
}
gint
response_sink_join (Thread *self)
{
    ResponseSink *sink = RESPONSE_SINK (self);
    gint ret;

    if (sink == NULL)
        g_error ("response_sink_join passed NULL sink");
    if (sink->thread == 0)
        g_error ("response_sink_join: cannot join thread with id 0");
    ret = pthread_join (sink->thread, NULL);
    if (ret == 0)
        sink->thread = 0;

    return ret;
}
