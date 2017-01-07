#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <inttypes.h>
#include <string.h>

#include "tabrmd.h"
#include "tcti-tabrmd.h"
#include "tcti-tabrmd-priv.h"
#include "util.h"

static TSS2_RC
tss2_tcti_tabrmd_transmit (TSS2_TCTI_CONTEXT *tcti_context,
                         size_t size,
                         uint8_t *command)
{
    int ret = 0;
    TSS2_RC tss2_ret = TSS2_RC_SUCCESS;

    g_debug ("tss2_tcti_tabrmd_transmit");
    ret = pthread_mutex_lock (&TSS2_TCTI_TABRMD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error acquiring TCTI lock: %s", strerror (errno));
    g_debug ("blocking on PIPE_TRANSMIT: %d", TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context));
    ret = write_all (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context),
                     command,
                     size);
    /* should switch on possible errors to translate to TSS2 error codes */
    if (ret == size)
        tss2_ret = TSS2_RC_SUCCESS;
    else
        tss2_ret = TSS2_TCTI_RC_GENERAL_FAILURE;
    ret = pthread_mutex_unlock (&TSS2_TCTI_TABRMD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error unlocking TCTI mutex: %s", strerror (errno));
    return tss2_ret;
}

static TSS2_RC
tss2_tcti_tabrmd_receive (TSS2_TCTI_CONTEXT *tcti_context,
                        size_t *size,
                        uint8_t *response,
                        int32_t timeout)
{
    ssize_t ret = 0;
    TSS2_RC tss2_ret = TSS2_RC_SUCCESS;

    g_debug ("tss2_tcti_tabrmd_receive");
    if (timeout != TSS2_TCTI_TIMEOUT_BLOCK)
        return TSS2_TCTI_RC_BAD_VALUE;
    ret = pthread_mutex_lock (&TSS2_TCTI_TABRMD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error acquiring TCTI lock: %s", strerror (errno));
    ret = read (TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context), response, *size);
    switch (ret) {
    case -1:
        g_debug ("tss2_tcti_tabrmd_receive: error reading from pipe: %s",
                 strerror (errno));
        tss2_ret = TSS2_TCTI_RC_IO_ERROR;
        break;
    case 0:
        g_debug ("tss2_tcti_tabrmd_receive: read returned 0: EOF!");
        tss2_ret = TSS2_TCTI_RC_NO_CONNECTION;
        break;
    default:
        g_debug ("tss2_tcti_tabrmd_receive: read returned: 0x%", PRIdMAX, ret);
        *size = ret;
        break;
    }
    ret = pthread_mutex_unlock (&TSS2_TCTI_TABRMD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Error unlocking TCTI mutex: %s", strerror (errno));
    return tss2_ret;
}

static void
tss2_tcti_tabrmd_finalize (TSS2_TCTI_CONTEXT *tcti_context)
{
    int ret = 0;

    g_debug ("tss2_tcti_tabrmd_finalize");
    if (tcti_context == NULL)
        g_warning ("tss2_tcti_tabrmm_finalize: NULL context");
    if (TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context));
        TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close receive pipe: %s", strerror (errno));
    if (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context));
        TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close send pipe: %s", strerror (errno));
    g_object_unref (TSS2_TCTI_TABRMD_PROXY (tcti_context));
}

static TSS2_RC
tss2_tcti_tabrmd_cancel (TSS2_TCTI_CONTEXT *tcti_context)
{
    GVariant *id_variant, *ret_variant;
    TSS2_RC ret = TSS2_RC_SUCCESS;
    GError *error = NULL;
    gboolean cancel_ret;

    g_info("tss2_tcti_tabrmd_cancel: id 0x%x", TSS2_TCTI_TABRMD_ID (tcti_context));
    cancel_ret = tcti_tabrmd_call_cancel_sync (TSS2_TCTI_TABRMD_PROXY (tcti_context),
                                                      TSS2_TCTI_TABRMD_ID (tcti_context),
                                                      &ret,
                                                      NULL,
                                                      &error);
    if (cancel_ret == FALSE) {
        g_warning ("cancel command failed: %s", error->message);
        g_error_free (error);
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }

    return ret;
}

static TSS2_RC
tss2_tcti_tabrmd_get_poll_handles (TSS2_TCTI_CONTEXT *tcti_context,
                                 TSS2_TCTI_POLL_HANDLE *handles,
                                 size_t *num_handles)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

static TSS2_RC
tss2_tcti_tabrmd_set_locality (TSS2_TCTI_CONTEXT *tcti_context,
                             guint8             locality)
{
    gboolean status;
    TSS2_RC ret;
    GError *error = NULL;

    g_info ("tss2_tcti_tabrmd_set_locality: id 0x%x", TSS2_TCTI_TABRMD_ID (tcti_context));
    status = tcti_tabrmd_call_set_locality_sync (TSS2_TCTI_TABRMD_PROXY (tcti_context),
                                                        TSS2_TCTI_TABRMD_ID (tcti_context),
                                                        locality,
                                                        &ret,
                                                        NULL,
                                                        &error);

    if (status == FALSE) {
        g_warning ("set locality command failed: %s", error->message);
        g_error_free (error);
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }

    return ret;
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
    TSS2_TCTI_TABRMD_CONTEXT *tcti_context = (TSS2_TCTI_TABRMD_CONTEXT*)user_data;
    int ret;

    g_debug("on_name_appeared");
    /* get sockets for transmit / receive */
    message_call =
        g_dbus_message_new_method_call (name_owner,
                                        TABRMD_DBUS_PATH,
                                        TABRMD_DBUS_INTERFACE,
                                        TABRMD_DBUS_METHOD_CREATE_CONNECTION);
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
                 TABRMD_DBUS_METHOD_CREATE_CONNECTION);
        goto out;
    }
    if (g_dbus_message_get_message_type (message_reply) ==
        G_DBUS_MESSAGE_TYPE_ERROR)
    {
        g_dbus_message_to_gerror (message_reply, &error);
        g_error ("Call to DBus method %s failed: %s",
                 TABRMD_DBUS_METHOD_CREATE_CONNECTION, error->message);
        goto out;
    }
    fd_list = g_dbus_message_get_unix_fd_list (message_reply);
    gint fd_list_length = g_unix_fd_list_get_length(fd_list);
    if (fd_list_length != 2)
        g_error ("Expecting 2 fds, received %d", fd_list_length);
    g_debug ("got some fds: %d", fd_list_length);
    TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context) =
        g_unix_fd_list_get (fd_list, 0, &error);
    TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context) =
        g_unix_fd_list_get (fd_list, 1, &error);
    g_debug ("receive fd: %d", TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context));
    g_debug ("transmit fd: %d", TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context));
    ret = pthread_mutex_unlock (&TSS2_TCTI_TABRMD_MUTEX (tcti_context));
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
    TSS2_TCTI_TABRMD_CONTEXT *tcti_context = (TSS2_TCTI_TABRMD_CONTEXT*)user_data;
    int ret = 0;

    g_warning ("No owner for name: %s", name);
    ret = pthread_mutex_trylock (&TSS2_TCTI_TABRMD_MUTEX (tcti_context));
    if (ret != 0)
        g_error ("Unable to acquire init lock through trylock: %s",
                 strerror (errno));
    if (TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context));
        TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close receive pipe: %s", strerror (errno));
    if (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context) != 0) {
        ret = close (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context));
        TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context) = 0;
    }
    if (ret != 0 && ret != EBADF)
        g_warning ("Failed to close send pipe: %s", strerror (errno));
}

void
init_function_pointers (TSS2_TCTI_CONTEXT *tcti_context)
{
    TSS2_TCTI_MAGIC (tcti_context)            = TSS2_TCTI_TABRMD_MAGIC;
    TSS2_TCTI_VERSION (tcti_context)          = 1;
    TSS2_TCTI_TRANSMIT (tcti_context)         = tss2_tcti_tabrmd_transmit;
    TSS2_TCTI_RECEIVE (tcti_context)          = tss2_tcti_tabrmd_receive;
    TSS2_TCTI_FINALIZE (tcti_context)         = tss2_tcti_tabrmd_finalize;
    TSS2_TCTI_CANCEL (tcti_context)           = tss2_tcti_tabrmd_cancel;
    TSS2_TCTI_GET_POLL_HANDLES (tcti_context) = tss2_tcti_tabrmd_get_poll_handles;
    TSS2_TCTI_SET_LOCALITY (tcti_context)     = tss2_tcti_tabrmd_set_locality;
}

static gboolean
tcti_tabrmd_call_create_connection_sync_fdlist (TctiTabrmd        *proxy,
                                                       GVariant     **out_fds,
                                                       guint64       *out_id,
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
    g_variant_get (_ret, "(@aht)", out_fds, out_id);
    g_variant_unref (_ret);
_out:
    return _ret != NULL;
}

TSS2_RC
tss2_tcti_tabrmd_init (TSS2_TCTI_CONTEXT *tcti_context,
                       size_t            *size)
{
    GError *error = NULL;
    GVariant *fds_variant;
    guint64 id;
    GUnixFDList *fd_list;
    gboolean call_ret;
    int ret;

    if (tcti_context == NULL && size == NULL)
        return TSS2_TCTI_RC_BAD_VALUE;
    if (tcti_context == NULL && size != NULL) {
        *size = sizeof (TSS2_TCTI_TABRMD_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    init_function_pointers (tcti_context);
    ret = pthread_mutex_init (&TSS2_TCTI_TABRMD_MUTEX (tcti_context), NULL);
    if (ret != 0)
        g_error ("Failed to initialize initialization mutex: %s",
                 strerror (errno));

    TSS2_TCTI_TABRMD_PROXY (tcti_context) =
        tcti_tabrmd_proxy_new_for_bus_sync (
            G_BUS_TYPE_SESSION,
            G_DBUS_PROXY_FLAGS_NONE,
            TABRMD_DBUS_NAME,              /* bus name */
            TABRMD_DBUS_PATH, /* object */
            NULL,                          /* GCancellable* */
            &error);
    if (TSS2_TCTI_TABRMD_PROXY (tcti_context) == NULL)
        g_error ("failed to allocate dbus proxy object: %s", error->message);
    call_ret = tcti_tabrmd_call_create_connection_sync_fdlist (
        TSS2_TCTI_TABRMD_PROXY (tcti_context),
        &fds_variant,
        &id,
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
    TSS2_TCTI_TABRMD_PIPE_RECEIVE (tcti_context) = fd;
    fd = g_unix_fd_list_get (fd_list, 1, &error);
    if (fd == -1)
        g_error ("failed to get transmit handle from GUnixFDList: %s",
                 error->message);
    TSS2_TCTI_TABRMD_PIPE_TRANSMIT (tcti_context) = fd;
    TSS2_TCTI_TABRMD_ID (tcti_context) = id;
    g_debug ("initialized tabrmd TCTI context with id: 0x%" PRIx64,
             TSS2_TCTI_TABRMD_ID (tcti_context));

    return TSS2_RC_SUCCESS;
}
