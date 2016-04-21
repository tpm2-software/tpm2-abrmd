#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <tss2-tabd.h>
#include <tss2-tcti-tabd.h>
#include "tss2-tcti-tabd-priv.h"

static TSS2_RC
tss2_tcti_tabd_transmit (TSS2_TCTI_CONTEXT *tcti_context,
                         size_t size,
                         uint8_t *command)
{
    int ret = 0;
    TSS2_RC tss2_ret = TSS2_RC_SUCCESS;

    g_debug ("tss2_tcti_tabd_transmit");
    ret = pthread_mutex_lock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error acquiring TCTI lock: %s", strerror (errno));
    g_debug ("blocking on PIPE_TRANSMIT: %d", TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context));
    /* should loop till all bytes written */
    ret = write (TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context), command, size);
    /* should switch on possible errors to translate to TSS2 error codes */
    if (ret == size)
        tss2_ret = TSS2_RC_SUCCESS;
    else
        tss2_ret = TSS2_TCTI_RC_GENERAL_FAILURE;
    ret = pthread_mutex_unlock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error unlocking TCTI mutex: %s", strerror (errno));
    return tss2_ret;
}

static TSS2_RC
tss2_tcti_tabd_receive (TSS2_TCTI_CONTEXT *tcti_context,
                        size_t *size,
                        uint8_t *response,
                        int32_t timeout)
{
    ssize_t ret = 0;
    TSS2_RC tss2_ret = TSS2_RC_SUCCESS;

    g_debug ("tss2_tcti_tabd_receive");
    ret = pthread_mutex_lock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error acquiring TCTI lock: %s", strerror (errno));
    ret = read (TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context), response, *size);
    if (ret != -1) {
        *size = ret;
        tss2_ret = TSS2_RC_SUCCESS;
    } else
        tss2_ret = TSS2_TCTI_RC_GENERAL_FAILURE;
    ret = pthread_mutex_unlock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error unlocking TCTI mutex: %s", strerror (errno));
    return tss2_ret;
}

static void
tss2_tcti_tabd_finalize (TSS2_TCTI_CONTEXT *tcti_context)
{
    GMainLoop *gmain_loop = TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context);
    int ret = 0;

    g_debug ("tss2_tcti_tabd_finalize");
    if (tcti_context == NULL)
        g_warning ("tss2_tcti_tabd_finalize: NULL context");
    if (gmain_loop && g_main_loop_is_running (gmain_loop))
        g_main_loop_quit (gmain_loop);
    TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context) = NULL;
    if (TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context));
        TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close receive pipe: %s", strerror (errno));
    if (TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context));
        TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close send pipe: %s", strerror (errno));
    ret = pthread_join (TSS2_TCTI_TABD_THREAD_ID (tcti_context), NULL);
    if (ret != 0)
        g_warning ("pthread_join: %s", strerror (ret));
    g_object_unref (TSS2_TCTI_TABD_PROXY (tcti_context));
    free (tcti_context);
}

static TSS2_RC
tss2_tcti_tabd_cancel (TSS2_TCTI_CONTEXT *tcti_context)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

static TSS2_RC
tss2_tcti_tabd_get_poll_handles (TSS2_TCTI_CONTEXT *tcti_context,
                                 TSS2_TCTI_POLL_HANDLE *handles,
                                 size_t *num_handles)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

static TSS2_RC
tss2_tcti_tabd_set_locality (TSS2_TCTI_CONTEXT *tcti_context, uint8_t locality)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

static void
on_name_appeared (GDBusConnection *connection,
                  const gchar *name,
                  const gchar *name_owner,
                  gpointer user_data)
{
    GDBusMessage *message_call = NULL, *message_reply = NULL;
    GError *error = NULL;
    GUnixFDList *fd_list = NULL;
    TSS2_TCTI_TABD_CONTEXT *tcti_context = (TSS2_TCTI_TABD_CONTEXT*)user_data;
    int ret;

    g_debug("on_name_appeared");
    /* get sockets for transmit / receive */
    message_call =
        g_dbus_message_new_method_call (name_owner,
                                        TAB_DBUS_PATH,
                                        TAB_DBUS_INTERFACE,
                                        TAB_DBUS_METHOD_CREATE_CONNECTION);
    message_reply =
        g_dbus_connection_send_message_with_reply_sync (
            connection,
            message_call,
            G_DBUS_SEND_MESSAGE_FLAGS_NONE,
            -1, /* may want a more sane timeout */
            NULL, /* out_serial */
            NULL, /* cancellable */
            &error);
    if (message_reply == NULL) {
        g_error ("Call to DBus method %s returned a null result.",
                 TAB_DBUS_METHOD_CREATE_CONNECTION);
        goto out;
    }
    if (g_dbus_message_get_message_type (message_reply) ==
        G_DBUS_MESSAGE_TYPE_ERROR)
    {
        g_dbus_message_to_gerror (message_reply, &error);
        g_error ("Call to DBus method %s failed: %s",
                 TAB_DBUS_METHOD_CREATE_CONNECTION, error->message);
        goto out;
    }
    fd_list = g_dbus_message_get_unix_fd_list (message_reply);
    gint fd_list_length = g_unix_fd_list_get_length(fd_list);
    if (fd_list_length != 2)
        g_error ("Expecting 2 fds, received %d", fd_list_length);
    g_debug ("got some fds: %d", fd_list_length);
    TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context) =
        g_unix_fd_list_get (fd_list, 0, &error);
    TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context) =
        g_unix_fd_list_get (fd_list, 1, &error);
    g_debug ("receive fd: %d", TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context));
    g_debug ("transmit fd: %d", TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context));
    ret = pthread_mutex_unlock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Failed to unlock init mutex: %s", strerror (errno));
out:
    if (error)
        g_error_free (error);
    g_object_unref (message_call);
    g_object_unref (message_reply);
}

static void
on_name_vanished (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
    TSS2_TCTI_TABD_CONTEXT *tcti_context = (TSS2_TCTI_TABD_CONTEXT*)user_data;
    int ret = 0;

    g_warning ("No owner for name: %s", name);
    ret = pthread_mutex_trylock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Unable to acquire init lock through trylock: %s",
                 strerror (errno));
    if (TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context));
        TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close receive pipe: %s", strerror (errno));
    if (TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context));
        TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close send pipe: %s", strerror (errno));
}

static void*
tss2_tcti_tabd_gmain_thread(void *arg)
{
    TSS2_TCTI_TABD_CONTEXT *tcti_context = (TSS2_TCTI_TABD_CONTEXT*)arg;

    TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context) = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context));
    TSS2_TCTI_TABD_WATCHER_ID (tcti_context) = 0;
}

void
init_function_pointers (TSS2_TCTI_CONTEXT *tcti_context)
{
    tss2_tcti_context_magic (tcti_context) = TSS2_TCTI_TABD_MAGIC;
    tss2_tcti_context_version (tcti_context) = 1;
    tss2_tcti_context_transmit (tcti_context) = tss2_tcti_tabd_transmit;
    tss2_tcti_context_receive (tcti_context) = tss2_tcti_tabd_receive;
    tss2_tcti_context_finalize (tcti_context) = tss2_tcti_tabd_finalize;
    tss2_tcti_context_cancel (tcti_context) = tss2_tcti_tabd_cancel;
    tss2_tcti_context_get_poll_handles (tcti_context) = tss2_tcti_tabd_get_poll_handles;
    tss2_tcti_context_set_locality (tcti_context) = tss2_tcti_tabd_set_locality;
}

static gboolean
tab_call_create_connection_sync_fdlist (Tab           *proxy,
                                        GVariant     **out_fds,
                                        GUnixFDList  **out_fd_list,
                                        GCancellable  *cancellable,
                                        GError       **error)
{
    GVariant *_ret;
    _ret = g_dbus_proxy_call_with_unix_fd_list_sync (G_DBUS_PROXY (proxy),
        "CreateConnection",
        g_variant_new ("()"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        out_fd_list,
        cancellable,
        error);
    if (_ret == NULL)
        goto _out;
    g_variant_get (_ret, "(@ah)", out_fds);
    g_variant_unref (_ret);
_out:
    return _ret != NULL;
}

/* Initialize the TCTI. We lock the TCTI mutex here and kick off the thread
 * that will become the gmain loop. This is the thread that handles all of the
 * dbus events. The TCTI mutex remains locked until the TCTI becomes usable.
 * "Usable" means that a client can call the standard TCTI functions. This
 * requires that we have a connection to tss2-tabd. The typical flow will
 * unlock the mutex in the 'on_name_appeared' function.
 */
TSS2_RC
tss2_tcti_tabd_init (TSS2_TCTI_CONTEXT *tcti_context, size_t *size)
{
    GError *error = NULL;
    GVariant *variant;
    GUnixFDList *fd_list;
    gboolean call_ret;
    int ret;

    if (tcti_context == NULL && size == NULL)
        return TSS2_TCTI_RC_BAD_VALUE;
    if (tcti_context == NULL && size != NULL) {
        *size = sizeof (TSS2_TCTI_TABD_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    init_function_pointers (tcti_context);
    ret = pthread_mutex_init (&TSS2_TCTI_TABD_MUTEX (tcti_context), NULL);
    if (ret != 0)
        g_error ("Failed to initialize initialization mutex: %s",
                 strerror (errno));

    ret = pthread_mutex_lock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Failed to acquire init mutex in init function: %s",
                 strerror (errno));
    ret = pthread_create(&TSS2_TCTI_TABD_THREAD_ID (tcti_context),
                         NULL,
                         tss2_tcti_tabd_gmain_thread,
                         tcti_context);
    if (ret != 0) {
        g_error ("Failed to create thread for TABD TCTI: %s", strerror (ret));
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }
    TSS2_TCTI_TABD_PROXY (tcti_context) = tab_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        TAB_DBUS_NAME,              /* bus name */
        TAB_DBUS_PATH, /* object */
        NULL,                          /* GCancellable* */
        &error);
    if (TSS2_TCTI_TABD_PROXY (tcti_context) == NULL)
        g_error ("failed to allocate dbus proxy object: %s", error->message);
    call_ret = tab_call_create_connection_sync_fdlist (
        TSS2_TCTI_TABD_PROXY (tcti_context),
        &variant,
        &fd_list,
        NULL,
        &error);
    if (call_ret == FALSE)
        g_error ("Failed to create connection with service: %s",
                 error->message);
    if (fd_list == NULL)
        g_error ("call to CreateConnection returned a NULL GUnixFDList");
    gint num_handles = g_unix_fd_list_get_length (fd_list);
    if (num_handles != 2)
        g_error ("CreateConnection expected to return 2 handles, received %d",
                 num_handles);
    gint fd = g_unix_fd_list_get (fd_list, 0, &error);
    if (fd == -1)
        g_error ("unable to get receive handle from GUnixFDList: %s",
                 error->message);
    TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context) = fd;
    fd = g_unix_fd_list_get (fd_list, 1, &error);
    if (fd == -1)
        g_error ("failed to get transmit handle from GUnixFDList: %s",
                 error->message);
    TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context) = fd;
    ret = pthread_mutex_unlock (&TSS2_TCTI_TABD_MUTEX (tcti_context));
    if (ret != 0)
                g_error ("Failed to unlock init mutex: %s", strerror (errno));

    return TSS2_RC_SUCCESS;
}
