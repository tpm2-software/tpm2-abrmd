/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <glib.h>
#include <inttypes.h>
#include <poll.h>
#include <string.h>

#include <tss2/tss2_tpm2_types.h>

#include "tabrmd.h"
#include "tss2-tcti-tabrmd.h"
#include "tcti-tabrmd-priv.h"
#include "tpm2-header.h"
#include "util.h"

TSS2_RC
tss2_tcti_tabrmd_transmit (TSS2_TCTI_CONTEXT *context,
                           size_t             size,
                           const uint8_t      *command)
{
    ssize_t write_ret;
    TSS2_RC tss2_ret = TSS2_RC_SUCCESS;
    GOutputStream *ostream;

    g_debug ("tss2_tcti_tabrmd_transmit");
    if (context == NULL || command == NULL) {
        return TSS2_TCTI_RC_BAD_REFERENCE;
    }
    if (size == 0) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (TSS2_TCTI_MAGIC (context) != TSS2_TCTI_TABRMD_MAGIC ||
        TSS2_TCTI_VERSION (context) != TSS2_TCTI_TABRMD_VERSION) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    if (TSS2_TCTI_TABRMD_STATE (context) != TABRMD_STATE_TRANSMIT) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    g_debug_bytes (command, size, 16, 4);
    ostream = g_io_stream_get_output_stream (TSS2_TCTI_TABRMD_IOSTREAM (context));
    g_debug ("%s: blocking write on ostream", __func__);
    write_ret = write_all (ostream, command, size);
    /* should switch on possible errors to translate to TSS2 error codes */
    switch (write_ret) {
    case -1:
        g_debug ("tss2_tcti_tabrmd_transmit: error writing to pipe: %s",
                 strerror (errno));
        tss2_ret = TSS2_TCTI_RC_IO_ERROR;
        break;
    case 0:
        g_debug ("tss2_tcti_tabrmd_transmit: EOF returned writing to pipe");
        tss2_ret = TSS2_TCTI_RC_NO_CONNECTION;
        break;
    default:
        if (write_ret == (ssize_t) size) {
            TSS2_TCTI_TABRMD_STATE (context) = TABRMD_STATE_RECEIVE;
        } else {
            g_debug ("tss2_tcti_tabrmd_transmit: short write");
            tss2_ret = TSS2_TCTI_RC_GENERAL_FAILURE;
        }
        break;
    }
    return tss2_ret;
}
/*
 * This function maps errno values to TCTI RCs.
 */
static TSS2_RC
errno_to_tcti_rc (int error_number)
{
    switch (error_number) {
    case -1:
        return TSS2_TCTI_RC_NO_CONNECTION;
    case 0:
        return TSS2_RC_SUCCESS;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
        return TSS2_TCTI_RC_TRY_AGAIN;
    case EIO:
        return TSS2_TCTI_RC_IO_ERROR;
    default:
        g_debug ("mapping errno %d with message \"%s\" to "
                 "TSS2_TCTI_RC_GENERAL_FAILURE",
                 error_number, strerror (error_number));
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }
}
/*
 * This function maps GError code values to TCTI RCs.
 */
static TSS2_RC
gerror_code_to_tcti_rc (int error_number)
{
    switch (error_number) {
    case -1:
        return TSS2_TCTI_RC_NO_CONNECTION;
    case G_IO_ERROR_WOULD_BLOCK:
        return TSS2_TCTI_RC_TRY_AGAIN;
    case G_IO_ERROR_FAILED:
    case G_IO_ERROR_HOST_UNREACHABLE:
    case G_IO_ERROR_NETWORK_UNREACHABLE:
#if G_IO_ERROR_BROKEN_PIPE != G_IO_ERROR_CONNECTION_CLOSED
    case G_IO_ERROR_CONNECTION_CLOSED:
#endif
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 44
    case G_IO_ERROR_NOT_CONNECTED:
#endif
        return TSS2_TCTI_RC_IO_ERROR;
    default:
        g_debug ("mapping errno %d with message \"%s\" to "
                 "TSS2_TCTI_RC_GENERAL_FAILURE",
                 error_number, strerror (error_number));
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }
}

#if defined(__FreeBSD__)
#ifndef POLLRDHUP
#define POLLRDHUP 0x0
#endif
#endif
/*
 * This is a thin wrapper around a call to poll. It packages up the provided
 * file descriptor and timeout and polls on that same FD for data or a hangup.
 * Returns:
 *   -1 on timeout
 *   0 when data is ready
 *   errno on error
 */
int
tcti_tabrmd_poll (int        fd,
                  int32_t    timeout)
{
    struct pollfd pollfds [] = {
        {
            .fd = fd,
             .events = POLLIN | POLLPRI | POLLRDHUP,
        }
    };
    int ret;
    int errno_tmp;

    ret = TABRMD_ERRNO_EINTR_RETRY (poll (pollfds,
                                    sizeof (pollfds) / sizeof (struct pollfd),
                                    timeout));
    errno_tmp = errno;
    switch (ret) {
    case -1:
        g_debug ("poll produced error: %d, %s",
                 errno_tmp, strerror (errno_tmp));
        return errno_tmp;
    case 0:
        g_debug ("poll timed out after %" PRId32 " milliseconds", timeout);
        return -1;
    default:
        g_debug ("poll has %d fds ready", ret);
        if (pollfds[0].revents & POLLIN) {
            g_debug ("  POLLIN");
        }
        if (pollfds[0].revents & POLLPRI) {
            g_debug ("  POLLPRI");
        }
        if (pollfds[0].revents & POLLRDHUP) {
            g_debug ("  POLLRDHUP");
        }
        return 0;
    }
}
/*
 * Read as much of the requested data as possible into the provided buffer.
 * If the read would block, return TSS2_TCTI_RC_TRY_AGAIN (a short read will
 * write data into the buffer first).
 * If an error occurs map that to the appropriate TSS2_RC and return.
 */
TSS2_RC
tcti_tabrmd_read (TSS2_TCTI_TABRMD_CONTEXT *ctx,
                  uint8_t *buf,
                  size_t size,
                  int32_t timeout)
{
    GError *error;
    ssize_t num_read;
    int ret;

    ret = tcti_tabrmd_poll (TSS2_TCTI_TABRMD_FD (ctx), timeout);
    switch (ret) {
    case -1:
        return TSS2_TCTI_RC_TRY_AGAIN;
    case 0:
        break;
    default:
        return errno_to_tcti_rc (ret);
    }

    num_read = g_input_stream_read (TSS2_TCTI_TABRMD_ISTREAM (ctx),
                                    (gchar*)&buf [ctx->index],
                                    size,
                                    NULL,
                                    &error);
    switch (num_read) {
    case 0:
        g_debug ("read produced EOF");
        return TSS2_TCTI_RC_NO_CONNECTION;
    case -1:
        g_assert (error != NULL);
        g_warning ("%s: read on istream produced error: %s", __func__,
                   error->message);
        ret = error->code;
        g_error_free (error);
        return gerror_code_to_tcti_rc (ret);
    default:
        g_debug ("successfully read %zd bytes", num_read);
        g_debug_bytes (&buf [ctx->index], num_read, 16, 4);
        /* Advance index by the number of bytes read. */
        ctx->index += num_read;
        /* short read means try again */
        if (size - num_read != 0) {
            return TSS2_TCTI_RC_TRY_AGAIN;
        } else {
            return TSS2_RC_SUCCESS;
        }
    }
}
/*
 * This is the receive function that is exposed to clients through the TCTI
 * API.
 */
TSS2_RC
tss2_tcti_tabrmd_receive (TSS2_TCTI_CONTEXT *context,
                          size_t            *size,
                          uint8_t           *response,
                          int32_t            timeout)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    TSS2_TCTI_TABRMD_CONTEXT *tabrmd_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)context;

    g_debug ("tss2_tcti_tabrmd_receive");
    if (context == NULL || size == NULL) {
        return TSS2_TCTI_RC_BAD_REFERENCE;
    }
    if (response == NULL && *size != 0) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (TSS2_TCTI_MAGIC (context) != TSS2_TCTI_TABRMD_MAGIC ||
        TSS2_TCTI_VERSION (context) != TSS2_TCTI_TABRMD_VERSION) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    if (tabrmd_ctx->state != TABRMD_STATE_RECEIVE) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    if (timeout < TSS2_TCTI_TIMEOUT_BLOCK) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (size == NULL || (response == NULL && *size != 0)) {
        return TSS2_TCTI_RC_BAD_REFERENCE;
    }
    /* response buffer must be at least as large as the header */
    if (response != NULL && *size < TPM_HEADER_SIZE) {
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    /* make sure we've got the response header */
    if (tabrmd_ctx->index < TPM_HEADER_SIZE) {
        rc = tcti_tabrmd_read (tabrmd_ctx,
                               tabrmd_ctx->header_buf,
                               TPM_HEADER_SIZE - tabrmd_ctx->index,
                               timeout);
        if (rc != TSS2_RC_SUCCESS)
            return rc;
        if (tabrmd_ctx->index == TPM_HEADER_SIZE) {
            tabrmd_ctx->header.tag  = get_response_tag  (tabrmd_ctx->header_buf);
            tabrmd_ctx->header.size = get_response_size (tabrmd_ctx->header_buf);
            tabrmd_ctx->header.code = get_response_code (tabrmd_ctx->header_buf);
            if (tabrmd_ctx->header.size < TPM_HEADER_SIZE) {
                tabrmd_ctx->state = TABRMD_STATE_TRANSMIT;
                return TSS2_TCTI_RC_MALFORMED_RESPONSE;
            }
        }
    }
    /* if response is NULL, caller is querying size, we know size isn't NULL */
    if (response == NULL) {
        *size = tabrmd_ctx->header.size;
        return TSS2_RC_SUCCESS;
    } else if (tabrmd_ctx->index == TPM_HEADER_SIZE) {
        /* once we have the header and a buffer from the caller, copy */
        memcpy (response, tabrmd_ctx->header_buf, TPM_HEADER_SIZE);
    }
    if (*size < tabrmd_ctx->header.size) {
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    /* the whole response has been read already */
    if (tabrmd_ctx->header.size == tabrmd_ctx->index) {
        goto out;
    }
    rc = tcti_tabrmd_read (tabrmd_ctx,
                           response,
                           tabrmd_ctx->header.size - tabrmd_ctx->index,
                           timeout);
out:
    if (rc == TSS2_RC_SUCCESS) {
        /* We got all the bytes we asked for, reset the index & state: done */
        *size = tabrmd_ctx->index;
        tabrmd_ctx->index = 0;
        tabrmd_ctx->state = TABRMD_STATE_TRANSMIT;
    }
    return rc;
}

void
tss2_tcti_tabrmd_finalize (TSS2_TCTI_CONTEXT *context)
{
    g_debug ("tss2_tcti_tabrmd_finalize");
    if (context == NULL) {
        g_warning ("Invalid parameter");
        return;
    }
    TSS2_TCTI_TABRMD_STATE (context) = TABRMD_STATE_FINAL;
    g_clear_object (&TSS2_TCTI_TABRMD_SOCK_CONNECT (context));
    g_clear_object (&TSS2_TCTI_TABRMD_PROXY (context));
}

TSS2_RC
tss2_tcti_tabrmd_cancel (TSS2_TCTI_CONTEXT *context)
{
    TSS2_RC ret = TSS2_RC_SUCCESS;
    GError *error = NULL;
    gboolean cancel_ret;

    if (context == NULL) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    g_info("tss2_tcti_tabrmd_cancel: id 0x%" PRIx64,
           TSS2_TCTI_TABRMD_ID (context));
    if (TSS2_TCTI_TABRMD_STATE (context) != TABRMD_STATE_RECEIVE) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    cancel_ret = tcti_tabrmd_call_cancel_sync (
                     TSS2_TCTI_TABRMD_PROXY (context),
                     TSS2_TCTI_TABRMD_ID (context),
                     &ret,
                     NULL,
                     &error);
    if (cancel_ret == FALSE) {
        g_warning ("cancel command failed with error code: 0x%" PRIx32
                   ", messag: %s", error->code, error->message);
        ret = error->code;
        g_error_free (error);
    }

    return ret;
}

TSS2_RC
tss2_tcti_tabrmd_get_poll_handles (TSS2_TCTI_CONTEXT     *context,
                                   TSS2_TCTI_POLL_HANDLE *handles,
                                   size_t                *num_handles)
{
    if (context == NULL) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    if (num_handles == NULL) {
        return TSS2_TCTI_RC_BAD_REFERENCE;
    }
    if (handles != NULL && *num_handles < 1) {
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    *num_handles = 1;
    if (handles != NULL) {
        handles [0].fd = TSS2_TCTI_TABRMD_FD (context);
    }
    return TSS2_RC_SUCCESS;
}

TSS2_RC
tss2_tcti_tabrmd_set_locality (TSS2_TCTI_CONTEXT *context,
                               guint8             locality)
{
    gboolean status;
    TSS2_RC ret = TSS2_RC_SUCCESS;
    GError *error = NULL;

    if (context == NULL) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    g_info ("tss2_tcti_tabrmd_set_locality: id 0x%" PRIx64,
            TSS2_TCTI_TABRMD_ID (context));
    if (TSS2_TCTI_TABRMD_STATE (context) != TABRMD_STATE_TRANSMIT) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    status = tcti_tabrmd_call_set_locality_sync (
                 TSS2_TCTI_TABRMD_PROXY (context),
                 TSS2_TCTI_TABRMD_ID (context),
                 locality,
                 &ret,
                 NULL,
                 &error);

    if (status == FALSE) {
        g_warning ("set locality command failed with error code: 0x%" PRIx32
                   ", message: %s", error->code, error->message);
        ret = error->code;
        g_error_free (error);
    }

    return ret;
}

/*
 * Initialization function to set context data values and function pointers.
 */
void
init_tcti_data (TSS2_TCTI_CONTEXT *context)
{
    memset (context, 0, sizeof (TSS2_TCTI_TABRMD_CONTEXT));

    TSS2_TCTI_MAGIC (context)            = TSS2_TCTI_TABRMD_MAGIC;
    TSS2_TCTI_VERSION (context)          = TSS2_TCTI_TABRMD_VERSION;
    TSS2_TCTI_TABRMD_STATE (context)     = TABRMD_STATE_TRANSMIT;
    TSS2_TCTI_TRANSMIT (context)         = tss2_tcti_tabrmd_transmit;
    TSS2_TCTI_RECEIVE (context)          = tss2_tcti_tabrmd_receive;
    TSS2_TCTI_FINALIZE (context)         = tss2_tcti_tabrmd_finalize;
    TSS2_TCTI_CANCEL (context)           = tss2_tcti_tabrmd_cancel;
    TSS2_TCTI_GET_POLL_HANDLES (context) = tss2_tcti_tabrmd_get_poll_handles;
    TSS2_TCTI_SET_LOCALITY (context)     = tss2_tcti_tabrmd_set_locality;
}

static gboolean
tcti_tabrmd_call_create_connection_sync_fdlist (TctiTabrmd     *proxy,
                                                GVariant      **out_fds,
                                                guint64        *out_id,
                                                GUnixFDList   **out_fd_list,
                                                GCancellable   *cancellable,
                                                GError        **error)
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
    if (_ret == NULL) {
        goto _out;
    }
    g_variant_get (_ret, "(@aht)", out_fds, out_id);
    g_variant_unref (_ret);
_out:
    return _ret != NULL;
}

typedef struct {
    char *name;
    GBusType type;
} bus_name_type_entry_t;

static const bus_name_type_entry_t bus_name_type_map[] = {
    {
        .name = "session",
        .type = G_BUS_TYPE_SESSION,
    },
    {
        .name = "system",
        .type = G_BUS_TYPE_SYSTEM,
    },
};
#define BUS_NAME_TYPE_MAP_LENGTH (sizeof (bus_name_type_map) / sizeof (bus_name_type_entry_t))
GBusType
tabrmd_bus_type_from_str (const char* const bus_type)
{
    size_t i;
    g_debug ("BUS_NAME_TYPE_MAP_LENGTH: %zu", BUS_NAME_TYPE_MAP_LENGTH);
    g_debug ("looking up type for bus_type string: %s", bus_type);
    for (i = 0; i < BUS_NAME_TYPE_MAP_LENGTH; ++i) {
        if (strcmp (bus_name_type_map [i].name, bus_type) == 0) {
            g_debug ("matched bus_type string \"%s\" to type %d",
                     bus_name_type_map [i].name,
                     bus_name_type_map [i].type);
            return bus_name_type_map [i].type;
        }
    }
    g_debug ("no match for bus_type string %s", bus_type);
    return G_BUS_TYPE_NONE;
}

TSS2_RC
tabrmd_kv_callback (const key_value_t *key_value,
                    gpointer user_data)
{
    tabrmd_conf_t *tabrmd_conf = (tabrmd_conf_t*)user_data;

    if (key_value == NULL || user_data == NULL) {
        g_warning ("%s passed NULL parameter", __func__);
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }
    g_debug ("key: %s / value: %s\n", key_value->key, key_value->value);
    if (strcmp (key_value->key, "bus_name") == 0) {
        tabrmd_conf->bus_name = key_value->value;
        return TSS2_RC_SUCCESS;
    } else if (strcmp (key_value->key, "bus_type") == 0) {
        tabrmd_conf->bus_type = tabrmd_bus_type_from_str (key_value->value);
        if (tabrmd_conf->bus_type == G_BUS_TYPE_NONE) {
            return TSS2_TCTI_RC_BAD_VALUE;
        }
        return TSS2_RC_SUCCESS;
    } else {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
}

/*
 * Establish a connection with the daemon. This includes calling the
 * CreateConnection dbus method, extracting the file descriptor used for
 * sending commands and receiving responses, and extracting the connection
 * ID used when sending commands over the dbus interface.
 *
 * The proxy object in the context structure must be created / valid before
 * calling this function.
 */
TSS2_RC
tcti_tabrmd_connect (TSS2_TCTI_CONTEXT *context)
{
    GError *error = NULL;
    GSocket *sock = NULL;
    GUnixFDList *fd_list = NULL;
    GVariant *fds_variant = NULL;
    gboolean call_ret;
    guint64 id;
    TSS2_RC rc = TSS2_RC_SUCCESS;

    call_ret = tcti_tabrmd_call_create_connection_sync_fdlist (
        TSS2_TCTI_TABRMD_PROXY (context),
        &fds_variant,
        &id,
        &fd_list,
        NULL,
        &error);
    if (call_ret == FALSE) {
        g_warning ("Failed to create connection with service: %s",
                 error->message);
        rc = TSS2_TCTI_RC_NO_CONNECTION;
        goto out;
    }
    if (fd_list == NULL) {
        g_critical ("call to CreateConnection returned a NULL GUnixFDList");
        rc = TSS2_TCTI_RC_NO_CONNECTION;
        goto out;
    }
    gint num_handles = g_unix_fd_list_get_length (fd_list);
    if (num_handles != 1) {
        g_critical ("CreateConnection expected to return 1 handles, received %d",
                    num_handles);
        rc = TSS2_TCTI_RC_GENERAL_FAILURE;
        goto out;
    }
    gint fd = g_unix_fd_list_get (fd_list, 0, &error);
    if (fd == -1) {
        g_critical ("unable to get receive handle from GUnixFDList: %s",
                    error->message);
        rc = TSS2_TCTI_RC_GENERAL_FAILURE;
        goto out;
    }
    sock = g_socket_new_from_fd (fd, NULL);
    TSS2_TCTI_TABRMD_SOCK_CONNECT (context) = \
        g_socket_connection_factory_create_connection (sock);
    TSS2_TCTI_TABRMD_ID (context) = id;
out:
    g_clear_pointer (&fds_variant, g_variant_unref);
    g_clear_error (&error);
    g_clear_object (&sock);
    g_clear_object (&fd_list);
    return rc;
}

/*
 * The longest configuration string we'll take. Each dbus name can be 255
 * characters long (see dbus spec). The bus_types that we support are
 * 'system' or 'session' (255 + 7 = 262). 'bus_type=' and 'bus_name=' are
 * each another 9 characters for a total of 280.
 */
#define CONF_STRING_MAX 280
TSS2_RC
Tss2_Tcti_Tabrmd_Init (TSS2_TCTI_CONTEXT *context,
                       size_t            *size,
                       const char        *conf)
{
    GError *error = NULL;
    size_t conf_len;
    char *conf_copy = NULL;
    TSS2_RC rc;
    tabrmd_conf_t tabrmd_conf = TABRMD_CONF_INIT_DEFAULT;

    if (context == NULL && size != NULL) {
        *size = sizeof (TSS2_TCTI_TABRMD_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    if (size == NULL) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (conf != NULL) {
        conf_len = strlen (conf);
        if (conf_len > CONF_STRING_MAX) {
            return TSS2_TCTI_RC_BAD_VALUE;
        }
        conf_copy = g_strdup (conf);
        if (conf_copy == NULL) {
            g_critical ("Failed to duplicate config string: %s", strerror (errno));
            return TSS2_TCTI_RC_GENERAL_FAILURE;
        }
        rc = parse_key_value_string (conf_copy,
                                     tabrmd_kv_callback,
                                     &tabrmd_conf);
        if (rc != TSS2_RC_SUCCESS) {
            goto out;
        }
    }
    /* Register dbus error mapping for tabrmd. Gets us RCs from Gerror codes */
    TABRMD_ERROR;
    init_tcti_data (context);
    TSS2_TCTI_TABRMD_PROXY (context) =
        tcti_tabrmd_proxy_new_for_bus_sync (tabrmd_conf.bus_type,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            tabrmd_conf.bus_name,
                                            TABRMD_DBUS_PATH,
                                            NULL,
                                            &error);
    if (TSS2_TCTI_TABRMD_PROXY (context) == NULL) {
        g_critical ("failed to allocate dbus proxy object: %s", error->message);
        rc = TSS2_TCTI_RC_NO_CONNECTION;
        goto out;
    }
    rc = tcti_tabrmd_connect (context);
    if (rc == TSS2_RC_SUCCESS) {
        g_debug ("initialized tabrmd TCTI context with id: 0x%" PRIx64,
                 TSS2_TCTI_TABRMD_ID (context));
    }
out:
    g_clear_pointer (&conf_copy, g_free);
    g_clear_error (&error);

    return rc;
}

/* public info structure */
static const TSS2_TCTI_INFO tss2_tcti_info = {
    .version = TSS2_TCTI_TABRMD_VERSION,
    .name = "tcti-abrmd",
    .description = "TCTI module for communication with tabrmd.",
    .config_help = "This conf string is a series of key / value pairs " \
        "where keys and values are separated by the '=' character and " \
        "each pair is separated by the ',' character. Valid keys are " \
        "\"bus_name\" and \"bus_type\".",
    .init = Tss2_Tcti_Tabrmd_Init,
};

const TSS2_TCTI_INFO*
Tss2_Tcti_Info (void)
{
    return &tss2_tcti_info;
}
