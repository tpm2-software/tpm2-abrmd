/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include <tss2/tss2_mu.h>

#include "tpm2-header.h"
#include "util.h"
#include "session-entry.h"

G_DEFINE_TYPE (SessionEntry, session_entry, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_CONNECTION,
    PROP_CONTEXT,
    PROP_HANDLE,
    PROP_STATE,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/*
 * GObject property getter.
 */
static void
session_entry_get_property (GObject        *object,
                            guint           property_id,
                            GValue         *value,
                            GParamSpec     *pspec)
{
    SessionEntry *self = SESSION_ENTRY (object);

    switch (property_id) {
    case PROP_CONNECTION:
        g_value_set_pointer (value, self->connection);
        break;
    case PROP_CONTEXT:
        g_value_set_pointer (value, &self->context);
        break;
    case PROP_HANDLE:
        g_value_set_uint (value, session_entry_get_handle (self));
        break;
    case PROP_STATE:
        g_value_set_enum (value, self->state);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}
/*
 * GObject property setter.
 */
static void
session_entry_set_property (GObject        *object,
                               guint           property_id,
                               GValue const   *value,
                               GParamSpec     *pspec)
{
    SessionEntry *self = SESSION_ENTRY (object);

    switch (property_id) {
    case PROP_CONNECTION:
        self->connection = CONNECTION (g_value_get_pointer (value));
        g_object_ref (self->connection);
        break;
    case PROP_CONTEXT:
        g_error ("Cannot set context property.");
        break;
    case PROP_HANDLE:
        self->handle = (TPM2_HANDLE)g_value_get_uint (value);
        break;
    case PROP_STATE:
        self->state = g_value_get_enum (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}
/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
session_entry_init (SessionEntry *entry)
{
    UNUSED_PARAM(entry);
    /* noop */
}
/*
 * Deallocate all associated resources. All are static so we just chain
 * up to the parent like a good GObject.
 */
static void
session_entry_dispose (GObject *object)
{
    SessionEntry *entry = SESSION_ENTRY (object);

    g_debug ("%s", __func__);
    g_clear_object (&entry->connection);
    G_OBJECT_CLASS (session_entry_parent_class)->dispose (object);
}
/*
 * Class initialization function. Register function pointers and properties.
 */
static void
session_entry_class_init (SessionEntryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (session_entry_parent_class == NULL)
        session_entry_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose = session_entry_dispose;
    object_class->get_property = session_entry_get_property;
    object_class->set_property = session_entry_set_property;

    obj_properties [PROP_CONNECTION] = 
        g_param_spec_pointer ("connection",
                              "Connection",
                              "Associated Connection.",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_CONTEXT] =
        g_param_spec_pointer ("context",
                              "TPMS_CONTEXT",
                              "Context blob from TPM.",
                              G_PARAM_READABLE);
    obj_properties [PROP_HANDLE] =
        g_param_spec_uint ("handle",
                           "TPM2_HANDLE",
                           "Handle from TPM.",
                           0,
                           UINT32_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_STATE] =
        g_param_spec_enum ("state",
                           "SessionEntryStateEnum",
                           "State of SessionEntry",
                           TYPE_SESSION_ENTRY_STATE_ENUM,
                           SESSION_ENTRY_SAVED_RM,
                           G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * Instance constructor.
 */
SessionEntry*
session_entry_new (Connection *connection,
                   TPM2_HANDLE  handle)
{
    g_debug ("%s", __func__);
    return SESSION_ENTRY (g_object_new (TYPE_SESSION_ENTRY,
                                        "connection", connection,
                                        "handle", handle,
                                        NULL));
}
/*
 * Access the 'context' member.
 * NOTE: This directly exposes memory from an object instance. The caller
 * must be sure to hold a reference to this object to keep it from being
 * garbage collected while the caller is accessing the context structure.
 * Further this object provides no thread safety ... yet.
 */
size_buf_t*
session_entry_get_context (SessionEntry *entry)
{
    return &entry->context;
}
size_buf_t*
session_entry_get_context_client (SessionEntry *entry)
{
    return &entry->context_client;
}
/*
 * Access the Connection associated with this SessionEntry. The reference
 * count for the Connection is incremented and must be decremented by the
 * caller.
 */
Connection*
session_entry_get_connection (SessionEntry *entry)
{
    if (entry->connection) {
        g_object_ref (entry->connection);
    }
    return entry->connection;
}
/*
 * Accessor for the savedHandle member of the associated context.
 */
TPM2_HANDLE
session_entry_get_handle (SessionEntry *entry)
{
    g_assert_nonnull (entry);
    return entry->handle;
}
/*
 * Simple accessor to the state of the SessionEntry.
 */
SessionEntryStateEnum
session_entry_get_state (SessionEntry *entry)
{
    return entry->state;
}
/*
 * This function allows the caller to set the state of the SessionEntry. It
 * also ensures that if the SessionEntry is put into the 'SAVED_CLIENT_CLOSED'
 * state that the connection field is clear / NULL.
 */
void
session_entry_set_state (SessionEntry *entry,
                         SessionEntryStateEnum state)
{
    assert (entry != NULL);
    if (state == SESSION_ENTRY_SAVED_CLIENT_CLOSED) {
        g_clear_object (&entry->connection);
    }
    entry->state = state;
}
/*
 * Set the contents of the 'context' blob. This blob holds the TPMS_CONTEXT
 * in its marshalled form (ready to be sent to the TPM in the body of a
 * ContextLoad command). We also copy this same blob to the 'context_client'
 * blob (the TPMS_CONTEXT that we expose to clients) if it has not yet been
 * initialized.
 */
void
session_entry_set_context (SessionEntry *entry,
                           uint8_t *buf,
                           size_t size)
{
    assert (entry != NULL && buf != NULL && size <= SIZE_BUF_MAX);

    memcpy (entry->context.buf, buf, size);
    entry->context.size = size;
    if (entry->context_client.size == 0) {
        memcpy (entry->context_client.buf, buf, size);
        entry->context_client.size = size;
    }
}
/*
 * When the connection is set the previous connection, if there was one, must
 * have its reference count decremented and the internal pointer NULLed.
 */
void
session_entry_set_connection (SessionEntry *entry,
                              Connection   *connection)
{
    g_object_ref (connection);
    g_clear_object (&entry->connection);
    entry->connection = connection;
}
/*
 * This function is used with the g_list_find_custom function to find
 * session_entry_t's in the list with a given handle. The first parameter
 * is a reference to an entry in the list. The second is a reference to the
 * handle we're looking for.
 */
gint
session_entry_compare (gconstpointer a,
                       gconstpointer b)
{
    if (a == NULL || b == NULL) {
        g_error ("session_entry_compare_on_handle received NULL parameter");
    }
    SessionEntry *entry_a = SESSION_ENTRY (a);
    SessionEntry *entry_b = SESSION_ENTRY (b);

    if (entry_a < entry_b) {
        return -1;
    } else if (entry_a > entry_b) {
        return 1;
    } else {
        return 0;
    }
}
/*
 * GCompareFunc to search list of session_entry_t structures for a match on
 * the connection object.
 */
gint
session_entry_compare_on_connection (gconstpointer a,
                                     gconstpointer b)
{
    gint ret;

    if (a == NULL || b == NULL) {
        g_error ("session_entry_compare_on_connection received NULL parameter");
    }
    SessionEntry    *entry_a          = SESSION_ENTRY (a);
    Connection      *connection_param = CONNECTION (b);
    Connection      *connection_entry = session_entry_get_connection (entry_a);
    if (connection_entry < connection_param) {
        ret = -1;
    } else if (connection_entry > connection_param) {
        ret = 1;
    } else {
        ret = 0;
    }
    g_object_unref (connection_entry);

    return ret;
}
/*
 * GCompareFunc used to compare SessionEntry objects on their handle members.
 * This was taken directly from the SessionEntry code. My abstractions are
 * leaking and this is a sign that the objects involved in session handling
 * need to be refactored.
 */
gint
session_entry_compare_on_handle (gconstpointer a,
                                 gconstpointer b)
{
    if (a == NULL || b == NULL) {
        g_error ("session_entry_compare_on_handle received NULL parameter");
    }
    SessionEntry *entry_a = SESSION_ENTRY (a);
    TPM2_HANDLE handle_a = session_entry_get_handle (entry_a);
    TPM2_HANDLE handle_b = *(TPM2_HANDLE*)b;

    if (handle_a < handle_b) {
        return -1;
    } else if (handle_a > handle_b) {
        return 1;
    } else {
        return 0;
    }
}
void
session_entry_abandon (SessionEntry *entry)
{
    g_clear_object (&entry->connection);
    entry->state = SESSION_ENTRY_SAVED_CLIENT_CLOSED;
}
/*
 * This function is used to compare the context_client field the TPMS_CONTEXT
 * provided by the caller. The result of this function is byte-for-byte
 * comparison between the two context structures. Both the context_client and
 * the provided context structure are marshalled to their network
 * representation before comparison to eliminate possible errors caused by
 * comparing structure padding.
 */
gint
session_entry_compare_on_context_client (SessionEntry *entry,
                                         uint8_t *buf,
                                         size_t size)
{
    size_buf_t *size_buf = NULL;

    g_assert (size <= SIZE_BUF_MAX);
    size_buf = session_entry_get_context_client (entry);
    return memcmp (size_buf->buf, buf, size);
}
