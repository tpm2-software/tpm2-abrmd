/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef SESSION_LIST_H
#define SESSION_LIST_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <tss2/tss2_tpm2_types.h>

#include "connection.h"
#include "session-entry.h"

G_BEGIN_DECLS

#define SESSION_LIST_MAX_ABANDONED_MAX 4
#define SESSION_LIST_MAX_ABANDONED_DEFAULT SESSION_LIST_MAX_ABANDONED_MAX

#define SESSION_LIST_MAX_ENTRIES_DEFAULT 4
#define SESSION_LIST_MAX_ENTRIES_MAX     64

typedef gboolean (*PruneFunc) (SessionEntry *entry, gpointer data);

typedef struct _SessionListClass {
    GObjectClass      parent;
} SessionListClass;

typedef struct _SessionList {
    GObject             parent_instance;
    GQueue             *abandoned_queue;
    guint               max_abandoned;
    guint               max_per_connection;
    GList              *session_entry_list;
} SessionList;

#define TYPE_SESSION_LIST              (session_list_get_type   ())
#define SESSION_LIST(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_SESSION_LIST, SessionList))
#define SESSION_LIST_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_SESSION_LIST, SessionListClass))
#define IS_SESSION_LIST(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_SESSION_LIST))
#define IS_SESSION_LIST_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_SESSION_LIST))
#define SESSION_LIST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_SESSION_LIST, SessionListClass))

GType          session_list_get_type          (void);
SessionList*   session_list_new               (guint             max_per_conn,
                                               guint             max_abandoned);
gboolean       session_list_insert            (SessionList      *list,
                                               SessionEntry     *entry);
SessionEntry*  session_list_lookup_handle     (SessionList      *list,
                                              TPM2_HANDLE        handle);
SessionEntry*  session_list_lookup_context_client (SessionList *list,
                                                   uint8_t     *buf,
                                                   size_t       size);
gint           session_list_remove_handle     (SessionList      *list,
                                               TPM2_HANDLE        handle);
gint           session_list_remove_connection (SessionList      *list,
                                               Connection       *connection);
void           session_list_remove            (SessionList      *list,
                                               SessionEntry     *entry);
guint          session_list_size              (SessionList      *list);
gboolean       session_list_is_full           (SessionList      *list,
                                               Connection       *connection);
void           session_list_prettyprint       (SessionList      *list);
void           session_list_foreach           (SessionList      *list,
                                               GFunc             func,
                                               gpointer          user_data);
size_t         session_list_connection_count  (SessionList      *list,
                                               Connection       *connection);
gboolean       session_list_abandon_handle    (SessionList      *list,
                                               Connection       *connection,
                                               TPM2_HANDLE       handle);
gboolean       session_list_claim             (SessionList      *list,
                                               SessionEntry     *entry,
                                               Connection       *connection);
gboolean       session_list_prune_abandoned   (SessionList      *list,
                                               PruneFunc         func,
                                               gpointer          data);

G_END_DECLS
#endif /* SESSION_LIST_H */
