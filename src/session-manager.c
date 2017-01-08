#include <errno.h>
#include <error.h>
#include <glib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "session-manager.h"

static gpointer session_manager_parent_class = NULL;

enum {
    SIGNAL_0,
    SIGNAL_NEW_SESSION,
    N_SIGNALS,
};

static guint signals [N_SIGNALS] = { 0, };

SessionManager*
session_manager_new (void)
{
    SessionManager *session_manager;

    session_manager = SESSION_MANAGER (g_object_new (TYPE_SESSION_MANAGER,
                                                     NULL));
    if (pthread_mutex_init (&session_manager->mutex, NULL) != 0)
        g_error ("Failed to initialize session _manager mutex: %s",
                 strerror (errno));
    /* These two data structures must be kept in sync. When the
     * session-manager object is destoryed the session-data objects in these
     * hash tables will be free'd by the g_object_unref function. We only
     * set this for one of the hash tables because we only want to free
     * each session-data object once.
     */
    session_manager->session_from_fd_table =
        g_hash_table_new_full (g_int_hash,
                               session_data_equal_fd,
                               NULL,
                               NULL);
    session_manager->session_from_id_table =
        g_hash_table_new_full (g_int64_hash,
                               session_data_equal_id,
                               NULL,
                               (GDestroyNotify)g_object_unref);
    return session_manager;
}

static void
session_manager_finalize (GObject *obj)
{
    gint ret;
    SessionManager *manager = SESSION_MANAGER (obj);

    ret = pthread_mutex_lock (&manager->mutex);
    if (ret != 0)
        g_error ("Error locking session_manager mutex: %s",
                 strerror (errno));
    g_hash_table_unref (manager->session_from_fd_table);
    g_hash_table_unref (manager->session_from_id_table);
    ret = pthread_mutex_unlock (&manager->mutex);
    if (ret != 0)
        g_error ("Error unlocking session_manager mutex: %s",
                 strerror (errno));
    ret = pthread_mutex_destroy (&manager->mutex);
    if (ret != 0)
        g_error ("Error destroying session_manager mutex: %s",
                 strerror (errno));
}
/**
 * Boilerplate GObject class init function. The only interesting thing that
 * we do here is creating / registering the 'new-session'signal. This signal
 * invokes callbacks with the new_session_callback type (see header). This
 * signal is emitted by the session_manager_insert function which is where
 * we add new sessions to those tracked by the SessionManager.
 */
static void
session_manager_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (session_manager_parent_class == NULL)
        session_manager_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize = session_manager_finalize;
    signals [SIGNAL_NEW_SESSION] =
        g_signal_new ("new-session",
                      G_TYPE_FROM_CLASS (object_class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_INT,
                      1,
                      TYPE_SESSION_DATA);
}

GType
session_manager_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "SessionManager",
                                              sizeof (SessionManagerClass),
                                              (GClassInitFunc) session_manager_class_init,
                                              sizeof (SessionManager),
                                              NULL,
                                              0);
    }
    return type;
}
gint
session_manager_insert (SessionManager    *manager,
                        SessionData       *session)
{
    gint ret;

    ret = pthread_mutex_lock (&manager->mutex);
    if (ret != 0)
        g_error ("Error locking session_manager mutex: %s",
                 strerror (errno));
    /*
     * Increase reference count on SessionData object on insert. The
     * corresponding call to g_hash_table_remove will cause the reference
     * count to be decreased (see g_hash_table_new_full).
     */
    g_object_ref (session);
    g_hash_table_insert (manager->session_from_fd_table,
                         session_data_key_fd (session),
                         session);
    g_hash_table_insert (manager->session_from_id_table,
                         session_data_key_id (session),
                         session);
    ret = pthread_mutex_unlock (&manager->mutex);
    if (ret != 0)
        g_error ("Error unlocking session_manager mutex: %s",
                 strerror (errno));
    /* not sure what to do about reference count on SEssionData obj */
    g_signal_emit (manager, signals [SIGNAL_NEW_SESSION], 0, session, &ret);
    return ret;
}

SessionData*
session_manager_lookup_fd (SessionManager *manager,
                           gint fd_in)
{
    SessionData *session;

    pthread_mutex_lock (&manager->mutex);
    session = g_hash_table_lookup (manager->session_from_fd_table,
                                   &fd_in);
    pthread_mutex_unlock (&manager->mutex);

    return session;
}

SessionData*
session_manager_lookup_id (SessionManager   *manager,
                           gint64 id)
{
    SessionData *session;

    g_debug ("locking manager mutex");
    pthread_mutex_lock (&manager->mutex);
    g_debug ("g_hash_table_lookup: session_from_id_table");
    session = g_hash_table_lookup (manager->session_from_id_table,
                                   &id);
    g_debug ("unlocking manager mutex");
    pthread_mutex_unlock (&manager->mutex);

    return session;
}

gboolean
session_manager_remove (SessionManager    *manager,
                        SessionData       *session)
{
    gboolean ret;

    g_debug ("session_manager 0x%x removing session 0x%x", manager, session);
    pthread_mutex_lock (&manager->mutex);
    ret = g_hash_table_remove (manager->session_from_fd_table,
                               session_data_key_fd (session));
    if (ret != TRUE)
        g_error ("failed to remove session 0x%x from g_hash_table 0x%x using "
                 "key %d", session, manager->session_from_fd_table,
                 session_data_key_fd (session));
    ret = g_hash_table_remove (manager->session_from_id_table,
                               session_data_key_id (session));
    if (ret != TRUE)
        g_error ("failed to remove session 0x%x from g_hash_table 0x%x using "
                 "key %d", session, manager->session_from_fd_table,
                 session_data_key_id (session));
    pthread_mutex_unlock (&manager->mutex);

    return ret;
}

void
set_fd (gpointer key,
        gpointer value,
        gpointer user_data)
{
    fd_set *fds = (fd_set*)user_data;
    SessionData *session = (SessionData*)value;

    FD_SET (session_data_receive_fd (session), fds);
}

void
session_manager_set_fds (SessionManager   *manager,
                         fd_set *fds)
{
    pthread_mutex_lock (&manager->mutex);
    g_hash_table_foreach (manager->session_from_fd_table,
                          set_fd,
                          fds);
    pthread_mutex_unlock (&manager->mutex);
}

guint
session_manager_size (SessionManager   *manager)
{
    return g_hash_table_size (manager->session_from_fd_table);
}
