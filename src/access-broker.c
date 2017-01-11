#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>

#include "tabrmd.h"

#include "access-broker.h"
#include "tpm2-command.h"
#include "tpm2-response.h"

static gpointer access_broker_parent_class = NULL;

enum {
    PROP_0,
    PROP_SAPI_CTX,
    PROP_TCTI,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * GObject property setter.
 */
static void
access_broker_set_property (GObject        *object,
                            guint           property_id,
                            GValue const   *value,
                            GParamSpec     *pspec)
{
    AccessBroker *self = ACCESS_BROKER (object);

    g_debug ("access_broker_set_property: 0x%x", self);
    switch (property_id) {
    case PROP_SAPI_CTX:
        self->sapi_context = g_value_get_pointer (value);
        g_debug ("  sapi_context: 0x%x", self->sapi_context);
        break;
    case PROP_TCTI:
        if (self->tcti != NULL) {
            g_warning ("  tcti already set");
            break;
        }
        self->tcti = g_value_get_object (value);
        g_object_ref (self->tcti);
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
access_broker_get_property (GObject     *object,
                            guint        property_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
    AccessBroker *self = ACCESS_BROKER (object);

    g_debug ("access_broker_get_property: 0x%x", self);
    switch (property_id) {
    case PROP_SAPI_CTX:
        g_value_set_pointer (value, self->sapi_context);
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
 * Bring down the ACCESS_BROKER gracefully.
 */
static void
access_broker_finalize (GObject *obj)
{
    AccessBroker *access_broker = ACCESS_BROKER (obj);

    g_debug ("access_broker_finalize: 0x%x", access_broker);
    if (access_broker == NULL)
        g_error ("access_broker_free passed NULL AccessBroker pointer");
    if (access_broker->sapi_context != NULL)
        free (access_broker->sapi_context);
    if (access_broker->tcti != NULL)
        g_object_unref (access_broker->tcti);
    g_debug ("call to parent finalize");
    if (access_broker_parent_class)
        G_OBJECT_CLASS (access_broker_parent_class)->finalize (obj);
}
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set finalize, property get/set.
 * - Install properties.
 */
static void
access_broker_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (access_broker_parent_class == NULL)
        access_broker_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = access_broker_finalize;
    object_class->get_property = access_broker_get_property;
    object_class->set_property = access_broker_set_property;

    obj_properties [PROP_SAPI_CTX] =
        g_param_spec_pointer ("sapi-ctx",
                              "SAPI context",
                              "TSS2_SYS_CONTEXT instance",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
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
 * GObject boilerplate get_type.
 */
GType
access_broker_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "AccessBroker",
                                              sizeof (AccessBrokerClass),
                                              (GClassInitFunc) access_broker_class_init,
                                              sizeof (AccessBroker),
                                              NULL,
                                              0);
    }
    return type;
}
static TSS2_SYS_CONTEXT*
sapi_context_init (Tcti *tcti)
{
    TSS2_SYS_CONTEXT *sapi_context;
    TSS2_TCTI_CONTEXT *tcti_context;
    TSS2_RC rc;
    size_t size;
    TSS2_ABI_VERSION abi_version = {
        .tssCreator = TSSWG_INTEROP,
        .tssFamily  = TSS_SAPI_FIRST_FAMILY,
        .tssLevel   = TSS_SAPI_FIRST_LEVEL,
        .tssVersion = TSS_SAPI_FIRST_VERSION,
    };

    g_debug ("sapi_context_init w/ Tcti: 0x%x", tcti);
    tcti_context = tcti_peek_context (tcti);
    if (tcti_context == NULL)
        g_error ("NULL TCTI_CONTEXT");
    size = Tss2_Sys_GetContextSize (0);
    g_debug ("Allocating 0x%x bytes for SAPI context", size);
    sapi_context = (TSS2_SYS_CONTEXT*)g_malloc0 (size);
    if (sapi_context == NULL) {
        g_error ("Failed to allocate 0x%zx bytes for the SAPI context", size);
        return NULL;
    }
    rc = Tss2_Sys_Initialize (sapi_context, size, tcti_context, &abi_version);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Failed to initialize SAPI context: 0x%x", rc);
        return NULL;
    }
    return sapi_context;
}

static TSS2_RC
access_broker_send_tpm_startup (AccessBroker *broker)
{
    TSS2_RC rc;

    rc = Tss2_Sys_Startup (broker->sapi_context, TPM_SU_CLEAR);
    if (rc != TSS2_RC_SUCCESS && rc != TPM_RC_INITIALIZE)
        g_warning ("Tss2_Sys_Startup returned unexpected RC: 0x%x", rc);
    else
        rc = TSS2_RC_SUCCESS;

    return rc;
}
/**
 * This is a very thin wrapper around the mutex mediating access to the
 * TSS2_SYS_CONTEXT. It locks the mutex.
 */
gint
access_broker_lock (AccessBroker *broker)
{
    gint error;

    error = pthread_mutex_lock (&broker->sapi_mutex);
    if (error != 0) {
        switch (error) {
        case EINVAL:
            g_error ("AccessBroker: attempted to lock uninitialized mutex");
            break;
        default:
            g_error ("AccessBroker: unkonwn error attempting to lock SAPI "
                     "mutex: 0x%x", error);
            break;
        }
    }
    return error;
}
/**
 * This is a very thin wrapper around the mutex mediating access to the
 * TSS2_SYS_CONTEXT. It unlocks the mutex.
 */
gint
access_broker_unlock (AccessBroker *broker)
{
    gint error;

    error= pthread_mutex_unlock (&broker->sapi_mutex);
    if (error != 0) {
        switch (error) {
        case EINVAL:
            g_error ("AccessBroker: attempted to unlock uninitialized mutex");
            break;
        default:
            g_error ("AccessBroker: unknown error attempting to unlock SAPI "
                     "mutex: 0x%x", error);
            break;
        }
    }
    return error;
}
/**
 * Query the TPM for fixed (PT_FIXED) TPM properties.
 * This function is intended for internal use only. The caller MUST
 * hold the sapi_mutex lock before calling.
 */
TSS2_RC
access_broker_get_tpm_properties_fixed (TSS2_SYS_CONTEXT     *sapi_context,
                                        TPMS_CAPABILITY_DATA *capability_data)
{
    TSS2_RC          rc;
    TPMI_YES_NO      more_data;

    g_debug ("access_broker_get_tpm_properties_fixed");
    if (capability_data == NULL)
        return TSS2_TABRMD_BAD_VALUE;
    if (sapi_context == NULL)
        return TSS2_TABRMD_SAPI_INIT;
    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 TPM_CAP_TPM_PROPERTIES,
                                 /* get PT_FIXED TPM property group */
                                 PT_FIXED,
                                 /* get all properties in the group */
                                 MAX_TPM_PROPERTIES,
                                 &more_data,
                                 capability_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Failed to GetCapability: TPM_CAP_TPM_PROPERTIES, "
                 "PT_FIXED: 0x%x", rc);
        return rc;
    }
    /* sanity check the property returned */
    if (capability_data->capability != TPM_CAP_TPM_PROPERTIES)
        g_warning ("GetCapability returned wrong capability: 0x%x",
                   capability_data->capability);
    return rc;
}
/**
 * Query the TM for a specific tagged property from the collection of
 * fixed TPM properties. If the requested property is found then the
 * 'value' parameter will be set accordingly. If no such property exists
 * then TSS2_TABRMD_BAD_VALUE will be returned.
 */
TSS2_RC
access_broker_get_fixed_property (AccessBroker           *broker,
                                  TPM_PT                  property,
                                  guint32                *value)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    gboolean found = false;
    gint i;

    if (broker->properties_fixed.data.tpmProperties.count == 0) {
        rc = TSS2_TABRMD_INTERNAL_ERROR;
        goto out;
    }
    for (i = 0; i < broker->properties_fixed.data.tpmProperties.count; ++i) {
        if (broker->properties_fixed.data.tpmProperties.tpmProperty[i].property == property) {
            found = true;
            *value = broker->properties_fixed.data.tpmProperties.tpmProperty[i].value;
        }
    }
    if (!found)
        rc = TSS2_TABRMD_BAD_VALUE;
out:
    return rc;
}
/*
 * This function exposes the underlying SAPI context in the AccessBroker.
 * It locks the AccessBroker object and returns the SAPI context for use
 * by the caller. Do not call this function if you already hold the
 * AccessBroker lock. If you do you'll deadlock.
 * When done with the context the caller must unlock the AccessBroker.
 */
TSS2_SYS_CONTEXT*
access_broker_lock_sapi (AccessBroker *broker)
{
    if (access_broker_lock (broker) != 0)
        return NULL;
    return broker->sapi_context;
}
/*
 * Return the TPM_PT_TOTAL_COMAMNDS fixed TPM property.
 */
TSS2_RC
access_broker_get_total_commands (AccessBroker *broker,
                                  guint        *value)
{
    return access_broker_get_fixed_property (broker,
                                             TPM_PT_TOTAL_COMMANDS,
                                             value);
}
/**
 * Return the TPM_PT_MAX_COMMAND_SIZE fixed TPM property.
 */
TSS2_RC
access_broker_get_max_command (AccessBroker *broker,
                               guint32      *value)
{
    return access_broker_get_fixed_property (broker,
                                             TPM_PT_MAX_COMMAND_SIZE,
                                             value);
}
/**
 * Return the TPM_PT_MAX_RESPONSE_SIZE fixed TPM property.
 */
TSS2_RC
access_broker_get_max_response (AccessBroker *broker,
                                guint32      *value)
{
    return access_broker_get_fixed_property (broker,
                                             TPM_PT_MAX_RESPONSE_SIZE,
                                             value);
}
/* Send the parameter Tpm2Command to the TPM. Return the TSS2_RC. */
static TSS2_RC
access_broker_send_cmd (AccessBroker *broker,
                        Tpm2Command  *command)
{
    TSS2_RC rc;

    rc = tcti_transmit (broker->tcti,
                        tpm2_command_get_size (command),
                        tpm2_command_get_buffer (command));
    if (rc != TSS2_RC_SUCCESS)
        g_warning ("AccessBroker 0x%" PRIxPTR " failed to transmit "
                   "Tpm2Command 0x%" PRIxPTR ": 0x%" PRIx32,
                   broker, command, rc);
    return rc;
}
/*
 * Get a response buffer from the TPM. Return the TSS2_RC through the
 * 'rc' parameter. Returns a buffer (that must be freed by the caller)
 * containing the response from the TPM. Determine the size of the buffer
 * by reading the size field from the TPM command header.
 */
static guint8*
access_broker_get_response (AccessBroker *broker,
                            TSS2_RC      *rc)
{
    guint32   max_resp_size;
    guint8   *buffer;
    size_t    size;

    *rc = access_broker_get_max_response (broker, &max_resp_size);
    if (*rc != TSS2_RC_SUCCESS)
        return NULL;
    buffer = calloc (1, max_resp_size);
    if (buffer == NULL) {
        g_warning ("failed to allocate buffer for Tpm2Response: %s",
                   strerror (errno));
        return NULL;
    }
    size = max_resp_size;
    *rc = tcti_receive (broker->tcti, &size, buffer, TSS2_TCTI_TIMEOUT_BLOCK);
    buffer = realloc (buffer, size);

    return buffer;
}
/**
 * In the most simple case the caller will want to send just a single
 * command represented by a Tpm2Command object. The response is passed
 * back as the return value. The resonse code is returend through the
 * 'rc' out parameter.
 * The caller MUST NOT hold the lock when calling. This function will take
 * the lock for itself.
 * Additionally this function *WILL ONLY* return a NULL Tpm2Response
 * pointer if it's unable to allocate memory for the object. In all other
 * error cases this function will create a Tpm2Response object with the
 * appropriate RC populated.
 */
Tpm2Response*
access_broker_send_command (AccessBroker  *broker,
                            Tpm2Command   *command,
                            TSS2_RC       *rc)
{
    Tpm2Response   *response = NULL;
    SessionData    *session = NULL;
    gint            error;
    size_t          size;
    guint32         max_resp_size;
    guint8         *buffer;

    g_debug ("access_broker_send_command: AccessBroker: 0x%x, "
             "Tpm2Command: 0x%x", broker, command);
    error = access_broker_lock (broker);
    if (error) {
        *rc = TSS2_TABRMD_INTERNAL_ERROR;
        goto err_out;
    }
    *rc = access_broker_send_cmd (broker, command);
    if (*rc != TSS2_RC_SUCCESS)
        goto unlock_out;
    buffer = access_broker_get_response (broker, rc);
    if (*rc != TSS2_RC_SUCCESS) {
        if (buffer != NULL)
            free (buffer);
        goto unlock_out;
    }
    error = access_broker_unlock (broker);
    if (error)
        g_error ("access_broker: Failed to unlock SAPI mutex.");
    session = tpm2_command_get_session (command);
    response = tpm2_response_new (session,
                                  buffer,
                                  tpm2_command_get_attributes (command));
    g_object_unref (session);
    return response;

unlock_out:
    access_broker_unlock (broker);
err_out:
    if (!session)
        session = tpm2_command_get_session (command);
    response = tpm2_response_new_rc (session, *rc);
    g_object_unref (session);
    return response;
}
/**
 * Create new TPM access broker (ACCESS_BROKER) object. This includes
 * using the provided TCTI to send the TPM the startup command and
 * creating the TCTI mutex.
 */
AccessBroker*
access_broker_new (Tcti *tcti)
{
    AccessBroker       *broker;
    TSS2_SYS_CONTEXT   *sapi_context;
    pthread_mutex_t    *mutex;

    if (tcti == NULL)
        g_error ("access_broker_new passed NULL Tcti");

    sapi_context = sapi_context_init (tcti);
    broker = ACCESS_BROKER (g_object_new (TYPE_ACCESS_BROKER,
                                          "sapi-ctx", sapi_context,
                                          "tcti", tcti,
                                          NULL));
    return broker;
}
/*
 * Initialize the AccessBroker. This is all about initializing internal data
 * that normally we would want to do in a constructor. But since this
 * initialization requires reaching out to the TPM and could fail we don't
 * want to do it in the constructur / _new function. So we put it here in
 * an explicit _init function that must be executed after the object has
 * been instantiated.
 */
TSS2_RC
access_broker_init (AccessBroker *broker)
{
    TSS2_RC rc;

    g_debug ("access_broker_init: 0x%x", broker);
    if (broker->initialized)
        return;
    pthread_mutex_init (&broker->sapi_mutex, NULL);
    rc = access_broker_send_tpm_startup (broker);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("access_broker_sent_tpm_startup failed: 0x%x", rc);
        goto out;
    }
    rc = access_broker_get_tpm_properties_fixed (broker->sapi_context,
                                                 &broker->properties_fixed);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to get fixed TPM properties: 0x%" PRIx32, rc);
        goto out;
    }
    broker->initialized = true;
out:
    return rc;
}
/*
 * Query the TPM for the current number of loaded transient objects.
 */
 TSS2_RC
access_broker_get_trans_object_count (AccessBroker *broker,
                                      uint32_t     *count)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    TSS2_SYS_CONTEXT *sapi_context;
    TPMI_YES_NO more_data;
    TPMS_CAPABILITY_DATA capability_data = { 0, };

    if (broker == NULL || count == NULL)
        g_error ("get_loaded_transient_object_count: got NULL parameter");
    sapi_context = access_broker_lock_sapi (broker);
    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 TPM_CAP_HANDLES,
                                 TRANSIENT_FIRST,
                                 TRANSIENT_LAST - TRANSIENT_FIRST,
                                 &more_data,
                                 &capability_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS)
        goto out;
    *count = capability_data.data.handles.count;
out:
    access_broker_unlock (broker);
    return rc;
}
TSS2_RC
access_broker_context_load (AccessBroker *broker,
                            TPMS_CONTEXT *context,
                            TPM_HANDLE   *handle)
{
    TSS2_RC           rc;
    TSS2_SYS_CONTEXT *sapi_context;

    if (broker == NULL || context == NULL || handle == NULL)
        g_error ("access_broker_context_load received NULL parameter");

    sapi_context = access_broker_lock_sapi (broker);
    rc = Tss2_Sys_ContextLoad (sapi_context, context, handle);
    access_broker_unlock (broker);
    if (rc == TSS2_RC_SUCCESS)
        g_debug ("Tss2_Sys_ContextLoad: successfully load context at 0x%"
                 PRIxPTR " got handle 0x%" PRIx32, context, *handle);
    else
        g_warning ("Tss2_Sys_ContextLoad: failed to load context for "
                   "context at: 0x%" PRIxPTR ", TSS2_RC: 0x%" PRIx32,
                   context, rc);

    return rc;
}
TSS2_RC
access_broker_context_saveflush (AccessBroker *broker,
                                 TPM_HANDLE    handle,
                                 TPMS_CONTEXT *context)
{
    TSS2_RC           rc;
    TSS2_SYS_CONTEXT *sapi_context;

    if (broker == NULL || context == NULL)
        g_error ("access_broker_context_save received NULL parameter");

    g_debug ("access_broker_context_saveflush: handle 0x%" PRIx32, handle);
    sapi_context = access_broker_lock_sapi (broker);
    rc = Tss2_Sys_ContextSave (sapi_context, handle, context);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("Tss2_Sys_ContextSave: failed to save context for "
                   "handle: 0x%" PRIxPTR " TSS2_RC: 0x%" PRIx32, handle, rc);
        goto out;
    }
    g_debug ("access_broker_context_saveflush: handle 0x%" PRIx32, handle);
    rc = Tss2_Sys_FlushContext (sapi_context, handle);
    if (rc != TSS2_RC_SUCCESS)
        g_warning("Tss2_Sys_FlushContext: failed to flushed context for "
                  "handle: 0x%" PRIxPTR ", TSS2_RC: 0x%" PRIx32, handle, rc);
out:
    access_broker_unlock (broker);
    return rc;
}
