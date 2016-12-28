#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "handle-map.h"

/* Boiler-plate gobject code. */
static gpointer handle_map_parent_class = NULL;

enum {
    PROP_0,
    PROP_HASH_TABLE,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/*
 * GObject property getter.
 */
static void
handle_map_get_property (GObject        *object,
                         guint           property_id,
                         GValue         *value,
                         GParamSpec     *pspec)
{
    HandleMap *self = HANDLE_MAP (object);

    switch (property_id) {
    case PROP_HASH_TABLE:
        g_value_set_pointer (value, self->handle_to_object_hash_table);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}
/*
 * GObject property setter.
 */
static void
handle_map_set_property (GObject        *object,
                               guint           property_id,
                               GValue const   *value,
                               GParamSpec     *pspec)
{
    HandleMap *self = HANDLE_MAP (object);

    switch (property_id) {
    case PROP_HASH_TABLE:
        self->handle_to_object_hash_table = g_value_get_pointer (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
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
    gint ret;

    pthread_mutex_destroy (&self->mutex);
    g_hash_table_unref (self->handle_to_object_hash_table);
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

    obj_properties [PROP_HASH_TABLE] =
        g_param_spec_pointer ("hash-table",
                              "GHashTable",
                              "Hash table mapping TPM_HANDLE to some object",
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
                                              NULL,
                                              0);

    return type;
}
/*
 * Instance initialization function. Create the resources the HandleMap needs
 * and create the instance.
 */
HandleMap*
handle_map_new (void)
{
    GHashTable *hash_table;
    HandleMap  *handle_map;

    hash_table = g_hash_table_new_full (g_direct_hash,
                                        g_direct_equal,
                                        NULL,
                                        (GDestroyNotify)g_object_unref);
    handle_map = HANDLE_MAP (g_object_new (TYPE_HANDLE_MAP,
                                           "hash-table", hash_table,
                                           NULL));
    if (pthread_mutex_init (&handle_map->mutex, NULL) != 0)
        g_error ("Failed to initialize HandleMap mutex: %s",
                 strerror (errno));

    return handle_map;
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
handle_map_insert (HandleMap   *map,
                   TPM_HANDLE   handle,
                   GObject     *object)
{
    handle_map_lock (map);
    g_object_ref (object);
    g_hash_table_insert (map->handle_to_object_hash_table, GINT_TO_POINTER (handle), object);
    handle_map_unlock (map);
}
/*
 * Remove the entry from the hash table associated with the provided handle.
 * Returns TRUE on success, FALSE on failure.
 */
gboolean
handle_map_remove (HandleMap *map,
                   TPM_HANDLE handle)
{
    gboolean ret;

    handle_map_lock (map);
    ret = g_hash_table_remove (map->handle_to_object_hash_table,
                               GINT_TO_POINTER (handle));
    handle_map_unlock (map);

    return ret;
}
/*
 * Look up the GObject associated with the handle in the hash table. The
 * object is not removed from the hash table. The reference count for the
 * object is incremented before it is returned to the caller. The caller
 * must free this reference when they are done with it.
 */
GObject*
handle_map_lookup (HandleMap *map,
                   TPM_HANDLE handle)
{
    GObject *object;

    handle_map_lock (map);
    object = g_hash_table_lookup (map->handle_to_object_hash_table,
                                  GINT_TO_POINTER (handle));
    g_object_ref (object);
    handle_map_unlock (map);

    return object;
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
    ret = g_hash_table_size (map->handle_to_object_hash_table);
    handle_map_unlock (map);

    return ret;
}
