/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "handle-map.h"
#include "util.h"

G_DEFINE_TYPE (HandleMap, handle_map, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_HANDLE_TYPE,
    PROP_MAX_ENTRIES,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/*
 * Property getter.
 */
static void
handle_map_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    HandleMap *map = HANDLE_MAP (object);

    switch (property_id) {
    case PROP_HANDLE_TYPE:
        g_value_set_uint (value, map->handle_count);
        break;
    case PROP_MAX_ENTRIES:
        g_value_set_uint (value, map->max_entries);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/*
 * Property setter.
 */
static void
handle_map_set_property (GObject        *object,
                         guint           property_id,
                         GValue const   *value,
                         GParamSpec     *pspec)
{
    HandleMap *map = HANDLE_MAP (object);

    switch (property_id) {
    case PROP_HANDLE_TYPE:
        map->handle_type = (TPM2_HT)g_value_get_uint (value);
        break;
    case PROP_MAX_ENTRIES:
        map->max_entries = g_value_get_uint (value);
        g_debug ("%s: max-entries: %u", __func__, map->max_entries);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/*
 * Initialize object. This requires:
 * 1) initializing the mutex that mediate access to the hash tables
 * 2) creating the hash tables
 * 3) initializing the handle_count
 * The handle_count is currently initialized to start allocating handles
 * @ 0xff. This is an arbitrary way we differentiate them from the handles
 * allocated by the TPM.
 */
static void
handle_map_init (HandleMap     *map)
{
    g_debug ("handle_map_init");
    pthread_mutex_init (&map->mutex, NULL);
    map->vhandle_to_entry_table =
        g_hash_table_new_full (g_direct_hash,
                               g_direct_equal,
                               NULL,
                               (GDestroyNotify)g_object_unref);
    map->handle_count = 0xff;
}
/*
 * GObject dispose function: release all references to GObjects. Currently
 * this is only the GHashTable mapping vhandles to HandleMapEntry objects.
 */
static void
handle_map_dispose (GObject *object)
{
    HandleMap *self = HANDLE_MAP (object);

    g_clear_pointer (&self->vhandle_to_entry_table, g_hash_table_unref);
    G_OBJECT_CLASS (handle_map_parent_class)->dispose (object);
}
/*
 * GObject finalize function: release all non-GObject resources. Currently
 * this is the mutex used to lock the GHashTable.
 */
static void
handle_map_finalize (GObject *object)
{
    HandleMap *self = HANDLE_MAP (object);

    g_debug ("handle_map_finalize");
    pthread_mutex_destroy (&self->mutex);
    G_OBJECT_CLASS (handle_map_parent_class)->finalize (object);
}
/*
 * boiler-plate GObject class init function. Registers function pointers
 * and properties.
 */
static void
handle_map_class_init (HandleMapClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (handle_map_parent_class == NULL)
        handle_map_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose      = handle_map_dispose;
    object_class->finalize     = handle_map_finalize;
    object_class->get_property = handle_map_get_property;
    object_class->set_property = handle_map_set_property;

    obj_properties [PROP_HANDLE_TYPE] =
        g_param_spec_uint ("handle-type",
                           "type of handle",
                           "type of handle tracked in this map",
                           0x0,
                           0xff,
                           0x80,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_MAX_ENTRIES] =
        g_param_spec_uint ("max-entries",
                           "max number of entries",
                           "maximum number of entries permitted",
                           0,
                           MAX_ENTRIES_MAX,
                           MAX_ENTRIES_DEFAULT,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * Instance initialization function. Create the resources the HandleMap needs
 * and create the instance.
 */
HandleMap*
handle_map_new (TPM2_HT handle_type,
                guint  max_entries)
{
    g_debug ("handle_map_new with handle_type 0x%" PRIx32
             ", max_entries: 0x%x", handle_type, max_entries);
    return HANDLE_MAP (g_object_new (TYPE_HANDLE_MAP,
                                     "handle-type", handle_type,
                                     "max-entries", max_entries,
                                     NULL));
}
/*
 * Lock the mutex that protects the hash table object.
 */
static inline void
handle_map_lock (HandleMap *map)
{
    if (pthread_mutex_lock (&map->mutex) != 0)
        g_error ("Error locking HandleMap: %s", strerror (errno));
}
/*
 * Unlock the mutex that protects the hash table object.
 */
static inline void
handle_map_unlock (HandleMap *map)
{
    if (pthread_mutex_unlock (&map->mutex) != 0)
        g_error ("Error unlocking HandleMap: %s", strerror (errno));
}
/*
 * Return false if the number of entries in the map is greater than or equal
 * to max_entries.
 */
gboolean
handle_map_is_full (HandleMap *map)
{
    guint table_size;

    table_size = g_hash_table_size (map->vhandle_to_entry_table);
    if (table_size < map->max_entries + 1) {
        return FALSE;
    } else {
        return TRUE;
    }
}
/*
 * Insert GObject into the hash table with the key being the provided handle.
 * We take a reference to the object before we insert the object since when
 * it is removed or if the hash table is destroyed the object will be unref'd.
 * If a handle provided is 0 we do not insert the entry in the corresponding
 * map.
 * If there is an entry with the given key already in the table we don't insert
 * anything, because it would overwrite the original entry.
 */
gboolean
handle_map_insert (HandleMap      *map,
                   TPM2_HANDLE     vhandle,
                   HandleMapEntry *entry)
{
    g_debug ("%s: vhandle: 0x%" PRIx32, __func__, vhandle);
    handle_map_lock (map);
    if (handle_map_is_full (map)) {
        g_warning ("%s: max_entries of %u exceeded", __func__, map->max_entries);
        handle_map_unlock (map);
        return FALSE;
    }
    if (entry && vhandle != 0) {
        TPM2_HANDLE *orig_key;
        HandleMapEntry *orig_value;

	/* Check if an entry for the key is already in already in the table */
        if (!g_hash_table_lookup_extended (map->vhandle_to_entry_table,
                                           GINT_TO_POINTER (vhandle),
                                           (gpointer *) &orig_key,
					   (gpointer *) &orig_value)) {
            g_object_ref (entry);
            g_hash_table_insert (map->vhandle_to_entry_table,
                                 GINT_TO_POINTER (vhandle),
                                 entry);
	}
    }
    handle_map_unlock (map);
    return TRUE;
}
/*
 * Remove the entry from the hash table associated with the provided handle.
 * Returns TRUE on success, FALSE on failure.
 */
gboolean
handle_map_remove (HandleMap *map,
                   TPM2_HANDLE vhandle)
{
    gboolean ret;

    handle_map_lock (map);
    ret = g_hash_table_remove (map->vhandle_to_entry_table,
                               GINT_TO_POINTER (vhandle));
    handle_map_unlock (map);

    return ret;
}
/*
 * Generic lookup function to find an entry in a GHashTable for the given
 * handle.
 */
static HandleMapEntry*
handle_map_lookup (HandleMap     *map,
                   TPM2_HANDLE     handle,
                   GHashTable    *table)
{
    HandleMapEntry *entry;

    handle_map_lock (map);
    entry = g_hash_table_lookup (table, GINT_TO_POINTER (handle));
    if (entry)
        g_object_ref (entry);
    handle_map_unlock (map);

    return entry;
}
/*
 * Look up the GObject associated with the virtual handle in the hash
 * table. The object is not removed from the hash table. The reference
 * count for the object is incremented before it is returned to the caller.
 * The caller must free this reference when they are done with it.
 * NULL is returned if no entry matches the provided handle.
 */
HandleMapEntry*
handle_map_vlookup (HandleMap    *map,
                    TPM2_HANDLE    vhandle)
{
    return handle_map_lookup (map, vhandle, map->vhandle_to_entry_table);
}
/*
 * Simple wrapper around the function that reports the number of entries in
 * the hash table.
 */
guint
handle_map_size (HandleMap *map)
{
    guint ret;

    handle_map_lock (map);
    ret = g_hash_table_size (map->vhandle_to_entry_table);
    handle_map_unlock (map);

    return ret;
}
/*
 * Combine the handle_type and the handle_count to create a new handle.
 * The handle_count is advanced as part of this. If the handle_count has
 * a bit set in the upper byte (the handle_type bits) then it has rolled
 * over and we return an error.
 * NOTE: recycling handles is something we should be able to do for long
 * lived programs / daemons that may want to use the TPM.
 */
TPM2_HANDLE
handle_map_next_vhandle (HandleMap *map)
{
    TPM2_HANDLE handle;

    /* (2 ^ 24) - 1 handles max, return 0 if we roll over*/
    if (map->handle_count & TPM2_HR_RANGE_MASK)
        return 0;
    handle = map->handle_count + (TPM2_HANDLE)(map->handle_type << TPM2_HR_SHIFT);
    ++map->handle_count;
    return handle;
}
void
handle_map_foreach (HandleMap *map,
                    GHFunc     callback,
                    gpointer   user_data)
{
    g_hash_table_foreach (map->vhandle_to_entry_table,
                          callback,
                          user_data);
}
/*
 * Get a GList containing all keys from the map. These will be returned in no
 * particular order so don't assume that it's sorted in any way.
 */
GList*
handle_map_get_keys (HandleMap *map)
{
    return g_hash_table_get_keys (map->vhandle_to_entry_table);
}
