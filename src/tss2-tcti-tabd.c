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
    
    /* should loop till all bytes written */
    ret = write (TSS2_TCTI_TABD_PIPE_TRANSMIT (tcti_context), command, size);
    /* should switch on possible errors to translate to TSS2 error codes */
    if (ret == size)
        return TSS2_RC_SUCCESS;
    else
        return TSS2_TCTI_RC_GENERAL_FAILURE;
}

static TSS2_RC
tss2_tcti_tabd_receive (TSS2_TCTI_CONTEXT *tcti_context,
                        size_t *size,
                        uint8_t *response,
                        int32_t timeout)
{
    ssize_t ret = 0;
    ret = read (TSS2_TCTI_TABD_PIPE_RECEIVE (tcti_context), response, *size);
    if (ret == -1)
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    *size = ret;
    return TSS2_RC_SUCCESS;
}

static void
tss2_tcti_tabd_finalize (TSS2_TCTI_CONTEXT *tcti_context)
{
    GMainLoop *gmain_loop = TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context);
    int ret = 0;

    g_info ("tss2_tcti_tabd_finalize");
    if (gmain_loop && g_main_loop_is_running (gmain_loop))
        g_main_loop_quit (gmain_loop);
    TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context) = NULL;
    ret = pthread_join (TSS2_TCTI_TABD_THREAD_ID (tcti_context), NULL);
    if (ret != 0)
        g_warning ("pthread_join: %s", strerror (ret));
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

    TSS2_TCTI_TABD_WATCHER_ID (tcti_context) = \
        g_bus_watch_name (G_BUS_TYPE_SESSION,
                          TAB_DBUS_NAME,
                          G_BUS_NAME_WATCHER_FLAGS_NONE,
                          on_name_appeared,
                          on_name_vanished,
                          tcti_context,
                          NULL);
    TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context) = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (TSS2_TCTI_TABD_GMAIN_LOOP (tcti_context));
    g_bus_unwatch_name (TSS2_TCTI_TABD_WATCHER_ID (tcti_context));
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

TSS2_RC
tss2_tcti_tabd_init (TSS2_TCTI_CONTEXT *tcti_context, size_t *size)
{
    int ret;

    if (tcti_context == NULL && size == NULL)
        return TSS2_TCTI_RC_BAD_VALUE;
    if (tcti_context == NULL && size != NULL) {
        *size = sizeof (TSS2_TCTI_TABD_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    init_function_pointers (tcti_context);
    ret = pthread_create(&TSS2_TCTI_TABD_THREAD_ID(tcti_context),
                         NULL,
                         tss2_tcti_tabd_gmain_thread,
                         tcti_context);
    if (ret != 0) {
        g_error ("Failed to create thread for TABD TCTI: %s", strerror (ret));
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }
    return TSS2_RC_SUCCESS;
}
