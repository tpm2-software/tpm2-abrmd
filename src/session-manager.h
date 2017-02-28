#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include "session-data.h"

G_BEGIN_DECLS

#define MAX_CONNECTIONS 100
#define MAX_CONNECTIONS_DEFAULT 27

typedef struct _SessionManagerClass {
    GObjectClass      parent;
} SessionManagerClass;

typedef struct _SessionManager {
    GObject           parent_instance;
    pthread_mutex_t   mutex;
    GHashTable       *session_from_fd_table;
    GHashTable       *session_from_id_table;
    guint             max_connections;
} SessionManager;

/* type for callbacks registered with the 'new-session' signal*/
typedef gint (*new_session_callback)(SessionManager *session_manager,
                                     SessionData    *session_data,
                                     gpointer       *data);

#define TYPE_SESSION_MANAGER              (session_manager_get_type   ())
#define SESSION_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_SESSION_MANAGER, SessionManager))
#define SESSION_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_SESSION_MANAGER, SessionManagerClass))
#define IS_SESSION_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_SESSION_MANAGER))
#define IS_SESSION_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_SESSION_MANAGER))
#define SESSION_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_SESSION_MANAGER, SessionManagerClass))

GType              session_manager_get_type    (void);
SessionManager*    session_manager_new         (gint             max_connections);
gint               session_manager_insert      (SessionManager  *manager,
                                                SessionData     *session);
gint               session_manager_remove      (SessionManager  *manager,
                                                SessionData     *session);
SessionData*       session_manager_lookup_fd   (SessionManager  *manager,
                                                gint             fd_in);
SessionData*       session_manager_lookup_id   (SessionManager  *manager,
                                                gint64           id_in);
void               session_manager_set_fds     (SessionManager  *manager,
                                                fd_set          *fds);
guint              session_manager_size        (SessionManager  *manager);
gboolean           session_manager_is_full     (SessionManager  *manager);

G_END_DECLS
#endif /* SESSION_MANAGER_H */
