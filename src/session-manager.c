#include <errno.h>
#include <error.h>
#include <glib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "session-manager.h"

session_manager_t*
session_manager_new (void)
{
    session_manager_t *session_manager = NULL;

    session_manager = calloc (1, sizeof (session_manager_t));
    if (session_manager == NULL)
        g_error ("Failed to allocate memory for session_manager_t: %s",
                 strerror (errno));
    if (pthread_mutex_init (&session_manager->mutex, NULL) != 0)
        g_error ("Failed to initialize session _manager mutex: %s",
                 strerror (errno));
    session_manager->session_from_fd_table =
        g_hash_table_new (g_int_hash, session_data_equal_fd);
    session_manager->session_from_id_table =
        g_hash_table_new (g_int64_hash, session_data_equal_id);
    return session_manager;
}

void
session_manager_free (session_manager_t *manager)
{
    gint ret;
    if (manager == NULL)
        return;
    ret = pthread_mutex_lock (&manager->mutex);
    if (ret != 0)
        g_error ("Error locking session_manager mutex: %s",
                 strerror (errno));
    g_hash_table_unref (manager->session_from_fd_table);
    g_hash_table_unref (manager->session_from_id_table);
    close (manager->wakeup_send_fd);
    ret = pthread_mutex_unlock (&manager->mutex);
    if (ret != 0)
        g_error ("Error unlocking session_manager mutex: %s",
                 strerror (errno));
    ret = pthread_mutex_destroy (&manager->mutex);
    if (ret != 0)
        g_error ("Error destroying session_manager mutex: %s",
                 strerror (errno));
    free (manager);
}

gint
session_manager_insert (session_manager_t *manager,
                        session_data_t *session)
{
    gint ret;

    ret = pthread_mutex_lock (&manager->mutex);
    if (ret != 0)
        g_error ("Error locking session_manager mutex: %s",
                 strerror (errno));
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
    return ret;
}

session_data_t*
session_manager_lookup (session_manager_t *manager,
                        gint fd_in)
{
    session_data_t *session;

    pthread_mutex_lock (&manager->mutex);
    session = g_hash_table_lookup (manager->session_from_fd_table,
                                   &fd_in);
    pthread_mutex_unlock (&manager->mutex);

    return session;
}

session_data_t*
session_manager_lookup_id (session_manager_t *manager,
                           gint64 id)
{
    session_data_t *session;

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
session_manager_remove (session_manager_t *manager,
                        session_data_t *session)
{
    gboolean ret;

    pthread_mutex_lock (&manager->mutex);
    ret = g_hash_table_remove (manager->session_from_fd_table,
                               session_data_key_fd (session));
    ret = g_hash_table_remove (manager->session_from_id_table,
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
    session_data_t *session = (session_data_t*)value;

    FD_SET (session->receive_fd, fds);
}

void
session_manager_set_fds (session_manager_t *manager,
                         fd_set *fds)
{
    pthread_mutex_lock (&manager->mutex);
    g_hash_table_foreach (manager->session_from_fd_table,
                          set_fd,
                          fds);
    pthread_mutex_unlock (&manager->mutex);
}

guint
session_manager_size (session_manager_t *manager)
{
    return g_hash_table_size (manager->session_from_fd_table);
}
