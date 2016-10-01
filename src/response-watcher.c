#include <errno.h>
#include <glib.h>
#include <pthread.h>

#include "response-watcher.h"
#include "control-message.h"
#include "data-message.h"

#define RESPONSE_WATCHER_TIMEOUT 1e6

response_watcher_t*
response_watcher_new (Tab             *tab)
{
    response_watcher_t *watcher = NULL;

    if (tab == NULL)
        g_error ("response_watcher_new passed NULL Tab");
    watcher = calloc (1, sizeof (response_watcher_t));
    if (watcher == NULL)
        g_error ("failed to allocate response_watcher_t: %s",
                 strerror (errno));
    g_object_ref (tab);
    watcher->tab = tab;

    return watcher;
}

void
response_watcher_free (response_watcher_t *watcher)
{
    g_debug ("response_watcher_free");
    if (watcher == NULL)
        g_error ("response_watcher_free passed NULL pointer");
    g_object_unref (watcher->tab);
    free (watcher);
}

gint
response_watcher_write_response (const gint fd,
                                 const DataMessage *msg)
{
    ssize_t written = 0;

    g_debug ("response_watcher_write_response");
    g_debug ("  writing 0x%x bytes", msg->size);
    g_debug_bytes (msg->data, msg->size, 16, 4);
    written = write_all (fd, msg->data, msg->size);
    if (written <= 0)
        g_warning ("write failed (%d) on fd %d for session 0x%x: %s",
                   written, fd, msg->session, strerror (errno));

    return written;
}

ssize_t
response_watcher_process_data_message (DataMessage *msg)
{
    gint fd;
    ssize_t written;

    g_debug ("response_watcher_thread got response message: 0x%x size %d",
             msg, msg->size);
    fd = session_data_send_fd (msg->session);
    written = response_watcher_write_response (fd, msg);
    if (written <= 0) {
        /* should have a reference to the session manager so we can remove
         * session after an error like this
         */
        g_warning ("Failed to write response to fd: %d", fd);
    }
}


void*
response_watcher_thread (void *data)
{
    response_watcher_t *watcher;
    GObject *obj;
    watcher = (response_watcher_t*)data;
    do {
        g_debug ("response_watcher_thread waiting for response on tab: 0x%x",
                 watcher->tab);
        obj = tab_get_response (watcher->tab);
        g_debug ("response_watcher_thread got obj: 0x%x", obj);
        if (IS_CONTROL_MESSAGE (obj))
            process_control_message (CONTROL_MESSAGE (obj));
        if (IS_DATA_MESSAGE (obj))
            response_watcher_process_data_message (DATA_MESSAGE (obj));
        g_object_unref (obj);
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
