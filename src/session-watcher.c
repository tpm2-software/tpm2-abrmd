#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "session-watcher.h"

int
session_watcher_echo (gint in_fd,
                      gint out_fd,
                      char *buf)
{
    ssize_t ret;

    memset (buf, 0, BUF_SIZE);
    ret = read (in_fd, buf, BUF_SIZE);
    switch (ret) {
    case 0:
        g_info ("read EOF, client connection is closed");
        goto out;
    case -1:
        g_warning ("read on fd %d failed: %s", strerror (errno));
        goto out;
    default: break;
    }
    ret = write (out_fd, buf, ret);
    switch (ret) {
    case 0:
        g_info ("wrote 0 bytes, client connection is closed");
        goto out;
    case -1:
        g_warning ("write on fd %d failed: %s", strerror (errno));
        goto out;
    default: break;
    }
out:
    return ret;
}

void
session_watcher_thread_cleanup (void *data)
{
    session_watcher_t *watcher = (session_watcher_t*)data;

    watcher->running = FALSE;
}

int
session_watcher_session_responder (session_watcher_t *watcher,
                                   gint fd,
                                   gpointer user_data)
{
    session_t *session;
    gint ret;

    session = session_manager_lookup (watcher->session_manager, fd);
    if (session == NULL)
        g_error ("failed to get session associated with fd: %d", fd);
    ret = session_watcher_echo (session->receive_fd,
                                session->send_fd,
                                watcher->buf);
    if (ret <= 0) {
        /* read / write returns -1 on error
         * read / write returns 0 when pipe is closed (EOF)
         */
        session_manager_remove (watcher->session_manager,
                                session);
        session_free (session);
    }
    return ret;
}

int
session_watcher_wakeup_responder (session_watcher_t *watcher,
                                  gpointer user_data)
{
    g_debug ("Got new session, updating fd_set");
    char buf[3] = { 0 };
    gint ret = read (watcher->wakeup_receive_fd, buf, WAKEUP_SIZE);
    if (ret != WAKEUP_SIZE)
        g_error ("read on wakeup_receive_fd returned %d, was expecting %d",
                 ret, WAKEUP_SIZE);
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
                watcher->session_callback (watcher, i, watcher->user_data);
            }
            if (FD_ISSET (i, &watcher->session_fdset) &&
                i == watcher->wakeup_receive_fd)
            {
                g_debug ("data ready on wakeup_receive_fd");
                watcher->wakeup_callback (watcher, watcher->user_data);
            }
        }
    } while (TRUE);
    g_info ("session_watcher function exiting");
    pthread_cleanup_pop (1);
}

session_watcher_t*
session_watcher_new (session_manager_t *session_manager,
                     gint wakeup_receive_fd)
{
    return session_watcher_new_full (session_manager,
                                     wakeup_receive_fd,
                                     NULL,
                                     NULL,
                                     NULL);
}

session_watcher_t*
session_watcher_new_full (session_manager_t *session_manager,
                          gint wakeup_receive_fd,
                          session_callback_t session_cb,
                          wakeup_callback_t wakeup_cb,
                          gpointer user_data)
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
    watcher->user_data = user_data;
    if (session_cb == NULL)
        watcher->session_callback = session_watcher_session_responder;
    else
        watcher->session_callback = session_cb;
    if (wakeup_cb == NULL)
        watcher->wakeup_callback = session_watcher_wakeup_responder;
    else
        watcher->wakeup_callback = wakeup_cb;
    watcher->buf = calloc (1, BUF_SIZE);
    if (watcher->buf == NULL)
        g_error ("failed to allocate data buffer for watcher: %s",
                 strerror (errno));

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
    if (watcher == NULL)
        return;
    if (watcher->buf != NULL)
        free (watcher->buf);
    if (watcher)
        free (watcher);
}
