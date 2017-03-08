#include <errno.h>
#include <inttypes.h>

#include "control-message.h"
#include "message-queue.h"
#include "resource-manager.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "tabrmd.h"
#include "tpm2-command.h"
#include "tpm2-response.h"
#include "thread-interface.h"
#include "util.h"

#define RM_RC(rc) TSS2_RESMGR_ERROR_LEVEL + rc

static gpointer resource_manager_parent_class = NULL;

enum {
    PROP_0,
    PROP_QUEUE_IN,
    PROP_SINK,
    PROP_ACCESS_BROKER,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/*
 * This is a helper function that does everything required to convert
 * a virtual handle to a physical one in a Tpm2Command object.
 * - load the context from the provided HandleMapEntry
 * - store the newly assigned TPM handle (physical handle) in the entry
 * - set this handle in the comamnd at the position indicated by
 *   'handle_number' (0-based index)
 */
TSS2_RC
resource_manager_virt_to_phys (ResourceManager *resmgr,
                               Tpm2Command     *command,
                               HandleMapEntry  *entry,
                               guint8           handle_number)
{
    TPM_HANDLE    phandle = 0;
    TPMS_CONTEXT *context;
    TSS2_RC       rc = TSS2_RC_SUCCESS;

    context = handle_map_entry_get_context (entry);
    rc = access_broker_context_load (resmgr->access_broker, context, &phandle);
    g_debug ("phandle: 0x%" PRIx32, phandle);
    if (rc == TSS2_RC_SUCCESS) {
        handle_map_entry_set_phandle (entry, phandle);
        tpm2_command_set_handle (command, phandle, handle_number);
    } else {
        g_warning ("Failed to load context: 0x%" PRIx32, rc);
    }
    return rc;
}
/*
 * This function operates on the provided command. It iterates over each
 * handle in the commands handle area. For each relevant handle it loads
 * the related context and fixes up the handles in the command.
 */
TSS2_RC
resource_manager_load_contexts (ResourceManager *resmgr,
                                Tpm2Command     *command,
                                HandleMapEntry  *entries[],
                                guint           *entry_count)
{
    HandleMap    *map;
    HandleMapEntry *entry;
    SessionData  *session;
    TSS2_RC       rc = TSS2_RC_SUCCESS;
    TPM_HANDLE    handles[3] = { 0, };
    guint8 i, handle_count;;

    g_debug ("resource_manager_load_contexts");
    if (!resmgr || !command || !entries || !entry_count) {
        g_warning ("resource_manager_load_contexts received NULL parameter.");
        return RM_RC (TSS2_BASE_RC_GENERAL_FAILURE);
    }
    session = tpm2_command_get_session (command);
    handle_count = tpm2_command_get_handle_count (command);
    if (handle_count > *entry_count) {
        g_warning ("resource_manager_load_contexts handle count > entry_count");
        return RM_RC (TSS2_BASE_RC_GENERAL_FAILURE);
    }
    *entry_count = 0;
    tpm2_command_get_handles (command, handles, handle_count);
    g_debug ("loading contexts for %" PRId8 " handles", handle_count);
    for (i = 0; i < handle_count; ++i) {
        switch (handles [i] >> HR_SHIFT) {
        case TPM_HT_TRANSIENT:
            g_debug ("processing TPM_HT_TRANSIENT: 0x%" PRIx32, handles [i]);
            map = session_data_get_trans_map (session);
            g_debug ("handle 0x%" PRIx32 " is virtual TPM_HT_TRANSIENT, "
                     "loading", handles [i]);
            entry = handle_map_vlookup (map, handles [i]);
            if (!entry) {
                g_warning ("No HandleMapEntry for vhandle: 0x%" PRIx32,
                           handles [i]);
                continue;
            }
            g_object_unref (map);
            rc = resource_manager_virt_to_phys (resmgr, command, entry, i);
            if (rc != TSS2_RC_SUCCESS)
                break;
            entries [i] = entry;
            ++(*entry_count);
            break;
        default:
            break;
        }
    }
    g_debug ("resource_manager_load_contexts end");
    g_object_unref (session);

    return rc;
}
/*
 * Remove all contexts with handles in the transitive range from the
 * TPM. Store them in the HandleMap and set the physical handle to 0.
 * A HandleMapEntry with a physical handle set to 0 means the context
 * has been saved and flushed.
 */
TSS2_RC
resource_manager_flushsave_context (ResourceManager *resmgr,
                                    HandleMapEntry  *entry)
{
    TPMS_CONTEXT   *context;
    TPM_HANDLE      phandle;
    TSS2_RC         rc = TSS2_RC_SUCCESS;

    g_debug ("resource_manager_flushsave_context for entry: 0x%" PRIxPTR,
             (uintptr_t)entry);
    if (resmgr == NULL || entry == NULL)
        g_error ("resource_manager_flushsave_context passed NULL parameter");
    phandle = handle_map_entry_get_phandle (entry);
    g_debug ("resource_manager_save_context phandle: 0x%" PRIx32, phandle);
    switch (phandle >> HR_SHIFT) {
    case TPM_HT_TRANSIENT:
        g_debug ("handle is transient, saving context");
        context = handle_map_entry_get_context (entry);
        rc = access_broker_context_saveflush (resmgr->access_broker,
                                              phandle,
                                              context);
        if (rc == TSS2_RC_SUCCESS) {
            handle_map_entry_set_phandle (entry, 0);
        } else {
            g_warning ("access_broker_context_save failed for handle: 0x%"
                       PRIx32 " rc: 0x%" PRIx32, phandle, rc);
        }
        break;
    default:
        break;
    }

    return rc;
}
/*
 * Each Tpm2Response object can have at most one handle in it.
 * This function assumes that the handle in the parameter Tpm2Response
 * object is a physical handle. It creates a new virtual handle and
 * allocates a new HandleMapEntry to map the virtual handle to a
 * TPMS_CONTEXT structure when processing future commands associated
 * with the same connection. This HandleMapEntry is inserted into the
 * handle map for the connection. It is also returned to the caller who
 * must decrement the reference count when they are done with it.
 */
HandleMapEntry*
resource_manager_virtualize_handle (ResourceManager *resmgr,
                                    Tpm2Response    *response)
{
    TPM_HANDLE      phandle, vhandle = 0;
    SessionData    *session;
    HandleMap      *handle_map;
    HandleMapEntry *entry = NULL;

    phandle = tpm2_response_get_handle (response);
    g_debug ("resource_manager_virtualize_handle 0x%" PRIx32, phandle);
    switch (phandle >> HR_SHIFT) {
    case TPM_HT_TRANSIENT:
        g_debug ("handle is transient, virtualizing");
        session = tpm2_response_get_session (response);
        handle_map = session_data_get_trans_map (session);
        vhandle = handle_map_next_vhandle (handle_map);
        if (vhandle == 0)
            g_error ("vhandle rolled over!");
        g_debug ("now has vhandle:0x%" PRIx32, vhandle);
        entry = handle_map_entry_new (phandle, vhandle);
        g_debug ("handle map entry: 0x%" PRIxPTR, (uintptr_t)entry);
        handle_map_insert (handle_map, vhandle, entry);
        tpm2_response_set_handle (response, vhandle);
        g_object_unref (session);
        g_object_unref (handle_map);
        break;
    default:
        g_debug ("handle isn't transient, not virtualizing");
        break;
    }

    return entry;
}
static void
dump_command (Tpm2Command *command)
{
    g_assert (command != NULL);
    g_debug ("Tpm2Command: 0x%" PRIxPTR, (uintptr_t)command);
    g_debug_bytes (tpm2_command_get_buffer (command),
                   tpm2_command_get_size (command),
                   16,
                   4);
    g_debug_tpma_cc (tpm2_command_get_attributes (command));
}
static void
dump_response (Tpm2Response *response)
{
    g_assert (response != NULL);
    g_debug ("Tpm2Response: 0x%" PRIxPTR, (uintptr_t)response);
    g_debug_bytes (tpm2_response_get_buffer (response),
                   tpm2_response_get_size (response),
                   16,
                   4);
    g_debug_tpma_cc (tpm2_response_get_attributes (response));
}
TSS2_RC
resource_manager_flush_context (ResourceManager *resmgr,
                                Tpm2Command     *command)
{
    SessionData *session;
    HandleMap   *map;
    HandleMapEntry *entry;
    TPM_HANDLE      vhandle;
    TSS2_RC rc;

    if (tpm2_command_get_code (command) != TPM_CC_FlushContext) {
        g_warning ("resource_manager_flush_context with wrong command");
        return TSS2_RC_SUCCESS;
    }
    session = tpm2_command_get_session (command);
    map = session_data_get_trans_map (session);
    g_object_unref (session);

    vhandle = tpm2_command_get_flush_handle (command);
    g_debug ("resource_manager_flush_context vhandle: 0x%" PRIx32, vhandle);
    entry = handle_map_vlookup (map, vhandle);
    if (entry != NULL) {
        handle_map_remove (map, vhandle);
        g_object_unref (entry);
        rc = TSS2_RC_SUCCESS;
    } else {
        /*
         * If the handle doesn't map to a HandleMapEntry then it's not one
         * that we're managing and so we can't flush it. Return an error
         * indicating that error is related to a handle, that it's a parameter
         * and that it's the first parameter.
         */
        rc = RM_RC (TPM_RC_HANDLE + TPM_RC_P + TPM_RC_1);
    }
    g_object_unref (map);

    return rc;
}
/*
 * Determine whether the command may return a transient handle. If so
 * be sure we have room in the handle map for it.
 */
gboolean
resource_manager_is_over_object_quota (ResourceManager *resmgr,
                                       Tpm2Command     *command)
{
    HandleMap   *handle_map;
    SessionData *session;
    gboolean     ret = FALSE;

    switch (tpm2_command_get_code (command)) {
    /* These commands load transient objects. */
    case TPM_CC_CreatePrimary:
    case TPM_CC_Load:
    case TPM_CC_LoadExternal:
        session = tpm2_command_get_session (command);
        handle_map = session_data_get_trans_map (session);
        if (handle_map_is_full (handle_map)) {
            ret = TRUE;
        }
        g_object_unref (session);
        g_object_unref (handle_map);
    }

    return ret;
}
/**
 * This function is invoked in response to the receipt of a Tpm2Command.
 * This is the place where we send the command buffer out to the TPM
 * through the AccessBroker which will eventually get it to the TPM for us.
 * The AccessBroker will send us back a Tpm2Response that we send back to
 * the client by way of our Sink object. The flow is roughly:
 * - Receive the Tpm2Command as a parameter
 * - Load all virtualized objects required by the command.
 * - Send the Tpm2Command out through the AccessBroker.
 * - Receive the response from the AccessBroker.
 * - Virtualize the new objects created by the command & referenced in the
 *   response.
 * - Enqueue the response back out to the processing pipeline through the
 *   Sink object.
 * - Flush all objects loaded for the command or as part of executing the
 *   command..
 */
void
resource_manager_process_tpm2_command (ResourceManager   *resmgr,
                                       Tpm2Command       *command)
{
    SessionData    *session;
    Tpm2Response   *response;
    TSS2_RC         rc = TSS2_RC_SUCCESS;
    HandleMapEntry *entries[4];
    guint           entry_count = 4;

    g_debug ("resource_manager_process_tpm2_command: resmgr: 0x%" PRIxPTR
             ", cmd: 0x%" PRIxPTR, (uintptr_t)resmgr, (uintptr_t)command);
    dump_command (command);
    session = tpm2_command_get_session (command);
    /* If session has reached quota limit kill command & send error response */
    if (resource_manager_is_over_object_quota (resmgr, command)) {
        response = tpm2_response_new_rc (session,
                                         TSS2_TABRMD_OBJECT_MEMORY);
        sink_enqueue (resmgr->sink, G_OBJECT (response));
        g_object_unref (response);
        g_object_unref (session);
        return;
    }
    /*
     * if necessary, load all related contexts and switch virtual for physical
     * handles in Tpm2Command
     */
    switch (tpm2_command_get_code (command)) {
    case TPM_CC_FlushContext:
        g_debug ("processing TPM_CC_FlushContext");
        rc = resource_manager_flush_context (resmgr, command);
        response = tpm2_response_new_rc (session, rc);
        entry_count = 0;
        g_object_unref (session);
        break;
    default:
        if (tpm2_command_get_handle_count (command) > 0) {
            resource_manager_load_contexts (resmgr, command, entries, &entry_count);
        }
        if (rc == TSS2_RC_SUCCESS) {
            response = access_broker_send_command (resmgr->access_broker,
                                                   command,
                                                   &rc);
            if (rc != TSS2_RC_SUCCESS)
                g_warning ("access_broker_send_command returned error: 0x%x", rc);
        if (response == NULL)
            g_error ("access_broker_send_command returned NULL Tpm2Response?");
        } else {
            response = tpm2_response_new_rc (session, rc);
            g_object_unref (session);
        }
        dump_response (response);
        /* transform the Tpm2Response */
        if (tpm2_response_has_handle (response)) {
            entries [entry_count] =
                resource_manager_virtualize_handle (resmgr, response);
            ++entry_count;
        }
        break;
    }
    sink_enqueue (resmgr->sink, G_OBJECT (response));
    g_object_unref (response);
    g_object_unref (session);
    /* flush contexts loaded for and created by the command */
    guint i;
    g_debug ("flushsave_context for %" PRIu32 " entries", entry_count);
    for (i = 0; i < entry_count; ++i) {
        rc = resource_manager_flushsave_context (resmgr, entries [i]);
        g_object_unref (entries [i]);
    }
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
                 "0x%" PRIxPTR, (uintptr_t)obj);
        if (obj == NULL) {
            g_debug ("resource_manager_thread: dequeued a null object");
            break;
        }
        if (IS_TPM2_COMMAND (obj)) {
            resource_manager_process_tpm2_command (resmgr, TPM2_COMMAND (obj));
            g_object_unref (obj);
        } else if (IS_CONTROL_MESSAGE (obj)) {
            ControlCode code =
                control_message_get_code (CONTROL_MESSAGE (obj));
            /* we must unref the message before processing the ControlCode
             * since the function may cause the thread to exit.
             */
            g_object_unref (obj);
            process_control_code (code);
        }
    }

    return (void*)0;
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
    g_debug ("resource_manager_cancel: enqueuing ControlMessage: 0x%" PRIxPTR,
             (uintptr_t)msg);
    message_queue_enqueue (resmgr->in_queue, G_OBJECT (msg));
    g_object_unref (msg);

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

    g_debug ("resource_manager_enqueue: ResourceManager: 0x%" PRIxPTR " obj: "
             "0x%" PRIxPTR, (uintptr_t)resmgr, (uintptr_t)obj);
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

    g_debug ("resource_manager_add_sink: ResourceManager: 0x%" PRIxPTR
             ", Sink: 0x%" PRIxPTR, (uintptr_t)resmgr, (uintptr_t)sink);
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

    g_debug ("resource_manager_set_property: 0x%" PRIxPTR,
             (uintptr_t)resmgr);
    switch (property_id) {
    case PROP_QUEUE_IN:
        resmgr->in_queue = g_value_get_object (value);
        g_debug ("  in_queue: 0x%" PRIxPTR, (uintptr_t)resmgr->in_queue);
        break;
    case PROP_SINK:
        if (resmgr->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        resmgr->sink = SINK (g_value_get_object (value));
        g_object_ref (resmgr->sink);
        g_debug ("  sink: 0x%" PRIxPTR, (uintptr_t)resmgr->sink);
        break;
    case PROP_ACCESS_BROKER:
        if (resmgr->access_broker != NULL) {
            g_warning ("  access_broker already set");
            break;
        }
        resmgr->access_broker = g_value_get_object (value);
        g_object_ref (resmgr->access_broker);
        g_debug ("  access_broker: 0x%" PRIxPTR, (uintptr_t)resmgr->access_broker);
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

    g_debug ("resource_manager_get_property: 0x%" PRIxPTR, (uintptr_t)resmgr);
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

    g_debug ("resource_manager_finalize: 0x%" PRIxPTR, (uintptr_t)resmgr);
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
