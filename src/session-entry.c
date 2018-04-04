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
#include <inttypes.h>

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
        g_value_set_uint (value, (guint)self->context.savedHandle);
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
        self->context.savedHandle = (TPM2_HANDLE)g_value_get_uint (value);
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

    g_debug ("%s: 0x%" PRIxPTR, __func__, (uintptr_t)entry);
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
    g_debug ("session_entry_new with connection: 0x%" PRIxPTR,
             (uintptr_t)connection);
    return SESSION_ENTRY (g_object_new (TYPE_SESSION_ENTRY,
                                        "connection", connection,
                                        "handle", handle,
                                        NULL));
}
/*
 * Access the TPMS_CONTEXT member.
 * NOTE: This directly exposes memory from an object instance. The caller
 * must be sure to hold a reference to this object to keep it from being
 * garbage collected while the caller is accessing the context structure.
 * Further this object provides no thread safety ... yet.
 */
TPMS_CONTEXT*
session_entry_get_context (SessionEntry *entry)
{
    return &entry->context;
}
/*
 * Access the Connection associated with this SessionEntry. The reference
 * count for the Connection is incremented and must be decremented by the
 * caller.
 */
Connection*
session_entry_get_connection (SessionEntry *entry)
{
    g_object_ref (entry->connection);
    return entry->connection;
}
/*
 * Accessor for the savedHandle member of the associated context.
 */
TPM2_HANDLE
session_entry_get_handle (SessionEntry *entry)
{
    return session_entry_get_context (entry)->savedHandle;
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
    if (state == SESSION_ENTRY_SAVED_CLIENT_CLOSED) {
        g_clear_object (&entry->connection);
    }
    entry->state = state;
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
void
session_entry_prettyprint (SessionEntry *entry)
{
    g_debug ("SessionEntry:    0x%"   PRIxPTR, (uintptr_t)entry);
    g_debug ("  Connection:    0x%"   PRIxPTR, (uintptr_t)entry->connection);
    g_debug ("  State:         %s",   session_entry_state_to_str (entry->state));
    g_debug ("  TPMS_CONTEXT:  0x%"   PRIxPTR, (uintptr_t)&entry->context);
    g_debug ("    sequence:    0x%"   PRIx64,  entry->context.sequence);
    g_debug ("    savedHandle: 0x%08" PRIx32,  entry->context.savedHandle);
    g_debug ("    hierarchy:   0x%08" PRIx32,  entry->context.hierarchy);
    g_debug ("    contextBlob: 0x%"   PRIxPTR,
             (uintptr_t)&entry->context.contextBlob);
}
