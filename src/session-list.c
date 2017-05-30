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
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "session-list.h"

G_DEFINE_TYPE (SessionList, session_list, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_MAX_ENTRIES,
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
    case PROP_MAX_ENTRIES:
        g_value_set_uint (value, list->max_entries);
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
    case PROP_MAX_ENTRIES:
        list->max_entries = g_value_get_uint (value);
        g_debug ("session_list_set_property: 0x%" PRIxPTR " max-entries: %u", (intptr_t)list, list->max_entries);
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
session_list_init (SessionList     *list)
{
    g_debug ("session_list_init");
    pthread_mutex_init (&list->mutex, NULL);
    list->session_entry_slist = NULL;
}
/*
 * Deallocate all associated resources. The mutex may be held by another
 * thread. If we attempt to lock / unlock it we may hang a thread in the
 * guts of the GObject system. Better to just destroy it and free all
 * other resources. The call to destroy may fail and if so we just carry
 * on.
 */
static void
session_list_finalize (GObject *object)
{
    SessionList *self = SESSION_LIST (object);

    g_debug ("session_list_finalize: SessionList: 0x%" PRIxPTR " with %"
             PRIu32 " entries", (uintptr_t)self,
             g_slist_length (self->session_entry_slist));
    pthread_mutex_destroy (&self->mutex);
    g_slist_free_full (self->session_entry_slist, g_object_unref);
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
    object_class->finalize     = session_list_finalize;
    object_class->get_property = session_list_get_property;
    object_class->set_property = session_list_set_property;

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
 * Instance initialization function. Create the resources the SessionList needs
 * and create the instance.
 */
SessionList*
session_list_new (guint  max_entries)
{
    g_debug ("session_list_new with max_entries: 0x%x", max_entries);
    return SESSION_LIST (g_object_new (TYPE_SESSION_LIST,
                                       "max-entries", max_entries,
                                       NULL));
}
/*
 * Lock the mutex that protects the hash table object.
 */
gint
session_list_lock (SessionList *list)
{
    return pthread_mutex_lock (&list->mutex);
}
/*
 * Unlock the mutex that protects the list and entry objects.
 */
gint
session_list_unlock (SessionList *list)
{
    return pthread_mutex_unlock (&list->mutex);
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
    g_debug ("session_list_insert: 0x%" PRIxPTR ", entry: 0x%" PRIxPTR,
             (uintptr_t)list, (uintptr_t)entry);
    if (list == NULL || entry == NULL) {
        g_error ("session_list_insert passed NULL parameter");
    }
    session_list_lock (list);
    if (session_list_is_full (list)) {
        g_warning ("SessionList: 0x%" PRIxPTR " max_entries of %u exceeded",
                   (uintptr_t)list, list->max_entries);
        session_list_unlock (list);
        return FALSE;
    }
    g_object_ref (entry);
    list->session_entry_slist = g_slist_prepend (list->session_entry_slist,
                                                 entry);
    session_list_unlock (list);

    return TRUE;
}
/*
 * This function is used with the g_slist_find_custom function to find
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
 * This function is used with the g_slist_find_custom function to find
 * session_entry_t's in the list with a given handle. The first parameter
 * is a reference to an entry in the list. The second is a reference to the
 * handle we're looking for.
 */
gint
session_entry_compare_on_handle (gconstpointer a,
                                 gconstpointer b)
{
    if (a == NULL || b == NULL) {
        g_error ("session_entry_compare_on_handle received NULL parameter");
    }
    SessionEntry *entry_a = SESSION_ENTRY (a);
    TPM_HANDLE handle_a = session_entry_get_handle (entry_a);
    TPM_HANDLE handle_b = *(TPM_HANDLE*)b;

    if (handle_a < handle_b) {
        return -1;
    } else if (handle_a > handle_b) {
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
static gboolean
session_list_remove_custom (SessionList  *list,
                            gconstpointer data,
                            GCompareFunc  func)
{
    GSList       *list_entry;
    SessionEntry *entry_data;

    session_list_lock (list);
    list_entry = g_slist_find_custom (list->session_entry_slist, data, func);
    if (list_entry == NULL) {
        session_list_unlock (list);
        return FALSE;
    }
    entry_data = SESSION_ENTRY (list_entry->data);
    list->session_entry_slist = g_slist_remove_link (list->session_entry_slist,
                                                     list_entry);
    g_object_unref (entry_data);
    session_list_unlock (list);

    return TRUE;
}
                          
/*
 * Remove the entry from the GSList. The SessionList assumes that since the
 * entry is in the container it must hold a reference to the object and so
 * upon successful removal 
 * Returns TRUE on success, FALSE on failure.
 */
gboolean
session_list_remove_handle (SessionList      *list,
                            TPM_HANDLE        handle)
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
 * This function is a thin wrapper around the g_slist_remove function. Pass it
 * a SessionEntry. It will find it in the list and remove the associated entry
 * and then unref it (to account for the SessionList no longer holding a
 * reference). This function does not lock the SessionList during this
 * operation so the caller will have to do this themselves.
 */
void
session_list_remove (SessionList   *list,
                     SessionEntry  *entry)
{
    g_debug ("session_list_remove: SessionList: 0x%" PRIxPTR " SessionEntry: "
             "0x%" PRIxPTR, (uintptr_t)list, (uintptr_t)entry);
    list->session_entry_slist = g_slist_remove (list->session_entry_slist, entry);
    g_object_unref (entry);
}
/*
 * This is a lookup function to find an entry in the SessionList given
 * handle. The caller *must* hold the lock on the SessionList. Further they
 * must hold this lock until they are done with the SessionEntry
 * returned. This function increases the reference count on the SessionEntry
 * returned. The caller must decrement the reference count when it is done
 * with the entry.
 */
SessionEntry*
session_list_lookup_connection (SessionList   *list,
                                Connection    *connection)
{
    GSList *list_entry;

    list_entry = g_slist_find_custom (list->session_entry_slist,
                                      connection,
                                      session_entry_compare_on_connection);
    if (list_entry != NULL) {
        g_object_ref (list_entry->data);
        return SESSION_ENTRY (list_entry->data);
    } else {
        return NULL;
    }
}
/*
 * This is a lookup function to find an entry in the SessionList given
 * handle. The caller *must* hold the lock on the SessionList. Further they
 * must hold this lock until they are done with the SessionListEntry
 * returned. This function increases the reference count on the
 * SessionEntry returned. The caller must decrement the reference count
 * when it is done with the entry.
 */
SessionEntry*
session_list_lookup_handle (SessionList   *list,
                            TPM_HANDLE     handle)
{
    GSList *list_entry;

    list_entry = g_slist_find_custom (list->session_entry_slist,
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
 * Simple wrapper around the function that reports the number of entries in
 * the hash table.
 */
guint
session_list_size (SessionList *list)
{
    guint ret;

    session_list_lock (list);
    ret = g_slist_length (list->session_entry_slist);
    session_list_unlock (list);

    return ret;
}
/*
 * Return false if the number of entries in the list is greater than or equal
 * to max_entries.
 */
gboolean
session_list_is_full (SessionList *list)
{
    guint table_size;

    table_size = g_slist_length (list->session_entry_slist);
    if (table_size < list->max_entries) {
        return FALSE;
    } else {
        return TRUE;
    }
}
/*
 */
static void
session_list_dump_entry (gpointer data,
                         gpointer user_data)
{
    SessionEntry *entry = SESSION_ENTRY (data);

    session_entry_prettyprint (entry);
}
/*
 */
void
session_list_prettyprint (SessionList *list)
{
    g_debug ("SessionList: 0x%" PRIxPTR, (uintptr_t)list);
    session_list_lock (list);
    g_slist_foreach (list->session_entry_slist,
                     session_list_dump_entry,
                     NULL);
    session_list_unlock (list);
}
/*
 */
void
session_list_foreach (SessionList *list,
                      GFunc        func,
                      gpointer     user_data)
{
    g_slist_foreach (list->session_entry_slist,
                     func,
                     user_data);
}
