#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data-message.h"
#include "session-data.h"
#include "session-watcher.h"
#include "tab.h"
#include "util.h"
DataMessage*
data_message_from_fd (SessionData     *session,
                      gint             fd)
{
    guint8 *buf = NULL;
    DataMessage *msg;
    ssize_t bytes_read;
    size_t  bytes_total = 0;

    g_debug ("data_message_from_fd: %d", fd);
    bytes_read = read_till_block (fd, &buf, &bytes_total);
    if (bytes_read == -1) {
        g_info ("read_till_block: error, or connection closed");
        if (buf)
            free (buf);
        return NULL;
    }
    return data_message_new (session, buf, bytes_total);
}

void
session_watcher_thread_cleanup (void *data)
{
    session_watcher_t *watcher = (session_watcher_t*)data;

    watcher->running = FALSE;
}

gboolean
session_watcher_session_responder (session_watcher_t *watcher,
                                   gint               fd,
                                   Tab               *tab)
{
    DataMessage *msg;
    SessionData *session;
    gint ret;

    g_debug ("session_watcher_session_responder 0x%x", watcher);
    session = session_manager_lookup_fd (watcher->session_manager, fd);
    if (session == NULL)
        g_error ("failed to get session associated with fd: %d", fd);
    else
        g_debug ("session_manager_lookup_fd for fd %d: 0x%x", fd, session);
    msg = data_message_from_fd (session, fd);
    if (msg == NULL) {
        /* msg will be NULL when read error on fd, or fd is closed (EOF)
         * In either case we remove the session and free it.
         */
        g_debug ("removing session 0x%x from session_manager 0x%x",
                 session, watcher->session_manager);
        session_manager_remove (watcher->session_manager,
                                session);
        return TRUE;
    }
    tab_send_command (tab, G_OBJECT (msg));
    /* the tab now owns this message */
    g_object_unref (msg);
    return TRUE;
}

int
wakeup_responder (session_watcher_t *watcher)
{
    g_debug ("Got new session, updating fd_set");
    char buf[3] = { 0 };
    gint ret = read (watcher->wakeup_receive_fd, buf, WAKEUP_SIZE);
    if (ret != WAKEUP_SIZE)
        g_error ("read on wakeup_receive_fd returned %d, was expecting %d",
                 ret, WAKEUP_SIZE);
    return ret;
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
    gint ret, i;

    watcher = (session_watcher_t*)data;
    pthread_cleanup_push (session_watcher_thread_cleanup, watcher);
    do {
        FD_ZERO (&watcher->session_fdset);
        session_manager_set_fds (watcher->session_manager,
                                 &watcher->session_fdset);
        FD_SET (watcher->wakeup_receive_fd, &watcher->session_fdset);
        g_debug ("session_watcher_thread: selecting session_fdset");
        ret = select (FD_SETSIZE, &watcher->session_fdset, NULL, NULL, NULL);
        if (ret == -1) {
            g_debug ("Error selecting on pipes: %s", strerror (errno));
            break;
        }
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &watcher->session_fdset) &&
                i != watcher->wakeup_receive_fd)
            {
                g_debug ("data ready on session fd: %d", i);
                session_watcher_session_responder (watcher, i, watcher->tab);
            }
            if (FD_ISSET (i, &watcher->session_fdset) &&
                i == watcher->wakeup_receive_fd)
            {
                g_debug ("data ready on wakeup_receive_fd");
                wakeup_responder (watcher);
            }
        }
    } while (TRUE);
    g_info ("session_watcher function exiting");
    pthread_cleanup_pop (1);
}

session_watcher_t*
session_watcher_new (SessionManager    *session_manager,
                     gint wakeup_receive_fd,
                     Tab               *tab)
{
    session_watcher_t *watcher;

    if (session_manager == NULL)
        g_error ("session_watcher_new passed NULL SessionManager");
    if (tab == NULL)
        g_error ("session_watcher_new passed NULL Tab");
    watcher = calloc (1, sizeof (session_watcher_t));
    if (watcher == NULL)
        g_error ("failed to allocate session_watcher_t: %s", strerror (errno));
    g_object_ref (tab);
    g_object_ref (session_manager);
    watcher->session_manager = session_manager;
    watcher->wakeup_receive_fd = wakeup_receive_fd;
    watcher->running = FALSE;
    watcher->tab = tab;
    return watcher;
}

/* Not doing any sanity checks here. Be sure to shut things downon your own
 * first.
 */
void
session_watcher_free (session_watcher_t *watcher)
{
    if (watcher == NULL)
        return;
    g_object_unref (watcher->tab);
    g_object_unref (watcher->session_manager);
    free (watcher);
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
