/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <tss2/tss2_rc.h>

#include "tabrmd.h"

#include "tpm2.h"
#include "tcti.h"
#include "tpm2-command.h"
#include "tpm2-response.h"
#include "util.h"

G_DEFINE_TYPE (Tpm2, tpm2, G_TYPE_OBJECT);

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
tpm2_set_property (GObject        *object,
                            guint           property_id,
                            GValue const   *value,
                            GParamSpec     *pspec)
{
    Tpm2 *self = TPM2 (object);

    g_debug (__func__);
    switch (property_id) {
    case PROP_SAPI_CTX:
        self->sapi_context = g_value_get_pointer (value);
        break;
    case PROP_TCTI:
        self->tcti = g_value_get_object (value);
        g_object_ref (self->tcti);
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
tpm2_get_property (GObject     *object,
                            guint        property_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
    Tpm2 *self = TPM2 (object);

    g_debug (__func__);
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
/*
 * Dispose function: release references to gobjects. We also free the SAPI
 * context here as well. Typically freeing this data would be done in the
 * finalize function but it can't be used w/o the underlying TCTI.
 */
static void
tpm2_dispose (GObject *obj)
{
    Tpm2 *self = TPM2 (obj);

    if (self->sapi_context != NULL) {
        Tss2_Sys_Finalize (self->sapi_context);
    }
    g_clear_pointer (&self->sapi_context, g_free);
    g_clear_object (&self->tcti);
    G_OBJECT_CLASS (tpm2_parent_class)->dispose (obj);
}
/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
tpm2_init (Tpm2 *tpm2)
{
    UNUSED_PARAM(tpm2);
    /* noop */
}
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set dispose, property get/set.
 * - Install properties.
 */
static void
tpm2_class_init (Tpm2Class *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tpm2_parent_class == NULL)
        tpm2_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose      = tpm2_dispose;
    object_class->get_property = tpm2_get_property;
    object_class->set_property = tpm2_set_property;

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

#define SUPPORTED_ABI_VERSION \
{ \
    .tssCreator = 1, \
    .tssFamily = 2, \
    .tssLevel = 1, \
    .tssVersion = 108, \
}

TSS2_SYS_CONTEXT*
sapi_context_init (Tcti *tcti)
{
    TSS2_SYS_CONTEXT *sapi_context;
    TSS2_TCTI_CONTEXT *tcti_context;
    TSS2_RC rc;
    size_t size;
    TSS2_ABI_VERSION abi_version = SUPPORTED_ABI_VERSION;

    assert (tcti != NULL);
    tcti_context = tcti_peek_context (tcti);
    assert (tcti_context != NULL);

    size = Tss2_Sys_GetContextSize (0);
    g_debug ("Allocating 0x%zx bytes for SAPI context", size);
    /* NOTE: g_malloc0 will terminate the program if allocation fails */
    sapi_context = (TSS2_SYS_CONTEXT*)g_malloc0 (size);

    rc = Tss2_Sys_Initialize (sapi_context, size, tcti_context, &abi_version);
    if (rc != TSS2_RC_SUCCESS) {
        g_free (sapi_context);
        RC_WARN ("Tss2_Sys_Initialize", rc);
        return NULL;
    }
    return sapi_context;
}

TSS2_RC
tpm2_send_tpm_startup (Tpm2 *tpm2)
{
    TSS2_RC rc;

    assert (tpm2 != NULL);

    rc = Tss2_Sys_Startup (tpm2->sapi_context, TPM2_SU_CLEAR);
    if (rc != TSS2_RC_SUCCESS && rc != TPM2_RC_INITIALIZE)
        RC_WARN ("Tss2_Sys_Startup", rc);
    else
        rc = TSS2_RC_SUCCESS;

    return rc;
}
/**
 * This is a very thin wrapper around the mutex mediating access to the
 * TSS2_SYS_CONTEXT. It locks the mutex.
 *
 * NOTE: Any errors locking the mutex are fatal and will cause the program
 * to halt.
 */
void
tpm2_lock (Tpm2 *tpm2)
{
    gint error;

    assert (tpm2 != NULL);

    error = pthread_mutex_lock (&tpm2->sapi_mutex);
    if (error != 0) {
        switch (error) {
        case EINVAL:
            g_error ("Tpm2: attempted to lock uninitialized mutex");
            break;
        default:
            g_error ("Tpm2: unknown error attempting to lock SAPI "
                     "mutex: 0x%x", error);
            break;
        }
    }
}
/**
 * This is a very thin wrapper around the mutex mediating access to the
 * TSS2_SYS_CONTEXT. It unlocks the mutex.
 *
 * NOTE: Any errors locking the mutex are fatal and will cause the program
 * to halt.
 */
void
tpm2_unlock (Tpm2 *tpm2)
{
    gint error;

    assert (tpm2 != NULL);

    error= pthread_mutex_unlock (&tpm2->sapi_mutex);
    if (error != 0) {
        switch (error) {
        case EINVAL:
            g_error ("Tpm2: attempted to unlock uninitialized mutex");
            break;
        default:
            g_error ("Tpm2: unknown error attempting to unlock SAPI "
                     "mutex: 0x%x", error);
            break;
        }
    }
}
/**
 * Query the TPM for fixed (TPM2_PT_FIXED) TPM properties.
 * This function is intended for internal use only. The caller MUST
 * hold the sapi_mutex lock before calling.
 */
TSS2_RC
tpm2_get_tpm_properties_fixed (TSS2_SYS_CONTEXT     *sapi_context,
                                        TPMS_CAPABILITY_DATA *capability_data)
{
    TSS2_RC          rc;
    TPMI_YES_NO      more_data;

    assert (sapi_context != NULL);
    assert (capability_data != NULL);

    g_debug ("tpm2_get_tpm_properties_fixed");
    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 TPM2_CAP_TPM_PROPERTIES,
                                 /* get TPM2_PT_FIXED TPM property group */
                                 TPM2_PT_FIXED,
                                 /* get all properties in the group */
                                 TPM2_MAX_TPM_PROPERTIES,
                                 &more_data,
                                 capability_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_GetCapability", rc);
        return rc;
    }
    /* sanity check the property returned */
    if (capability_data->capability != TPM2_CAP_TPM_PROPERTIES)
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
tpm2_get_fixed_property (Tpm2           *tpm2,
                                  TPM2_PT                  property,
                                  guint32                *value)
{
    unsigned int i;

    assert (tpm2 != NULL);
    assert (value != NULL);

    if (tpm2->properties_fixed.data.tpmProperties.count == 0) {
        return TSS2_RESMGR_RC_INTERNAL_ERROR;
    }
    for (i = 0; i < tpm2->properties_fixed.data.tpmProperties.count; ++i) {
        if (tpm2->properties_fixed.data.tpmProperties.tpmProperty[i].property == property) {
            *value = tpm2->properties_fixed.data.tpmProperties.tpmProperty[i].value;
            return TSS2_RC_SUCCESS;
        }
    }
    return TSS2_RESMGR_RC_BAD_VALUE;
}
/*
 * This function exposes the underlying SAPI context in the Tpm2.
 * It locks the Tpm2 object and returns the SAPI context for use
 * by the caller. Do not call this function if you already hold the
 * Tpm2 lock. If you do you'll deadlock.
 * When done with the context the caller must unlock the Tpm2.
 */
TSS2_SYS_CONTEXT*
tpm2_lock_sapi (Tpm2 *tpm2)
{
    assert (tpm2 != NULL);
    assert (tpm2->sapi_context != NULL);

    tpm2_lock (tpm2);

    return tpm2->sapi_context;
}
/**
 * Return the TPM2_PT_TPM2_MAX_RESPONSE_SIZE fixed TPM property.
 */
TSS2_RC
tpm2_get_max_response (Tpm2 *tpm2,
                                guint32      *value)
{
    return tpm2_get_fixed_property (tpm2,
                                             TPM2_PT_MAX_RESPONSE_SIZE,
                                             value);
}
/*
 * Get a response buffer from the TPM. Return the TSS2_RC through the
 * 'rc' parameter. Returns a buffer (that must be freed by the caller)
 * containing the response from the TPM. Determine the size of the buffer
 * by reading the size field from the TPM command header.
 */
static TSS2_RC
tpm2_get_response (Tpm2 *tpm2,
                            uint8_t     **buffer,
                            size_t       *buffer_size)
{
    TSS2_RC rc;
    guint32 max_size;

    assert (tpm2 != NULL);
    assert (buffer != NULL);
    assert (buffer_size != NULL);

    rc = tpm2_get_max_response (tpm2, &max_size);
    if (rc != TSS2_RC_SUCCESS)
        return rc;

    *buffer = calloc (1, max_size);
    if (*buffer == NULL) {
        g_warning ("failed to allocate buffer for Tpm2Response: %s",
                   strerror (errno));
        return RM_RC (TPM2_RC_MEMORY);
    }
    *buffer_size = max_size;
    rc = tcti_receive (tpm2->tcti, buffer_size, *buffer, TSS2_TCTI_TIMEOUT_BLOCK);
    if (rc != TSS2_RC_SUCCESS) {
        free (*buffer);
        return rc;
    }
    *buffer = realloc (*buffer, *buffer_size);

    return rc;
}
/**
 * In the most simple case the caller will want to send just a single
 * command represented by a Tpm2Command object. The response is passed
 * back as the return value. The response code from the TCTI (not the TPM)
 * is returned through the 'rc' out parameter.
 * The caller MUST NOT hold the lock when calling. This function will take
 * the lock for itself.
 * Additionally this function *WILL ONLY* return a NULL Tpm2Response
 * pointer if it's unable to allocate memory for the object. In all other
 * error cases this function will create a Tpm2Response object with the
 * appropriate RC populated.
 */
Tpm2Response*
tpm2_send_command (Tpm2  *tpm2,
                            Tpm2Command   *command,
                            TSS2_RC       *rc)
{
    Tpm2Response   *response = NULL;
    Connection     *connection = NULL;
    guint8         *buffer = NULL;
    size_t          buffer_size = 0;

    g_debug (__func__);
    assert (tpm2 != NULL);
    assert (command != NULL);
    assert (rc != NULL);

    tpm2_lock (tpm2);
    *rc = tcti_transmit (tpm2->tcti,
                         tpm2_command_get_size (command),
                         tpm2_command_get_buffer (command));
    if (*rc != TSS2_RC_SUCCESS)
        goto unlock_out;
    *rc = tpm2_get_response (tpm2, &buffer, &buffer_size);
    if (*rc != TSS2_RC_SUCCESS) {
        goto unlock_out;
    }
    tpm2_unlock (tpm2);
    connection = tpm2_command_get_connection (command);
    response = tpm2_response_new (connection,
                                  buffer,
                                  buffer_size,
                                  tpm2_command_get_attributes (command));
    g_clear_object (&connection);
    return response;

unlock_out:
    tpm2_unlock (tpm2);
    if (!connection)
        connection = tpm2_command_get_connection (command);
    response = tpm2_response_new_rc (connection, *rc);
    g_object_unref (connection);
    return response;
}
/**
 * Create new TPM access tpm2 (TPM2) object. This includes
 * using the provided TCTI to send the TPM the startup command and
 * creating the TCTI mutex.
 */
Tpm2*
tpm2_new (Tcti *tcti)
{
    Tpm2       *tpm2;
    TSS2_SYS_CONTEXT   *sapi_context;

    assert (tcti != NULL);

    sapi_context = sapi_context_init (tcti);
    tpm2 = TPM2 (g_object_new (TYPE_TPM2,
                                          "sapi-ctx", sapi_context,
                                          "tcti", tcti,
                                          NULL));
    return tpm2;
}
/*
 * Initialize the Tpm2. This is all about initializing internal data
 * that normally we would want to do in a constructor. But since this
 * initialization requires reaching out to the TPM and could fail we don't
 * want to do it in the constructor / _new function. So we put it here in
 * an explicit _init function that must be executed after the object has
 * been instantiated.
 */
TSS2_RC
tpm2_init_tpm (Tpm2 *tpm2)
{
    TSS2_RC rc;

    g_debug (__func__);
    assert (tpm2 != NULL);

    if (tpm2->initialized)
        return TSS2_RC_SUCCESS;
    pthread_mutex_init (&tpm2->sapi_mutex, NULL);
    rc = tpm2_send_tpm_startup (tpm2);
    if (rc != TSS2_RC_SUCCESS)
        goto out;
    rc = tpm2_get_tpm_properties_fixed (tpm2->sapi_context,
                                                 &tpm2->properties_fixed);
    if (rc != TSS2_RC_SUCCESS)
        goto out;
    tpm2->initialized = true;
out:
    return rc;
}
/*
 * Query the TPM for the current number of loaded transient objects.
 */
 TSS2_RC
tpm2_get_trans_object_count (Tpm2 *tpm2,
                                      uint32_t     *count)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    TSS2_SYS_CONTEXT *sapi_context;
    TPMI_YES_NO more_data;
    TPMS_CAPABILITY_DATA capability_data = { 0, };

    assert (tpm2 != NULL);
    assert (count != NULL);

    sapi_context = tpm2_lock_sapi (tpm2);
    /*
     * GCC gets confused by the TPM2_TRANSIENT_FIRST constant being used for
     * the 4th parameter. It assumes that it's a signed type which causes
     * -Wsign-conversion to complain. Casting to UINT32 is all we can do.
     */
    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 TPM2_CAP_HANDLES,
                                 (UINT32)TPM2_TRANSIENT_FIRST,
                                 TPM2_TRANSIENT_LAST - TPM2_TRANSIENT_FIRST,
                                 &more_data,
                                 &capability_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_GetCapability", rc);
        goto out;
    }
    *count = capability_data.data.handles.count;
out:
    tpm2_unlock (tpm2);
    return rc;
}
TSS2_RC
tpm2_context_load (Tpm2 *tpm2,
                            TPMS_CONTEXT *context,
                            TPM2_HANDLE   *handle)
{
    TSS2_RC           rc;
    TSS2_SYS_CONTEXT *sapi_context;

    assert (tpm2 != NULL);
    assert (context != NULL);
    assert (handle != NULL);

    sapi_context = tpm2_lock_sapi (tpm2);
    rc = Tss2_Sys_ContextLoad (sapi_context, context, handle);
    tpm2_unlock (tpm2);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_ContextLoad", rc);
    }

    return rc;
}
/*
 * This function is a simple wrapper around the TPM2_ContextSave command.
 * It will save the context associated with the provided handle, returning
 * the TPMS_CONTEXT to the caller. The response code returned will be
 * TSS2_RC_SUCCESS or an RC indicating failure from the TPM.
 */
TSS2_RC
tpm2_context_save (Tpm2 *tpm2,
                            TPM2_HANDLE    handle,
                            TPMS_CONTEXT *context)
{
    TSS2_RC rc;
    TSS2_SYS_CONTEXT *sapi_context;

    assert (tpm2 != NULL);
    assert (context != NULL);

    g_debug ("tpm2_context_save: handle 0x%08" PRIx32, handle);
    sapi_context = tpm2_lock_sapi (tpm2);
    rc = Tss2_Sys_ContextSave (sapi_context, handle, context);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_ContextSave", rc);
    }
    tpm2_unlock (tpm2);

    return rc;
}
/*
 * This function is a simple wrapper around the TPM2_FlushContext command.
 */
TSS2_RC
tpm2_context_flush (Tpm2 *tpm2,
                             TPM2_HANDLE    handle)
{
    TSS2_RC rc;
    TSS2_SYS_CONTEXT *sapi_context;

    assert (tpm2 != NULL);

    g_debug ("tpm2_context_flush: handle 0x%08" PRIx32, handle);
    sapi_context = tpm2_lock_sapi (tpm2);
    rc = Tss2_Sys_FlushContext (sapi_context, handle);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_FlushContext", rc);
    }
    tpm2_unlock (tpm2);

    return rc;
}
TSS2_RC
tpm2_context_saveflush (Tpm2 *tpm2,
                                 TPM2_HANDLE    handle,
                                 TPMS_CONTEXT *context)
{
    TSS2_RC           rc;
    TSS2_SYS_CONTEXT *sapi_context;

    assert (tpm2 != NULL);
    assert (context != NULL);

    g_debug ("tpm2_context_save: handle 0x%" PRIx32, handle);
    sapi_context = tpm2_lock_sapi (tpm2);
    rc = Tss2_Sys_ContextSave (sapi_context, handle, context);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_ContextSave", rc);
        goto out;
    }
    g_debug ("tpm2_context_flush: handle 0x%" PRIx32, handle);
    rc = Tss2_Sys_FlushContext (sapi_context, handle);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_FlushContext", rc);
    }
out:
    tpm2_unlock (tpm2);
    return rc;
}
/*
 * Flush all handles in a given range. This function will return an error if
 * we're unable to query for handles within the requested range. Failures to
 * flush handles returned will be ignored since our goal here is to flush as
 * many as possible.
 */
TSS2_RC
tpm2_flush_all_unlocked (Tpm2     *tpm2,
                                  TSS2_SYS_CONTEXT *sapi_context,
                                  TPM2_RH            first,
                                  TPM2_RH            last)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    TPMI_YES_NO more_data;
    TPMS_CAPABILITY_DATA capability_data = { 0, };
    TPM2_HANDLE handle;
    size_t i;

    g_debug ("%s: first: 0x%08" PRIx32 ", last: 0x%08" PRIx32,
             __func__, first, last);
    assert (tpm2 != NULL);
    assert (sapi_context != NULL);

    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 TPM2_CAP_HANDLES,
                                 first,
                                 last - first,
                                 &more_data,
                                 &capability_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_GetCapability", rc);
        return rc;
    }
    g_debug ("%s: got %u handles", __func__, capability_data.data.handles.count);
    for (i = 0; i < capability_data.data.handles.count; ++i) {
        handle = capability_data.data.handles.handle [i];
        g_debug ("%s: flushing context with handle: 0x%08" PRIx32, __func__,
                 handle);
        rc = Tss2_Sys_FlushContext (sapi_context, handle);
        if (rc != TSS2_RC_SUCCESS) {
            RC_WARN ("Tss2_Sys_FlushContext", rc);
        }
    }

    return TSS2_RC_SUCCESS;
}
void
tpm2_flush_all_context (Tpm2 *tpm2)
{
    TSS2_SYS_CONTEXT *sapi_context;

    g_debug (__func__);
    assert (tpm2 != NULL);

    sapi_context = tpm2_lock_sapi (tpm2);
    tpm2_flush_all_unlocked (tpm2,
                                      sapi_context,
                                      TPM2_ACTIVE_SESSION_FIRST,
                                      TPM2_ACTIVE_SESSION_LAST);
    tpm2_flush_all_unlocked (tpm2,
                                      sapi_context,
                                      TPM2_LOADED_SESSION_FIRST,
                                      TPM2_LOADED_SESSION_LAST);
    tpm2_flush_all_unlocked (tpm2,
                                      sapi_context,
                                      TPM2_TRANSIENT_FIRST,
                                      TPM2_TRANSIENT_LAST);
    tpm2_unlock (tpm2);
}

TSS2_RC
tpm2_get_command_attrs (Tpm2 *tpm2,
                                 UINT32 *count,
                                 TPMA_CC **attrs)
{
    TSS2_RC rc;
    TPMI_YES_NO more = TPM2_NO;
    TPMS_CAPABILITY_DATA cap_data = { 0, };
    TSS2_SYS_CONTEXT *sys_ctx;

    assert (tpm2 != NULL);
    assert (count != NULL);
    assert (attrs != NULL);

    sys_ctx = tpm2_lock_sapi (tpm2);
    rc = Tss2_Sys_GetCapability (sys_ctx,
                                 NULL,
                                 TPM2_CAP_COMMANDS,
                                 TPM2_CC_FIRST,
                                 TPM2_MAX_CAP_CC,
                                 &more,
                                 &cap_data,
                                 NULL);
    tpm2_unlock (tpm2);
    if (rc != TSS2_RC_SUCCESS) {
        RC_WARN ("Tss2_Sys_GetCapability", rc);
        return rc;
    }

    *count = cap_data.data.command.count;
    *attrs = g_malloc0 (*count * sizeof (TPMA_CC));
    memcpy (*attrs,
            cap_data.data.command.commandAttributes,
            *count * sizeof (TPMA_CC));

    return rc;
}
