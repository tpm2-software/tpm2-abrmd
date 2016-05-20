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
                                 const blob_t *blob)
{
    ssize_t written = 0;
    size_t written_total = 0;

    do {
        g_debug ("writing %d bytes starting at 0x%x to fd %d",
                 blob_get_size (blob) - written_total,
                 blob_get_data (blob) + written_total,
                 fd);
        written = write (fd,
                         blob_get_data (blob) + written_total,
                         blob_get_size (blob) - written_total);
        if (written <= 0) {
            /* close & free session here? */
            g_warning ("write failed (%d) on fd %d for session 0x%x: %s",
                       written, fd, blob_get_session (blob), strerror (errno));
            goto out;
        } else {
            g_debug ("wrote %d bytes to fd %d", written, fd);
        }
        written_total += written;
    } while (written_total < blob_get_size (blob));
out:
    return written;
}


void*
response_watcher_thread (void *data)
{
    response_watcher_t *watcher;
    blob_t *blob;
    gint fd;
    ssize_t written;

    watcher = (response_watcher_t*)data;
    do {
        pthread_testcancel ();
        blob = tab_get_timeout_response (watcher->tab, RESPONSE_WATCHER_TIMEOUT);
        if (blob == NULL)
            continue;
        g_debug ("response_watcher_thread got response blob: 0x%x size %d",
                 blob, blob_get_size (blob));
        fd = session_data_send_fd (blob_get_session (blob));
        written = response_watcher_write_response (fd, blob);
        if (written <= 0) {

        }
        blob_free (blob);
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
