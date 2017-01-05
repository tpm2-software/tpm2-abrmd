#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "handle-map.h"

/* Boiler-plate gobject code. */
static gpointer handle_map_parent_class = NULL;
enum {
    PROP_0,
    PROP_HANDLE_TYPE,
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
        map->handle_type = g_value_get_uint (value);
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
handle_map_init (GTypeInstance *instance,
                 gpointer       klass)
{
    HandleMap *map = HANDLE_MAP (instance);

    g_debug ("handle_map_init");
    pthread_mutex_init (&map->mutex, NULL);
    map->phandle_to_entry_table =
        g_hash_table_new_full (g_direct_hash,
                               g_direct_equal,
                               NULL,
                               (GDestroyNotify)g_object_unref);
    map->vhandle_to_entry_table =
        g_hash_table_new_full (g_direct_hash,
                               g_direct_equal,
                               NULL,
                               (GDestroyNotify)g_object_unref);
    map->handle_count = 0xff;
}
/*
 * Deallocate all associated resources. The mutex may be held by another
 * thread. If we attempt to lock / unlock it we may hang a thread in the
 * guts of the GObject system. Better to just destroy it and free all
 * other resources. The call to destroy may fail and if so we just carry
 * on.
 */
static void
handle_map_finalize (GObject *object)
{
    HandleMap *self = HANDLE_MAP (object);

    g_debug ("handle_map_finalize");
    pthread_mutex_destroy (&self->mutex);
    g_hash_table_unref (self->phandle_to_entry_table);
    g_hash_table_unref (self->vhandle_to_entry_table);
    G_OBJECT_CLASS (handle_map_parent_class)->finalize (object);
}
/*
 * boiler-plate GObject class init function. Registers function pointers
 * and properties.
 */
static void
handle_map_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (handle_map_parent_class == NULL)
        handle_map_parent_class = g_type_class_peek_parent (klass);
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
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * Boiler-plate GObject 'get_type' function.
 */
GType
handle_map_get_type (void)
{
    static GType type = 0;

    if (type == 0)
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "HandleMap",
                                              sizeof (HandleMapClass),
                                              (GClassInitFunc) handle_map_class_init,
                                              sizeof (HandleMap),
                                              handle_map_init,
                                              0);

    return type;
}
/*
 * Instance initialization function. Create the resources the HandleMap needs
 * and create the instance.
 */
HandleMap*
handle_map_new (TPM_HT handle_type)
{
    return HANDLE_MAP (g_object_new (TYPE_HANDLE_MAP,
                                     "handle-type", handle_type,
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
 * Insert GObject into the hash table with the key bing the provided handle.
 * We take a reference to the object before we insert the object since when
 * it is removed or if the hash table is destroyed the object will be unref'd.
 */
void
handle_map_insert (HandleMap      *map,
                   TPM_HANDLE      phandle,
                   TPM_HANDLE      vhandle,
                   HandleMapEntry *entry)
{
    handle_map_lock (map);
    g_object_ref (entry);
    g_hash_table_insert (map->phandle_to_entry_table, GINT_TO_POINTER (phandle), entry);
    g_object_ref (entry);
    g_hash_table_insert (map->vhandle_to_entry_table, GINT_TO_POINTER (vhandle), entry);
    handle_map_unlock (map);
}
/*
 * Remove the entry from the hash table associated with the provided handle.
 * Returns TRUE on success, FALSE on failure.
 */
gboolean
handle_map_remove (HandleMap *map,
                   TPM_HANDLE phandle)
{
    gboolean ret;
    HandleMapEntry *entry;
    TPM_HANDLE vhandle;

    entry = handle_map_plookup (map, phandle);
    vhandle = handle_map_entry_get_vhandle (entry);
    g_object_unref (entry);
    handle_map_lock (map);
    ret = g_hash_table_remove (map->phandle_to_entry_table,
                               GINT_TO_POINTER (phandle));
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
                   TPM_HANDLE     handle,
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
 * Look up the GObject associated with the handle in the hash table. The
 * object is not removed from the hash table. The reference count for the
 * object is incremented before it is returned to the caller. The caller
 * must free this reference when they are done with it.
 * NULL is returned if no entry matches the provided handle.
 */
HandleMapEntry*
handle_map_plookup (HandleMap    *map,
                    TPM_HANDLE    phandle)
{
    return handle_map_lookup (map, phandle, map->phandle_to_entry_table);
}
/*
 * Same thing for the vhandle.
 */
HandleMapEntry*
handle_map_vlookup (HandleMap    *map,
                    TPM_HANDLE    vhandle)
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
    ret = g_hash_table_size (map->phandle_to_entry_table);
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
TPM_HANDLE
handle_map_next_vhandle (HandleMap *map)
{
    TPM_HANDLE handle;

    /* (2 ^ 24) - 1 handles max, return 0 if we roll over*/
    if (map->handle_count & HR_RANGE_MASK)
        return 0;
    handle = map->handle_count + (map->handle_type << HR_SHIFT);
    ++map->handle_count;
    return handle;
}
