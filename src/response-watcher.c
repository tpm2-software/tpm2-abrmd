#include <errno.h>
#include <glib.h>
#include <pthread.h>

#include "response-watcher.h"

#define RESPONSE_WATCHER_TIMEOUT 1e6

response_watcher_t*
response_watcher_new (tab_t             *tab)
{
    response_watcher_t *watcher = NULL;

    if (tab == NULL)
        g_error ("response_watcher_new passed NULL tab_t");
    watcher = calloc (1, sizeof (response_watcher_t));
    if (watcher == NULL)
        g_error ("failed to allocate response_watcher_t: %s",
                 strerror (errno));
    watcher->tab = tab;

    return watcher;
}

void
response_watcher_free (response_watcher_t *watcher)
{
    g_debug ("response_watcher_free");
    if (watcher == NULL)
        g_error ("response_watcher_free passed NULL pointer");
    free (watcher);
}

gint
response_watcher_write_response (const gint fd,
                                 const DataMessage *msg)
{
    ssize_t written = 0;

    written = write_all (fd, msg->data, msg->size);
    if (written <= 0)
        g_warning ("write failed (%d) on fd %d for session 0x%x: %s",
                   written, fd, msg->session, strerror (errno));

    return written;
}


void*
response_watcher_thread (void *data)
{
    response_watcher_t *watcher;
    DataMessage *msg;
    GObject *obj;
    gint fd;
    ssize_t written;

    watcher = (response_watcher_t*)data;
    do {
        g_debug ("response_watcher_thread waiting for response on tab: 0x%x",
                 watcher->tab);
        obj = tab_get_timeout_response (watcher->tab, RESPONSE_WATCHER_TIMEOUT);
        g_debug ("response_watcher_thread got obj: 0x%x", obj);
        pthread_testcancel ();
        if (obj == NULL)
            continue;
        g_assert (IS_DATA_MESSAGE (obj));
        msg = DATA_MESSAGE (obj);
        g_debug ("response_watcher_thread got response message: 0x%x size %d",
                 obj, msg->size);
        fd = session_data_send_fd (msg->session);
        written = response_watcher_write_response (fd, msg);
        if (written <= 0) {
            /* should have a reference to the session manager so we can remove
             * session after an error like this
             */
            g_warning ("Failed to write response to fd: %d", fd);
        }
        g_object_unref (msg);
    } while (TRUE);
}

gint
response_watcher_start (response_watcher_t *watcher)
{
    if (watcher->thread != 0)
        g_error ("response_watcher already started");
    return pthread_create (&watcher->thread, NULL, response_watcher_thread, watcher);
}
gint
response_watcher_cancel (response_watcher_t *watcher)
{
    if (watcher == NULL)
        g_error ("response_watcher_cancel passed NULL watcher");
    return pthread_cancel (watcher->thread);
}
gint
response_watcher_join (response_watcher_t *watcher)
{
    if (watcher == NULL)
        g_error ("response_watcher_join passed NULL watcher");
    return pthread_join (watcher->thread, NULL);
}
