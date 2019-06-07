/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "util.h"
#include "session-list.h"

G_DEFINE_TYPE (SessionList, session_list, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_MAX_ABANDONED,
    PROP_MAX_PER_CONNECTION,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/*
 * Property getter.
 */
static void
session_list_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    SessionList *list = SESSION_LIST (object);

    switch (property_id) {
    case PROP_MAX_ABANDONED:
        g_value_set_uint (value, list->max_abandoned);
        break;
    case PROP_MAX_PER_CONNECTION:
        g_value_set_uint (value, list->max_per_connection);
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
session_list_set_property (GObject        *object,
                         guint           property_id,
                         GValue const   *value,
                         GParamSpec     *pspec)
{
    SessionList *list = SESSION_LIST (object);

    switch (property_id) {
    case PROP_MAX_ABANDONED:
        list->max_abandoned = g_value_get_uint (value);
        break;
   case PROP_MAX_PER_CONNECTION:
        list->max_per_connection = g_value_get_uint (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/*
 * Initialize object.
 * GQueue for 'abandoned_queue' must be explicitly created.
 * GList for 'session_entry_list' does not.
 */
static void
session_list_init (SessionList     *list)
{
    g_debug ("session_list_init");
    list->abandoned_queue = g_queue_new ();
    list->session_entry_list = NULL;
}
/*
 * GObject dispose function: unref all SessionEntry objects in the internal
 * GSList and unref the list itself. NULL the pointer to the internal list
 * as well.
 */
static void
session_list_dispose (GObject *object)
{
    SessionList *self = SESSION_LIST (object);

    g_debug ("%s: SessionList with %" PRIu32 " entries", __func__,
             g_list_length (self->session_entry_list));
    g_queue_free (self->abandoned_queue);
    self->abandoned_queue = NULL;
    g_list_free_full (self->session_entry_list, g_object_unref);
    self->session_entry_list = NULL;
    G_OBJECT_CLASS (session_list_parent_class)->dispose (object);
}
/*
 * Deallocate all associated resources.
 */
static void
session_list_finalize (GObject *object)
{
    SessionList *self = SESSION_LIST (object);

    g_debug ("%s: SessionList with %" PRIu32 " entries", __func__,
             g_list_length (self->session_entry_list));
    g_list_free_full (self->session_entry_list, g_object_unref);
    G_OBJECT_CLASS (session_list_parent_class)->finalize (object);
}
/*
 * boiler-plate GObject class init function. Registers function pointers
 * and properties.
 */
static void
session_list_class_init (SessionListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (session_list_parent_class == NULL)
        session_list_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose      = session_list_dispose;
    object_class->finalize     = session_list_finalize;
    object_class->get_property = session_list_get_property;
    object_class->set_property = session_list_set_property;

    obj_properties [PROP_MAX_ABANDONED] =
        g_param_spec_uint ("max-abandoned",
                           "max abandoned sessions",
                           "maximum number of entries permitted in the abandoned state",
                           0,
                           SESSION_LIST_MAX_ABANDONED_MAX,
                           SESSION_LIST_MAX_ABANDONED_DEFAULT,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_MAX_PER_CONNECTION] =
        g_param_spec_uint ("max-per-connection",
                           "max entries per connection",
                           "maximum number of entries permitted for each connection",
                           0,
                           MAX_ENTRIES_MAX,
                           MAX_ENTRIES_DEFAULT,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * Instance initialization function. Create the resources the SessionList needs
 * and create the instance.
 */
SessionList*
session_list_new (guint  max_per_conn,
                  guint  max_abandoned)
{
    g_debug ("session_list_new with max-per-connection: 0x%x", max_per_conn);
    return SESSION_LIST (g_object_new (TYPE_SESSION_LIST,
                                       "max-abandoned", max_abandoned,
                                       "max-per-connection", max_per_conn,
                                       NULL));
}
/*
 * Insert GObject into the session list. We take a reference to the object
 * before we insert the object. When it is removed or if the SessionList
 * object is destroyed the object will be unref'd.
 */
gboolean
session_list_insert (SessionList      *list,
                     SessionEntry     *entry)
{
    if (list == NULL || entry == NULL) {
        g_error ("session_list_insert passed NULL parameter");
    }
    if (session_list_is_full (list, entry->connection)) {
        g_warning ("%s: max_per_connection of %u exceeded", __func__,
                    list->max_per_connection);
        return FALSE;
    }
    g_object_ref (entry);
    list->session_entry_list = g_list_append (list->session_entry_list,
                                                 entry);

    return TRUE;
}
static gboolean
session_list_remove_custom (SessionList  *list,
                            gconstpointer data,
                            GCompareFunc  func)
{
    GList       *list_entry;
    SessionEntry *entry_data;

    list_entry = g_list_find_custom (list->session_entry_list, data, func);
    if (list_entry == NULL) {
        return FALSE;
    }
    entry_data = SESSION_ENTRY (list_entry->data);
    list->session_entry_list = g_list_remove_link (list->session_entry_list,
                                                     list_entry);
    g_object_unref (entry_data);

    return TRUE;
}
/*
 * Remove the entry from the GList. The SessionList assumes that since the
 * entry is in the container it must hold a reference to the object and so
 * upon successful removal 
 * Returns TRUE on success, FALSE on failure.
 */
gboolean
session_list_remove_handle (SessionList      *list,
                            TPM2_HANDLE        handle)
{
    return session_list_remove_custom (list,
                                       &handle,
                                       session_entry_compare_on_handle);
}
/*
 * Returns TRUE on success, FALSE on failure.
 */
gboolean
session_list_remove_connection (SessionList      *list,
                                Connection       *connection)
{
    return session_list_remove_custom (list,
                                       connection,
                                       session_entry_compare_on_connection);
}
/*
 * This function is a thin wrapper around the g_list_remove function. Pass it
 * a SessionEntry. It will find it in the list and remove the associated entry
 * and then unref it (to account for the SessionList no longer holding a
 * reference).
 */
void
session_list_remove (SessionList   *list,
                     SessionEntry  *entry)
{
    g_debug ("%s", __func__);
    list->session_entry_list = g_list_remove (list->session_entry_list, entry);
    g_object_unref (entry);
}
/*
 * Get last entry in list and remove it from the list.
 */
SessionEntry*
session_list_remove_last (SessionList *list)
{
    GList       *list_entry;
    SessionEntry *entry_data;

    list_entry = g_list_last (list->session_entry_list);
    if (list_entry == NULL) {
        return NULL;
    }
    entry_data = SESSION_ENTRY (list_entry->data);
    list->session_entry_list = g_list_remove_link (list->session_entry_list,
                                                     list_entry);

    return entry_data;
}
/*
 * This is a lookup function to find an entry in the SessionList given
 * handle. This function increases the reference count on the
 * SessionEntry returned. The caller must decrement the reference count
 * when it is done with the entry.
 */
SessionEntry*
session_list_lookup_handle (SessionList   *list,
                            TPM2_HANDLE     handle)
{
    GList *list_entry;

    list_entry = g_list_find_custom (list->session_entry_list,
                                      &handle,
                                      session_entry_compare_on_handle);
    if (list_entry != NULL) {
        g_object_ref (list_entry->data);
        return SESSION_ENTRY (list_entry->data);
    } else {
        return NULL;
    }
}
/*
 * This type is used as a container for passing a buffer and its size to the
 * 'session_list_compare_context' callback. This callback must have the
 * prototype defined as 'GCompareFunc' which forces us to pass only one
 * arbitrary parameter.
 */
typedef struct size_buf_ptr {
    uint8_t *buf;
    size_t size;
} size_buf_ptr_t;
static gint
session_list_compare_context (gconstpointer a,
                              gconstpointer b)
{
    SessionEntry *entry = SESSION_ENTRY (a);
    size_buf_ptr_t *size_buf_ptr = (size_buf_ptr_t*)b;

    return session_entry_compare_on_context_client (entry,
                                                    size_buf_ptr->buf,
                                                    size_buf_ptr->size);
}
SessionEntry*
session_list_lookup_context_client (SessionList *list,
                                    uint8_t *buf,
                                    size_t size)
{
    GList *list_entry;
    size_buf_ptr_t size_buf_ptr = {
        .size = size,
        .buf = buf,
    };

    list_entry = g_list_find_custom (list->session_entry_list,
                                     &size_buf_ptr,
                                     session_list_compare_context);
    if (list_entry != NULL) {
        g_object_ref (list_entry->data);
        return SESSION_ENTRY (list_entry->data);
    } else {
        return NULL;
    }
}
/*
 * Simple wrapper around the function that reports the number of entries in
 * the hash table.
 */
guint
session_list_size (SessionList *list)
{
    guint ret;

    ret = g_list_length (list->session_entry_list);

    return ret;
}
/*
 * Structure used to hold data needed to count SessionEntry objects associated
 * with a given connection.
 */
typedef struct {
    Connection *connection;
    size_t      count;
} connection_count_data_t;
/*
 * Callback function used to count the number of SessionEntry objects in the
 * list associated with a given connection.
 */
static void
session_list_connection_counter (gpointer data,
                                 gpointer user_data)
{
    SessionEntry *entry = SESSION_ENTRY (data);
    Connection *conn = session_entry_get_connection (entry);
    connection_count_data_t *count_data = (connection_count_data_t*)user_data;

    if (count_data->connection == conn) {
        ++count_data->count;
    }
    g_clear_object (&conn);
}
/*
 * Returns the number of entries associated with the provided connection.
 */
size_t
session_list_connection_count (SessionList *list,
                               Connection  *connection)
{
    connection_count_data_t count_data = {
        .connection = connection,
        .count = 0,
    };

    session_list_foreach (list, session_list_connection_counter, &count_data);
    return count_data.count;
}
/*
 * Return false if the number of entries in the list is greater than or equal
 * to max_per_connection.
 */
gboolean
session_list_is_full (SessionList *session_list,
                      Connection  *connection)
{
    size_t session_count;
    gboolean ret;

    session_count = session_list_connection_count (session_list,
                                                   connection);
    if (session_count >= session_list->max_per_connection) {
        g_info ("%s: Connection has exceeded session limit", __func__);
        ret = TRUE;
    } else {
        ret= FALSE;
    }
    return ret;
}
/*
 */
void
session_list_foreach (SessionList *list,
                      GFunc        func,
                      gpointer     user_data)
{
    g_list_foreach (list->session_entry_list,
                     func,
                     user_data);
}
/*
 * Find the associated SessionEntry in the list.
 * Check that the SessionEntry has the same
 */
gboolean
session_list_abandon_handle (SessionList *list,
                             Connection *connection,
                             TPM2_HANDLE handle)
{
    SessionEntry *entry = NULL;

    entry = session_list_lookup_handle (list, handle);
    if (entry == NULL) {
        g_debug ("%s: Handle 0x%08" PRIx32 " doesn't exist in SessionList",
                 __func__, handle);
        return FALSE;
    }
    if (session_entry_compare_on_connection (entry, connection)) {
        g_warning ("%s: Connection attempted to abandon SessionEntry with "
                   "handle 0x%08" PRIx32, __func__, handle);
        g_clear_object (&entry);
        return FALSE;
    }
    session_entry_abandon (entry);
    g_queue_push_head (list->abandoned_queue, entry);
    g_clear_object (&entry);

    return TRUE;
}
/*
 * SessionEntry objects can be claimed when they're in two states:
 * - If the SessionEntry has been abandoned by another connection it will be
 *   in the 'abandoned_queue'. In this case we remove it from the
 *   'abandoned_queue', set the state to 'LOADED' and associate the provided
 *   connection with the object.
 * - If the SessionEntry has been saved BY THE CLIENT then it will *not* be
 *   in the 'abandoned_queue'. In this case we find the SessionEntry in the
 *   'session_entry_list' and change the connection.
 */
gboolean
session_list_claim (SessionList *list,
                    SessionEntry *entry,
                    Connection  *connection)
{
    GList *link = NULL;

    link = g_queue_find (list->abandoned_queue, entry);
    if (link != NULL) {
        g_assert (link->data == entry);
        g_debug ("%s: GQueue of abandoned sessions does not contain "
                 "SessionEntry", __func__);
        session_entry_set_state (entry, SESSION_ENTRY_LOADED);
        session_entry_set_connection (entry, connection);
        g_queue_remove (list->abandoned_queue, link->data);
        return TRUE;
    }
    link = g_list_find (list->session_entry_list, entry);
    if (link != NULL) {
        g_assert (link->data == entry);
        g_debug ("%s: SessionEntry found in SessionList", __func__);
        session_entry_set_state (entry, SESSION_ENTRY_LOADED);
        session_entry_set_connection (entry, connection);
    } else {
        return FALSE;
    }
    return TRUE;
}
/*
 * Remove oldest from abandoned queue and call the caller provided function
 * on it.
 */
gboolean
session_list_prune_abandoned (SessionList *list,
                              PruneFunc func,
                              gpointer data)
{
    SessionEntry *entry = NULL;
    gboolean ret = FALSE;

    if (g_queue_get_length (list->abandoned_queue) <= list->max_abandoned) {
        g_debug ("%s: abandoned_queue has not exceeded 'max_abandoned', "
                 "nothing to do.", __func__);
        return TRUE;
    }
    entry = g_queue_pop_tail (list->abandoned_queue);
    if (entry == NULL) {
        g_debug ("%s: Abandoned queue is empty.", __func__);
        return TRUE;
    }
    g_object_ref (entry);
    ret = func (entry, data);
    g_clear_object (&entry);
    return ret;
}
