#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "handle-map.h"

/* Boiler-plate gobject code. */
static gpointer handle_map_parent_class = NULL;
/*
 * Initialize object.
 */
static void
handle_map_init (GTypeInstance *instance,
                 gpointer       klass)
{
    HandleMap *map = HANDLE_MAP (instance);

    g_debug ("handle_map_init");
    pthread_mutex_init (&map->mutex, NULL);
    map->handle_to_object_hash_table =
        g_hash_table_new_full (g_direct_hash,
                               g_direct_equal,
                               NULL,
                               (GDestroyNotify)g_object_unref);
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
handle_map_new (void)
{
    return HANDLE_MAP (g_object_new (TYPE_HANDLE_MAP,
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
