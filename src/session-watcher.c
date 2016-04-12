#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "session-watcher.h"

#define BUF_SIZE 4096

int
session_watcher_echo (gint in_fd,
                      gint out_fd,
                      char *buf)
{
    ssize_t ret;

    ret = read (in_fd, buf, BUF_SIZE);
    if (ret == -1) {
        g_warning ("read on fd %d failed: %s", strerror (errno));
        return ret;
    }
    ret = write (out_fd, buf, ret);
    if (ret == -1)
        g_warning ("write on fd %d failed: %s", strerror (errno));
    return ret;
}

void
session_watcher_thread_cleanup (void *data)
{
    session_watcher_t *watcher = (session_watcher_t*)data;

    if (data == NULL)
        return;
    watcher->running = FALSE;
    if (watcher->buf)
        free (watcher->buf);
}

/* This function is run as a separate thread dedicated to monitoring the
 * active sessions with clients. It also monitors an additional file
 * descriptor that we call the wakeup_receive_fd. This pipe is the mechanism used
 * by the code that handles dbus calls to notify the session_watcher that
 * it's added a new session to the session_manager. When this happens the
 * session_watcher will begin to watch for data from this new session.
 */
void*
session_watcher_thread (void *data)
{
    session_watcher_t *watcher;
    fd_set session_set;

    if (data == NULL)
        g_error ("session_watcher thread started without any watcher_data");

    watcher = (session_watcher_t*)data;
    pthread_cleanup_push (session_watcher_thread_cleanup, watcher);
    watcher->buf = calloc (1, BUF_SIZE);
    if (watcher->buf == NULL)
        g_error ("failed to allocate buffer for session_watcher: %s",
                 strerror (errno));
    FD_ZERO (&session_set);
    session_manager_set_fds (watcher->session_manager,
                                &session_set);
    FD_SET (watcher->wakeup_receive_fd, &session_set);
    while (select (FD_SETSIZE, &session_set, NULL, NULL, NULL) != -1) {
        ssize_t ret;
        session_t *session;
        int i;
        for (i = 0; i < FD_SETSIZE; ++i) {
            g_debug ("data ready on fd: %d", i);
            if (FD_ISSET (i, &session_set) &&
                i == watcher->wakeup_receive_fd)
            {
                g_debug ("data read on wakeup_receive_fd");
                continue;
            }
            session = session_manager_lookup (watcher->session_manager,
                                              i);
            if (session == NULL)
                g_error ("failed to get session associated with fd: %d", i);
            ret = session_watcher_echo (session->receive_fd,
                                        session->send_fd,
                                        watcher->buf);
            if (ret == -1) {
                session_manager_remove (watcher->session_manager,
                                   session);
                session_free (session);
            }
        }
        if (FD_ISSET (watcher->wakeup_receive_fd, &session_set)) {
            g_debug ("Got new session, updating fd_set");
            session_manager_set_fds (watcher->session_manager,
                                        &session_set);
        }
    }
    g_info ("session_watcher function exiting");
    pthread_cleanup_pop (1);
}

session_watcher_t*
session_watcher_new (session_manager_t *session_manager,
                     gint wakeup_receive_fd)
{
    session_watcher_t *watcher;

    if (session_manager == NULL)
        g_error ("session_watcher_new passed NULL session_manager_t");
    watcher = calloc (1, sizeof (session_watcher_t));
    if (watcher == NULL)
        g_error ("failed to allocate session_watcher_t: %s", strerror (errno));
    watcher->session_manager = session_manager;
    watcher->wakeup_receive_fd = wakeup_receive_fd;
    watcher->running = FALSE;

    return watcher;
}

gint
session_watcher_start (session_watcher_t *watcher)
{
    if (watcher->thread != 0) {
        g_warning ("session_watcher already started");
        return -1;
    }
    watcher->running = TRUE;
    return pthread_create (&watcher->thread, NULL, session_watcher_thread, watcher);
}

gint
session_watcher_cancel (session_watcher_t *watcher)
{
    if (watcher == NULL) {
        g_warning ("session_watcher_cancel passed NULL watcher");
        return -1;
    }
    return pthread_cancel (watcher->thread);
}

gint
session_watcher_join (session_watcher_t *watcher)
{
    if (watcher == NULL) {
        g_warning ("session_watcher_join passed null watcher");
        return -1;
    }
    return pthread_join (watcher->thread, NULL);
}

/* Not doing any sanity checks here. Be sure to shut things downon your own
 * first.
 */
void
session_watcher_free (session_watcher_t *watcher)
{
    if (watcher)
        free (watcher);
}
