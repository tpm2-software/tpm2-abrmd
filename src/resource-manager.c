#include <errno.h>

#include "control-message.h"
#include "message-queue.h"
#include "resource-manager.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "tpm2-command.h"
#include "tpm2-response.h"
#include "thread-interface.h"
#include "util.h"

static gpointer resource_manager_parent_class = NULL;

enum {
    PROP_0,
    PROP_QUEUE_IN,
    PROP_SINK,
    PROP_ACCESS_BROKER,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * This function is invoked in response to the receipt of a Tpm2Command.
 * This is the place where we send the command buffer out to the TPM
 * through the AccessBroker which will eventually get it to the TPM for us.
 * The AccessBroker will send us back a Tpm2Response that we send back to
 * the client by way of our Sink object. The flow is roughly:
 * - Receive the Tpm2Command as a parameter
 * - Send the Tpm2Command out through the AccessBroker.
 * - Receive the response from the AccessBroker.
 * - Enqueue the response back out to the processing pipeline through the
 *   Sink object.
 */
void
resource_manager_process_tpm2_command (ResourceManager   *resmgr,
                                       Tpm2Command       *command)
{
    TSS2_RC rc;
    Tpm2Response *response;

    g_debug ("resource_manager_process_tpm2_command: resmgr: 0x%x, cmd: 0x%x",
             resmgr, command);
    g_debug_bytes (tpm2_command_get_buffer (command),
                   tpm2_command_get_size (command),
                   16,
                   4);
    /* transform the Tpm2Command */
    /* send the command */
    response = access_broker_send_command (resmgr->access_broker,
                                           command,
                                           &rc);
    if (rc != TSS2_RC_SUCCESS)
        g_warning ("access_broker_send_command returned error: 0x%x", rc);
    if (response == NULL)
        g_error ("access_broker_send_command returned NULL Tpm2Response?");
    g_object_unref (command);
    g_debug ("  received:");
    g_debug_bytes (tpm2_response_get_buffer (response),
                   tpm2_response_get_size (response),
                   16,
                   4);
    /* transform the Tpm2Response */
    /* send the response to the sinrk */
    sink_enqueue (resmgr->sink, G_OBJECT (response));
    g_object_unref (response);
}
/**
 * This function acts as a thread. It simply:
 * - Blocks on the in_queue. Then wakes up and
 * - Dequeues a message from the in_queue.
 * - Processes the message (depending on TYPE)
 * - Does it all over again.
 */
gpointer
resource_manager_thread (gpointer data)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (data);
    GObject         *obj = NULL;

    g_debug ("resource_manager_thread start");
    while (TRUE) {
        obj = message_queue_dequeue (resmgr->in_queue);
        g_debug ("resource_manager_thread: message_queue_dequeue got obj: "
                 "0x%x", obj);
        if (obj == NULL) {
            g_debug ("resource_manager_thread: dequeued a null object");
            break;
        }
        if (IS_TPM2_COMMAND (obj))
            resource_manager_process_tpm2_command (resmgr, TPM2_COMMAND (obj));
        if (IS_CONTROL_MESSAGE (obj))
            process_control_message (CONTROL_MESSAGE (obj));
     }
}
/**
 * Thread creation / start function.
 */
gint
resource_manager_start (Thread *self)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (self);

    if (resmgr == NULL)
        g_error ("resource_manager_start passed NULL ResourceManager");
    if (resmgr->thread != 0)
        g_error ("ResourceManager thread already running");
    return  pthread_create (&resmgr->thread,
                            NULL,
                            resource_manager_thread,
                            resmgr);
}
/**
 * Cancel the internal resource_manager thread. This requires not just
 * calling the pthread_cancel function, but also waking up the thread
 * by creating a new ControlMessage and enquing it in the in_queue.
 */
gint
resource_manager_cancel (Thread *self)
{
    gint ret;
    ControlMessage *msg;
    ResourceManager *resmgr = RESOURCE_MANAGER (self);

    if (resmgr == NULL)
        g_error ("resource_manager_cancel passed NULL ResourceManager");
    if (resmgr->thread == 0)
        g_error ("ResourceManager not running, cannot cancel");
    ret = pthread_cancel (resmgr->thread);
    msg = control_message_new (CHECK_CANCEL);
    g_debug ("resource_manager_cancel: enqueuing ControlMessage: 0x%x", msg);
    message_queue_enqueue (resmgr->in_queue, G_OBJECT (msg));

    return ret;
}
/**
 * Simple wrapper around pthread_join and the internal thread.
 */
gint
resource_manager_join (Thread *self)
{
    gint ret;
    ResourceManager *resmgr = RESOURCE_MANAGER (self);

    if (resmgr == NULL)
        g_error ("resource_manager_join passed NULL param");
    if (resmgr->thread == 0)
        g_error ("ResourceManager not running, cannot join");
    ret = pthread_join (resmgr->thread, NULL);
    resmgr->thread = 0;

    return ret;
}
/**
 * Implement the 'enqueue' function from the Sink interface. This is how
 * new messages / commands get into the AccessBroker.
 */
void
resource_manager_enqueue (Sink        *sink,
                          GObject     *obj)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (sink);

    g_debug ("resource_manager_enqueue: ResourceManager: 0x%x obj: 0x%x", resmgr, obj);
    message_queue_enqueue (resmgr->in_queue, obj);
}
/**
 * Implement the 'add_sink' function from the SourceInterface. This adds a
 * reference to an object that implements the SinkInterface to this objects
 * internal structure. We pass it data.
 */
void
resource_manager_add_sink (Source *self,
                           Sink   *sink)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (self);
    GValue value = G_VALUE_INIT;

    g_debug ("resource_manager_add_sink: ResourceManager: 0x%x, Sink: 0x%x",
             resmgr, sink);
    g_value_init (&value, G_TYPE_OBJECT);
    g_value_set_object (&value, sink);
    g_object_set_property (G_OBJECT (resmgr), "sink", &value);
    g_value_unset (&value);
}
/**
 * GObject property setter.
 */
static void
resource_manager_set_property (GObject        *object,
                               guint           property_id,
                               GValue const   *value,
                               GParamSpec     *pspec)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (object);

    g_debug ("resource_manager_set_property: 0x%x", resmgr);
    switch (property_id) {
    case PROP_QUEUE_IN:
        resmgr->in_queue = g_value_get_object (value);
        g_debug ("  in_queue: 0x%x", resmgr->in_queue);
        break;
    case PROP_SINK:
        if (resmgr->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        resmgr->sink = SINK (g_value_get_object (value));
        g_object_ref (resmgr->sink);
        g_debug ("  sink: 0x%x", resmgr->sink);
        break;
    case PROP_ACCESS_BROKER:
        if (resmgr->access_broker != NULL) {
            g_warning ("  access_broker already set");
            break;
        }
        resmgr->access_broker = g_value_get_object (value);
        g_object_ref (resmgr->access_broker);
        g_debug ("  access_broker: 0x%x", resmgr->access_broker);
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
resource_manager_get_property (GObject     *object,
                               guint        property_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (object);

    g_debug ("resource_manager_get_property: 0x%x", resmgr);
    switch (property_id) {
    case PROP_QUEUE_IN:
        g_value_set_object (value, resmgr->in_queue);
        break;
    case PROP_SINK:
        g_value_set_object (value, resmgr->sink);
        break;
    case PROP_ACCESS_BROKER:
        g_value_set_object (value, resmgr->access_broker);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * Bring down the ResourceManager as gracefully as we can.
 */
static void
resource_manager_finalize (GObject *obj)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (obj);

    g_debug ("resource_manager_finalize: 0x%x", resmgr);
    if (resmgr == NULL)
        g_error ("resource_manager_finalize passed NULL ResourceManager pointer");
    if (resmgr->thread != 0)
        g_error ("resource_manager_finalize called with thread running, "
                 "cancel thread first");
    g_object_unref (resmgr->in_queue);
    if (resmgr->sink) {
        g_object_unref (resmgr->sink);
        resmgr->sink = NULL;
    }
    if (resmgr->access_broker) {
        g_object_unref (resmgr->access_broker);
        resmgr->access_broker = NULL;
    }
    if (resource_manager_parent_class)
        G_OBJECT_CLASS (resource_manager_parent_class)->finalize (obj);
}
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set finalize, property get/set.
 * - Install properties.
 */
static void
resource_manager_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (resource_manager_parent_class == NULL)
        resource_manager_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = resource_manager_finalize;
    object_class->get_property = resource_manager_get_property;
    object_class->set_property = resource_manager_set_property;

    obj_properties [PROP_QUEUE_IN] =
        g_param_spec_object ("queue-in",
                             "input queue",
                             "Input queue for messages.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SINK] =
        g_param_spec_object ("sink",
                             "Sink",
                             "Reference to a Sink object that we pass messages to.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE);
    obj_properties [PROP_ACCESS_BROKER] =
        g_param_spec_object ("access-broker",
                             "AccessBroker object",
                             "TPM Access Broker for communication with TPM",
                             TYPE_ACCESS_BROKER,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate code to register function s with the ThreadInterface.
 */
static void
resource_manager_thread_interface_init (gpointer g_iface)
{
    ThreadInterface *thread = (ThreadInterface*)g_iface;
    thread->cancel = resource_manager_cancel;
    thread->join   = resource_manager_join;
    thread->start  = resource_manager_start;
}
/**
 * Boilerplate code to register functions with the SourceInterface.
 */
static void
resource_manager_source_interface_init (gpointer g_iface)
{
    SourceInterface *source = (SourceInterface*)g_iface;
    source->add_sink = resource_manager_add_sink;
}
/**
 * Boilerplate code to register function with the SinkInterface.
 */
static void
resource_manager_sink_interface_init (gpointer g_iface)
{
    SinkInterface *sink = (SinkInterface*)g_iface;
    sink->enqueue = resource_manager_enqueue;
}
/**
 * GObject boilerplate get_type.
 */
GType
resource_manager_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "ResourceManager",
                                              sizeof (ResourceManagerClass),
                                              (GClassInitFunc) resource_manager_class_init,
                                              sizeof (ResourceManager),
                                              NULL,
                                              0);
        const GInterfaceInfo interface_info_thread = {
            (GInterfaceInitFunc) resource_manager_thread_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_THREAD, &interface_info_thread);

        const GInterfaceInfo interface_info_sink = {
            (GInterfaceInitFunc) resource_manager_sink_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_SINK, &interface_info_sink);

        const GInterfaceInfo interface_info_source = {
            (GInterfaceInitFunc) resource_manager_source_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_SOURCE, &interface_info_source);
    }
    return type;
}
/**
 * Create new ResourceManager object.
 */
ResourceManager*
resource_manager_new (AccessBroker    *broker)
{
    if (broker == NULL)
        g_error ("resource_manager_new passed NULL AccessBroker");
    MessageQueue *queue = message_queue_new ("ResourceManager input queue");
    return RESOURCE_MANAGER (g_object_new (TYPE_RESOURCE_MANAGER,
                                           "queue-in",        queue,
                                           "access-broker",   broker,
                                           NULL));
}
