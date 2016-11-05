#include <errno.h>

#include "control-message.h"
#include "message-queue.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "tab.h"
#include "tpm2-command.h"
#include "tpm2-response.h"
#include "thread-interface.h"

#define TAB_TIMEOUT_DEQUEUE 1e6

static gpointer tab_parent_class = NULL;

enum {
    PROP_0,
    PROP_QUEUE_IN,
    PROP_SINK,
    PROP_TCTI,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * Forward declare the functions for the thread interface. We need this
 * for the type registration in _get_type.
 */
gint tab_cancel (Thread *self);
gint tab_join   (Thread *self);
gint tab_start  (Thread *self);
/**
 * Implement the 'enqueue' function from the Sink interface. This is how
 * new messages / commands get into the Tab.
 */
void
tab_enqueue (Sink        *sink,
             GObject     *obj)
{
    Tab *tab = TAB (sink);

    g_debug ("tab_enqueue: Tab: 0x%x obj: 0x%x", tab, obj);
    /* The TAB takes ownership of this object */
    g_object_ref (obj);
    message_queue_enqueue (tab->in_queue, obj);
}
/**
 * Implement the 'add_sink' function from the SourceInterface. This adds a
 * reference to an object that implements the SinkInterface to this objects
 * internal structure. We pass it data.
 */
void
tab_add_sink (Source *self,
              Sink   *sink)
{
    Tab *src = TAB (self);
    GValue value = G_VALUE_INIT;

    g_debug ("tab_add_sink: Tab: 0x%x, Sink: 0x%x", src, sink);
    g_value_init (&value, G_TYPE_OBJECT);
    g_value_set_object (&value, sink);
    g_object_set_property (G_OBJECT (src), "sink", &value);
    g_value_unset (&value);
}
/**
 * GObject property setter.
 */
static void
tab_set_property (GObject        *object,
                  guint           property_id,
                  GValue const   *value,
                  GParamSpec     *pspec)
{
    Tab *self = TAB (object);

    g_debug ("tab_set_property: 0x%x", self);
    switch (property_id) {
    case PROP_QUEUE_IN:
        self->in_queue = g_value_get_object (value);
        g_debug ("  in_queue: 0x%x", self->in_queue);
        break;
    case PROP_SINK:
        if (self->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        self->sink = SINK (g_value_get_object (value));
        g_object_ref (self->sink);
        g_debug ("  sink: 0x%x", self->sink);
        break;
    case PROP_TCTI:
        self->tcti = g_value_get_object (value);
        g_debug ("  tcti: 0x%x", self->tcti);
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
tab_get_property (GObject     *object,
                  guint        property_id,
                  GValue      *value,
                  GParamSpec  *pspec)
{
    Tab *self = TAB (object);

    g_debug ("tab_get_property: 0x%x", self);
    switch (property_id) {
    case PROP_QUEUE_IN:
        g_value_set_object (value, self->in_queue);
        break;
    case PROP_SINK:
        g_value_set_object (value, self->sink);
        break;
    case PROP_TCTI:
        g_value_set_object (value, self->tcti);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * Bring down the TAB as gracefully as we can.
 */
static void
tab_finalize (GObject *obj)
{
    Tab *tab = TAB (obj);

    g_debug ("tab_finalize: 0x%x", tab);
    if (tab == NULL)
        g_error ("tab_free passed NULL Tab pointer");
    if (tab->thread != 0)
        g_error ("tab_free called with thread running, cancel thread first");
    g_object_unref (tab->in_queue);
    if (tab->sink) {
        g_object_unref (tab->sink);
        tab->sink = NULL;
    }
    if (tab->tcti)
        g_object_unref (tab->tcti);
    if (tab_parent_class)
        G_OBJECT_CLASS (tab_parent_class)->finalize (obj);
}
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set finalize, property get/set.
 * - Install properties.
 */
static void
tab_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tab_parent_class == NULL)
        tab_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = tab_finalize;
    object_class->get_property = tab_get_property;
    object_class->set_property = tab_set_property;

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
    obj_properties [PROP_TCTI] =
        g_param_spec_object ("tcti",
                             "Tcti object",
                             "Tcti for communication with TPM",
                             TYPE_TCTI,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate code to register function s with the ThreadInterface.
 */
static void
tab_thread_interface_init (gpointer g_iface)
{
    ThreadInterface *thread = (ThreadInterface*)g_iface;
    thread->cancel = tab_cancel;
    thread->join   = tab_join;
    thread->start  = tab_start;
}
/**
 * Boilerplate code to register functions with the SourceInterface.
 */
static void
tab_source_interface_init (gpointer g_iface)
{
    SourceInterface *source = (SourceInterface*)g_iface;
    source->add_sink = tab_add_sink;
}
/**
 * Boilerplate code to register function with the SinkInterface.
 */
static void
tab_sink_interface_init (gpointer g_iface)
{
    SinkInterface *sink = (SinkInterface*)g_iface;
    sink->enqueue = tab_enqueue;
}
/**
 * GObject boilerplate get_type.
 */
GType
tab_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "Tab",
                                              sizeof (TabClass),
                                              (GClassInitFunc) tab_class_init,
                                              sizeof (Tab),
                                              NULL,
                                              0);
        const GInterfaceInfo interface_info_thread = {
            (GInterfaceInitFunc) tab_thread_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_THREAD, &interface_info_thread);

        const GInterfaceInfo interface_info_sink = {
            (GInterfaceInitFunc) tab_sink_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_SINK, &interface_info_sink);

        const GInterfaceInfo interface_info_source = {
            (GInterfaceInitFunc) tab_source_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_SOURCE, &interface_info_source);
    }
    return type;
}
/**
 * This function is invoked in response to the receipt of a Tpm2Command.
 * This is the place where we send the command buffer out of the tabd
 * "down-stream" TCTI and then get a response that we send back to the
 * client. The flow is roughly:
 * - Send the data from the Tpm2Command out: tss2_tcti_transmit.
 * - Create a response message with the same session as the command.
 * - Receive the response from downstream: tss2_tcti_receive.
 * - Enqueue the response in the out_queue.
 */
ssize_t
tab_process_tpm2_command (Tab          *tab,
                          Tpm2Command  *command)
{
    TSS2_RC rc;
    Tpm2Response *response;
    uint8_t      *data;
    size_t        size = 4096;


    g_debug ("tab_process_tpm2_command");
    g_debug ("  transmitting:");
    g_debug_bytes (tpm2_command_get_buffer (command),
                   tpm2_command_get_size (command),
                   16,
                   4);
    /* send the command */
    rc = tcti_transmit (tab->tcti,
                        tpm2_command_get_size (command),
                        tpm2_command_get_buffer (command));
    if (rc != TSS2_RC_SUCCESS)
        g_error ("tss2_tcti_transmit returned error: 0x%x", rc);
    /* create response object passing it the same session as the command */
    data = g_malloc0 (size);
    response = tpm2_response_new (tpm2_command_get_session (command),
                                  data);
    if (response == NULL)
        g_error ("failed to create response message");
    /* we're done with the command, unref it and get the response */
    g_object_unref (command);

    rc = tcti_receive (tab->tcti,
                       &size,
                       tpm2_response_get_buffer (response),
                       TSS2_TCTI_TIMEOUT_BLOCK);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("tss2_tcti_receive returned error: 0x%x", rc);
    g_debug ("  received:");
    g_debug_bytes (tpm2_response_get_buffer (response), size, 16, 4);

    /* send the response to the sink */
    if (tab->sink)
        sink_enqueue (tab->sink, G_OBJECT (response));
}
static TSS2_SYS_CONTEXT*
sapi_context_init (TSS2_TCTI_CONTEXT *tcti_ctx)
{
    TSS2_SYS_CONTEXT *sapi_ctx;
    TSS2_RC rc;
    size_t size;
    TSS2_ABI_VERSION abi_version = {
        .tssCreator = TSSWG_INTEROP,
        .tssFamily  = TSS_SAPI_FIRST_FAMILY,
        .tssLevel   = TSS_SAPI_FIRST_LEVEL,
        .tssVersion = TSS_SAPI_FIRST_VERSION,
    };

    size = Tss2_Sys_GetContextSize (0);
    sapi_ctx = (TSS2_SYS_CONTEXT*)g_malloc0 (size);
    if (sapi_ctx == NULL) {
        g_error ("Failed to allocate 0x%zx bytes for the SAPI context\n",
                 size);
        return NULL;
    }
    rc = Tss2_Sys_Initialize (sapi_ctx, size, tcti_ctx, &abi_version);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Failed to initialize SAPI context: 0x%x\n", rc);
        return NULL;
    }
    return sapi_ctx;
}
static void
send_tpm_startup (Tab *tab)
{
    TSS2_TCTI_CONTEXT *tcti_context;
    TSS2_SYS_CONTEXT *sapi_context;
    TSS2_RC rc;

    tcti_context = tcti_peek_context (tab->tcti);
    if (tcti_context == NULL)
        g_error ("  NULL TCTI context");
    sapi_context = sapi_context_init (tcti_context);
    if (sapi_context == NULL)
        g_error ("  NULL SAPI context");
    rc = Tss2_Sys_Startup (sapi_context, TPM_SU_CLEAR);
    if (rc != TSS2_RC_SUCCESS && rc != TPM_RC_INITIALIZE)
        g_error ("Tss2_Sys_Startup returned unexpected RC: 0x%x", rc);
    Tss2_Sys_Finalize (sapi_context);
    g_free (sapi_context);
}
/**
 * This function acts as a thread. It simply:
 * - Blocks on the in_queue. Then wakes up and
 * - Dequeues a message from the in_queue.
 * - Processes the message (depending on TYPE)
 * - Checks to see if it has been canceled.
 * - Does it all over again.
 */
gpointer
cmd_runner (gpointer data)
{
    Tab         *tab = TAB (data);
    GObject     *obj = NULL;

    g_debug ("cmd_runner start");
    send_tpm_startup (tab);
    while (TRUE) {
        obj = message_queue_dequeue (tab->in_queue);
        g_debug ("cmd_runner message_queue_dequeue got obj: 0x%x", obj);
        if (IS_TPM2_COMMAND (obj))
            tab_process_tpm2_command (tab, TPM2_COMMAND (obj));
        if (IS_CONTROL_MESSAGE (obj))
            process_control_message (CONTROL_MESSAGE (obj));
        if (obj == NULL)
            g_debug ("cmd_runner: dequeued a null object");
        pthread_testcancel ();
    }
}

/**
 * Create new TPM access broker (TAB) object. This includes the private
 * input / output queues.
 */
Tab*
tab_new (Tcti             *tcti)
{
    if (tcti == NULL)
        g_error ("tab_new passed NULL Tcti");
    MessageQueue *queue = message_queue_new ("TAB input queue");
    return TAB (g_object_new (TYPE_TAB,
                              "queue-in",         queue,
                              "tcti",             tcti,
                              NULL));
}
/**
 * Thread creation / start function.
 */
gint
tab_start (Thread *self)
{
    Tab *tab = TAB (self);

    if (tab == NULL)
        g_error ("tab_start passed NULL Tab");
    if (tab->thread != 0)
        g_error ("Tab cmd_runner thread already running");
    return  pthread_create (&tab->thread, NULL, cmd_runner, tab);
}
/**
 * Cancel the internal tab thread. This requires not just calling the
 * pthread_cancel function, but also waking up the thread by creating
 * a new ControlMessage and enquing it in the in_queue. We do the same
 * for the out_queue as a way to kill off the response sink (assuming
 * that some higher power has already called it for us).
 */
gint
tab_cancel (Thread *self)
{
    gint ret;
    ControlMessage *msg;
    Tab *tab = TAB (self);

    if (tab == NULL)
        g_error ("tab_cancel passed NULL Tab");
    if (tab->thread == 0)
        g_error ("Tab not running, cannot cancel");
    /* cancel our internal thread before we unblock it */
    ret = pthread_cancel (tab->thread);
    /* Let anything blocked on the in_queue know that they should check to
     * see if they need to cancel too.
     */
    msg = control_message_new (CHECK_CANCEL);
    g_debug ("tab_cancel: enqueuing ControlMessage: 0x%x", msg);
    message_queue_enqueue (tab->in_queue, G_OBJECT (msg));

    return ret;
}
/**
 * Simple wrapper around pthread_join and the internal thread.
 */
gint
tab_join (Thread *self)
{
    gint ret;
    Tab *tab = TAB (self);

    if (tab == NULL)
        g_error ("tab_join passed NULL Tab");
    if (tab->thread == 0)
        g_error ("Tab not running, cannot join");
    ret = pthread_join (tab->thread, NULL);
    tab->thread = 0;

    return ret;
}
/**
 * Cancel pending commands for a session in the TAB.
 * Cancel each pending command associated with the given session in the TAB
 * provided. The number of commands canceled is returned to the caller. If
 * there are no pending commands, 0 is returned. -1 is returned when an error
 * occurs.
 */
gint
tab_cancel_commands (Tab            *tab,
                     SessionData    *session)
{
    g_error ("tab_cancel_commands: unimplemented");
    return 0;
}
