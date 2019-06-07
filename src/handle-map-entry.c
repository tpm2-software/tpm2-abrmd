/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <inttypes.h>

#include "util.h"
#include "handle-map-entry.h"

G_DEFINE_TYPE (HandleMapEntry, handle_map_entry, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_PHANDLE,
    PROP_VHANDLE,
    PROP_CONTEXT,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/*
 * GObject property getter.
 */
static void
handle_map_entry_get_property (GObject        *object,
                               guint           property_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
    HandleMapEntry *self = HANDLE_MAP_ENTRY (object);

    switch (property_id) {
    case PROP_PHANDLE:
        g_value_set_uint (value, (guint)self->phandle);
        break;
    case PROP_VHANDLE:
        g_value_set_uint (value, (guint)self->vhandle);
        break;
    case PROP_CONTEXT:
        g_value_set_pointer (value, &self->context);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}
/*
 * GObject property setter.
 */
static void
handle_map_entry_set_property (GObject        *object,
                               guint           property_id,
                               GValue const   *value,
                               GParamSpec     *pspec)
{
    HandleMapEntry *self = HANDLE_MAP_ENTRY (object);

    switch (property_id) {
    case PROP_PHANDLE:
        self->phandle = (TPM2_HANDLE)g_value_get_uint (value);
        break;
    case PROP_VHANDLE:
        self->vhandle = (TPM2_HANDLE)g_value_get_uint (value);
        break;
    case PROP_CONTEXT:
        g_error ("Cannot set context property.");
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}
/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
handle_map_entry_init (HandleMapEntry *entry)
{
    UNUSED_PARAM(entry);
    /* noop */
}
/*
 * Deallocate all associated resources. All are static so we just chain
 * up to the parent like a good GObject.
 */
static void
handle_map_entry_finalize (GObject *object)
{
    g_debug ("%s", __func__);
    G_OBJECT_CLASS (handle_map_entry_parent_class)->finalize (object);
}
/*
 * Class initialization function. Register function pointers and properties.
 */
static void
handle_map_entry_class_init (HandleMapEntryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (handle_map_entry_parent_class == NULL)
        handle_map_entry_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = handle_map_entry_finalize;
    object_class->get_property = handle_map_entry_get_property;
    object_class->set_property = handle_map_entry_set_property;

    obj_properties [PROP_PHANDLE] =
        g_param_spec_uint ("phandle",
                           "Physical handle",
                           "Handle from TPM.",
                           0,
                           UINT32_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_VHANDLE] =
        g_param_spec_uint ("vhandle",
                           "Virtual handle",
                           "Handle exposed to client.",
                           0,
                           UINT32_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_CONTEXT] =
        g_param_spec_pointer ("context",
                              "TPMS_CONTEXT",
                              "Context blob from TPM.",
                              G_PARAM_READABLE);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * Instance constructor.
 */
HandleMapEntry*
handle_map_entry_new (TPM2_HANDLE phandle,
                      TPM2_HANDLE vhandle)
{
    HandleMapEntry *entry;

    entry = HANDLE_MAP_ENTRY (g_object_new (TYPE_HANDLE_MAP_ENTRY,
                                            "phandle", (guint)phandle,
                                            "vhandle", (guint)vhandle,
                                            NULL));
    g_debug ("%s: with vhandle: 0x%" PRIx32 " and phandle: 0x%" PRIx32,
             __func__, vhandle, phandle);
    return entry;
}
/*
 * Access the TPMS_CONTEXT member.
 * NOTE: This directly exposes memory from an object instance. The caller
 * must be sure to hold a reference to this object to keep it from being
 * garbage collected while the caller is accessing the context structure.
 * Further this object provides no thread safety ... yet.
 */
TPMS_CONTEXT*
handle_map_entry_get_context (HandleMapEntry *entry)
{
    return &entry->context;
}
/*
 * Accessor for the physical handle member.
 */
TPM2_HANDLE
handle_map_entry_get_phandle (HandleMapEntry *entry)
{
    return entry->phandle;
}
/*
 * Accessor for the virtual handle member.
 */
TPM2_HANDLE
handle_map_entry_get_vhandle (HandleMapEntry *entry)
{
    return entry->vhandle;
}
void
handle_map_entry_set_phandle (HandleMapEntry *entry,
                              TPM2_HANDLE      phandle)
{
    entry->phandle = phandle;
}
