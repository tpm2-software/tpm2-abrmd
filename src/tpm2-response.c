/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <tss2/tss2_tpm2_types.h>
#include <tss2/tss2_mu.h>

#include "tpm2-header.h"
#include "tpm2-response.h"
#include "util.h"

#define HANDLE_AREA_OFFSET TPM_HEADER_SIZE
#define HANDLE_OFFSET (HANDLE_AREA_OFFSET)
#define HANDLE_END_OFFSET (HANDLE_OFFSET + sizeof (TPM2_HANDLE))
#define HANDLE_GET(buffer) (*(TPM2_HANDLE*)(&buffer [HANDLE_OFFSET]))

#define TPM_RESPONSE_TAG(buffer) (*(TPM2_ST*)buffer)
#define TPM_RESPONSE_SIZE(buffer) (*(guint32*)(buffer + \
                                               sizeof (TPM2_ST)))
#define TPM_RESPONSE_CODE(buffer) (*(TSS2_RC*)(buffer + \
                                              sizeof (TPM2_ST) + \
                                              sizeof (UINT32)))

G_DEFINE_TYPE (Tpm2Response, tpm2_response, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_ATTRIBUTES,
    PROP_SESSION,
    PROP_BUFFER,
    PROP_BUFFER_SIZE,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * GObject property setter.
 */
static void
tpm2_response_set_property (GObject        *object,
                            guint           property_id,
                            GValue const   *value,
                            GParamSpec     *pspec)
{
    Tpm2Response *self = TPM2_RESPONSE (object);

    switch (property_id) {
    case PROP_ATTRIBUTES:
        self->attributes = (TPMA_CC)g_value_get_uint (value);
        break;
    case PROP_BUFFER:
        if (self->buffer != NULL) {
            g_warning ("  buffer already set");
            break;
        }
        self->buffer = (guint8*)g_value_get_pointer (value);
        break;
    case PROP_BUFFER_SIZE:
        self->buffer_size = g_value_get_uint (value);
        break;
    case PROP_SESSION:
        if (self->connection != NULL) {
            g_warning ("  connection already set");
            break;
        }
        self->connection = g_value_get_object (value);
        if (self->connection) {
            g_object_ref (self->connection);
        }
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
tpm2_response_get_property (GObject     *object,
                            guint        property_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
    Tpm2Response *self = TPM2_RESPONSE (object);

    switch (property_id) {
    case PROP_ATTRIBUTES:
        g_value_set_uint (value, self->attributes);
        break;
    case PROP_BUFFER:
        g_value_set_pointer (value, self->buffer);
        break;
    case PROP_BUFFER_SIZE:
        g_value_set_uint (value, self->buffer_size);
        break;
    case PROP_SESSION:
        g_value_set_object (value, self->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tpm2_response_dispose (GObject *obj)
{
    Tpm2Response *self = TPM2_RESPONSE (obj);

    g_clear_object (&self->connection);
    G_OBJECT_CLASS (tpm2_response_parent_class)->dispose (obj);
}
/**
 * override the parent finalize method so we can free the data associated with
 * the DataMessage instance.
 */
static void
tpm2_response_finalize (GObject *obj)
{
    Tpm2Response *self = TPM2_RESPONSE (obj);

    g_debug ("tpm2_response_finalize");
    g_clear_pointer (&self->buffer, g_free);
    G_OBJECT_CLASS (tpm2_response_parent_class)->finalize (obj);
}
static void
tpm2_response_init (Tpm2Response *response)
{
    UNUSED_PARAM(response);
    /* noop */
}
/**
 * Boilerplate GObject initialization. Get a pointer to the parent class,
 * setup a finalize function.
 */
static void
tpm2_response_class_init (Tpm2ResponseClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tpm2_response_parent_class == NULL)
        tpm2_response_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose      = tpm2_response_dispose;
    object_class->finalize     = tpm2_response_finalize;
    object_class->get_property = tpm2_response_get_property;
    object_class->set_property = tpm2_response_set_property;

    obj_properties [PROP_ATTRIBUTES] =
        g_param_spec_uint ("attributes",
                           "TPMA_CC",
                           "Attributes for command.",
                           0,
                           UINT32_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_BUFFER] =
        g_param_spec_pointer ("buffer",
                              "TPM2 response buffer",
                              "memory buffer holding a TPM2 response",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_BUFFER_SIZE] =
        g_param_spec_uint ("buffer-size",
                           "sizeof command buffer",
                           "size of buffer holding the TPM2 command",
                           0,
                           UTIL_BUF_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SESSION] =
        g_param_spec_object ("connection",
                             "Connection object",
                             "The Connection object that sent the response",
                             TYPE_CONNECTION,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate constructor, but some GObject properties would be nice.
 */
Tpm2Response*
tpm2_response_new (Connection     *connection,
                   guint8          *buffer,
                   size_t           buffer_size,
                   TPMA_CC          attributes)
{
    return TPM2_RESPONSE (g_object_new (TYPE_TPM2_RESPONSE,
                                        "attributes", attributes,
                                       "buffer",  buffer,
                                       "buffer-size", buffer_size,
                                       "connection", connection,
                                       NULL));
}
/**
 * This is a convenience wrapper that is used to create an error response
 * where all we need is a header with the RC set to something specific.
 */
Tpm2Response*
tpm2_response_new_rc (Connection *connection,
                      TSS2_RC       rc)
{
    guint8 *buffer;

    buffer = calloc (1, TPM_RESPONSE_HEADER_SIZE);
    if (buffer == NULL) {
        g_warning ("tpm2_response_new_rc: failed to allocate 0x%zx"
                   " bytes for response: errno: %d: %s",
                   TPM_RESPONSE_HEADER_SIZE, errno, strerror (errno));
        return NULL;
    }
    TPM_RESPONSE_TAG (buffer)  = htobe16 (TPM2_ST_NO_SESSIONS);
    TPM_RESPONSE_SIZE (buffer) = htobe32 (TPM_RESPONSE_HEADER_SIZE);
    TPM_RESPONSE_CODE (buffer) = htobe32 (rc);
    return tpm2_response_new (connection, buffer, be32toh (TPM_RESPONSE_SIZE (buffer)), (TPMA_CC){ 0 });
}
/*
 * Create a new Tpm2Response object with a message body / buffer formatted
 * for the response to the TPM2_ContextLoad command. This command has no
 * session and the body of the response is set to the parameter TPM2_HANDLE.
 */
Tpm2Response*
tpm2_response_new_context_load (Connection *connection,
                                SessionEntry *entry)
{
    Tpm2Response *response = NULL;
    size_t offset = TPM_HEADER_SIZE;
    /* allocate buffer be large enough to hold TPM2_ContextSave response */
    uint8_t *buf = g_malloc0 (TPM_HEADER_SIZE + sizeof (TPM2_HANDLE));
    TSS2_RC rc;

    rc = Tss2_MU_TPM2_HANDLE_Marshal (session_entry_get_handle (entry),
                                      buf,
                                      TPM_HEADER_SIZE + sizeof (TPM2_HANDLE),
                                      &offset);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: Failed to write TPMS_CONTEXT to response header: 0x%"
                   PRIx32, __func__, rc);
        goto out;
    }
    /* offset now has size of response */
    rc = tpm2_header_init (buf, offset, TPM2_ST_NO_SESSIONS, offset, TSS2_RC_SUCCESS);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: Failed to initialize header: 0x%" PRIx32,
                   __func__, rc);
        goto out;
    }
    response = tpm2_response_new (connection, buf, offset, 0x10000161);
out:
    if (response == NULL) {
        g_free (buf);
    }
    return response;
}
/*
 * Create a new Tpm2Response object with a message body / buffer formatted
 * for the response to the TPM2_ContextSave command. This command has no
 * session and the body of the response is set to the parameter TPMS_CONTEXT
 * structure. This Tpm2Response will be sent to the client so we ensure that
 * the 'context_client' field is sent.
 */
Tpm2Response*
tpm2_response_new_context_save (Connection *connection,
                                SessionEntry *entry)
{
    Tpm2Response *response = NULL;
    size_buf_t *size_buf;
    /* allocate buffer be large enough to hold TPM2_ContextSave response */
    uint8_t *buf;
    TSS2_RC rc;

    size_buf = session_entry_get_context_client (entry);
    buf = g_malloc0 (TPM_HEADER_SIZE + size_buf->size);
    memcpy (&buf[TPM_HEADER_SIZE], size_buf->buf, size_buf->size);
    /* offset now has size of response */
    rc = tpm2_header_init (buf,
                           TPM_HEADER_SIZE + size_buf->size,
                           TPM2_ST_NO_SESSIONS,
                           TPM_HEADER_SIZE + size_buf->size,
                           TSS2_RC_SUCCESS);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: Failed to initialize header: 0x%" PRIx32,
                   __func__, rc);
        goto out;
    }
    response = tpm2_response_new (connection, buf, TPM_HEADER_SIZE + size_buf->size, 0x02000162);
out:
    if (response == NULL) {
        g_free (buf);
    }
    return response;
}
/* Simple "getter" to expose the attributes associated with the command. */
TPMA_CC
tpm2_response_get_attributes (Tpm2Response *response)
{
    return response->attributes;
}
/**
 */
guint8*
tpm2_response_get_buffer (Tpm2Response *response)
{
    return response->buffer;
}
/**
 */
TSS2_RC
tpm2_response_get_code (Tpm2Response *response)
{
    return be32toh (TPM_RESPONSE_CODE (response->buffer));
}
/**
 */
guint32
tpm2_response_get_size (Tpm2Response *response)
{
    return be32toh (TPM_RESPONSE_SIZE (response->buffer));
}
/**
 */
TPM2_ST
tpm2_response_get_tag (Tpm2Response *response)
{
    return be16toh (TPM_RESPONSE_TAG (response->buffer));
}
/*
 * Return the Connection object associated with this Tpm2Response. This
 * is the Connection object representing the client that should receive
 * this response. The reference count on this object is incremented before
 * the object is returned to the caller. The caller is responsible for
 * decrementing the reference count when it is done using the connection
 * object.
 */
Connection*
tpm2_response_get_connection (Tpm2Response *response)
{
    if (response->connection) {
        g_object_ref (response->connection);
    }
    return response->connection;
}
/*
 * Return the number of handles in the response. For a response to contain
 * a handle it must:
 * 1) the size of the response must be larger than the headder size
 * 2) the response code must indicate SUCCESS
 * 3) the associated attributes must have the TPMA_CC_RHANDLE bit set
 * If all of these conditions are met then the response has a handle in it.
 * Otherwise it does now.
 */
gboolean
tpm2_response_has_handle (Tpm2Response  *response)
{
    g_debug ("%s", __func__);
    if (tpm2_response_get_size (response) > TPM_HEADER_SIZE &&
        tpm2_response_get_code (response) == TSS2_RC_SUCCESS &&
        tpm2_response_get_attributes (response) & TPMA_CC_RHANDLE)
    {
        return TRUE;
    }
    return FALSE;
}
/*
 * Return the handle from the response handle area. Always check to be sure
 * the response has a handle in it before calling this function. If the
 * Tpm2Response has no handle in the handle area the return value from this
 * function will be indeterminate.
 */
TPM2_HANDLE
tpm2_response_get_handle (Tpm2Response *response)
{
    if (response == NULL) {
        g_error ("%s passed NULL parameter", __func__);
    }
    if (HANDLE_END_OFFSET > response->buffer_size) {
        g_warning ("%s: insufficient buffer to get handle", __func__);
        return 0;
    }
    return be32toh (HANDLE_GET (response->buffer));
}
/*
 */
void
tpm2_response_set_handle (Tpm2Response *response,
                          TPM2_HANDLE    handle)
{
    if (response == NULL) {
        g_error ("%s passed NULL parameter", __func__);
    }
    if (HANDLE_END_OFFSET > response->buffer_size) {
        g_warning ("%s: insufficient buffer to set handle", __func__);
        return;
    }
    HANDLE_GET (response->buffer) = htobe32 (handle);
}
/*
 * Return the type of the handle from the Tpm2Response object.
 */
TPM2_HT
tpm2_response_get_handle_type (Tpm2Response *response)
{
    /*
     * Explicit cast required to keep compiler type checking happy. The
     * shift operation preserves the 8bits we want to return but compiler
     * must consider the result a TPM2_HANDLE (32bits).
     */
    return (TPM2_HT)(tpm2_response_get_handle (response) >> TPM2_HR_SHIFT);
}
