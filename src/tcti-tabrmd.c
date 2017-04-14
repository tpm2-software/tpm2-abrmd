/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <errno.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <glib.h>
#include <inttypes.h>
#include <string.h>

#include <sapi/tpm20.h>

#include "tabrmd.h"
#include "tcti-tabrmd.h"
#include "tcti-tabrmd-priv.h"
#include "tpm2-header.h"
#include "util.h"

static TSS2_RC
tss2_tcti_tabrmd_transmit (TSS2_TCTI_CONTEXT *context,
                         size_t size,
                         uint8_t *command)
{
    ssize_t write_ret;
    TSS2_RC tss2_ret = TSS2_RC_SUCCESS;

    g_debug ("tss2_tcti_tabrmd_transmit");
    if (TSS2_TCTI_MAGIC (context) != TSS2_TCTI_TABRMD_MAGIC ||
        TSS2_TCTI_VERSION (context) != TSS2_TCTI_TABRMD_VERSION) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    if (TSS2_TCTI_TABRMD_STATE (context) != TABRMD_STATE_TRANSMIT) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    g_debug_bytes (command, size, 16, 4);
    g_debug ("blocking on PIPE_TRANSMIT: %d", TSS2_TCTI_TABRMD_PIPE_TRANSMIT (context));
    write_ret = write_all (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (context),
                           command,
                           size);
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
        if (write_ret == size) {
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
    case 0:
        return TSS2_RC_SUCCESS;
    case EPROTO:
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
        return TSS2_TCTI_RC_TRY_AGAIN;
    default:
        return TSS2_TCTI_RC_IO_ERROR;
    }
}
/*
 * This function receives the tpm response header from the tabrmd. It is
 * not exposed externally and so has no parameter validation. These checks
 * should already be done by the caller.
 *
 * NOTE: this function changes the contents of the context blob.
 */
static TSS2_RC
tss2_tcti_tabrmd_receive_header (TSS2_TCTI_CONTEXT *context,
                                 uint8_t *response,
                                 int32_t  timeout)
{
    int     ret;
    TSS2_RC rc;

    ret = tpm_header_from_fd (TSS2_TCTI_TABRMD_PIPE_RECEIVE (context),
                              response);
    rc = errno_to_tcti_rc (ret);
    if (rc != TSS2_RC_SUCCESS) {
        return rc;
    }

    TSS2_TCTI_TABRMD_HEADER (context).tag = get_response_tag (response);
    TSS2_TCTI_TABRMD_HEADER (context).size = get_response_size (response);
    TSS2_TCTI_TABRMD_HEADER (context).code = get_response_code (response);

    return TSS2_RC_SUCCESS;
}
/*
 * This is the receive function that is exposed to clients through the TCTI
 * API.
 */
static TSS2_RC
tss2_tcti_tabrmd_receive (TSS2_TCTI_CONTEXT *context,
                        size_t *size,
                        uint8_t *response,
                        int32_t timeout)
{
    int ret = 0;
    TSS2_RC rc = TSS2_RC_SUCCESS;

    g_debug ("tss2_tcti_tabrmd_receive");
    if (TSS2_TCTI_MAGIC (context) != TSS2_TCTI_TABRMD_MAGIC ||
        TSS2_TCTI_VERSION (context) != TSS2_TCTI_TABRMD_VERSION) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    /* async not yet supported */
    if (timeout != TSS2_TCTI_TIMEOUT_BLOCK) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (size == NULL && response == NULL) {
        return TSS2_TCTI_RC_BAD_REFERENCE;
    }
    if (*size < TPM_HEADER_SIZE) {
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    if (TSS2_TCTI_TABRMD_STATE (context) != TABRMD_STATE_RECEIVE) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    rc = tss2_tcti_tabrmd_receive_header (context,
                                          response,
                                          timeout);
    if (rc != TSS2_RC_SUCCESS) {
        return rc;
    }
    if (TSS2_TCTI_TABRMD_HEADER (context).size > *size) {
        g_warning ("tss2_tcti_tabrmd_receive got response with size larger "
                   "than the supplied buffer");
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    if (TSS2_TCTI_TABRMD_HEADER (context).size == TPM_HEADER_SIZE) {
        g_debug ("tss2_tcti_tabrmd_receive got response header with size "
                 "equal to header size: no response body to read");
        *size = TPM_HEADER_SIZE;
        TSS2_TCTI_TABRMD_STATE (context) = TABRMD_STATE_TRANSMIT;
        return TSS2_RC_SUCCESS;
    }
    g_debug ("tss2_tcti_tabrmd_receive reading command body of size %" PRIu32,
             TSS2_TCTI_TABRMD_HEADER (context).size - TPM_HEADER_SIZE);
    ret = tpm_body_from_fd (TSS2_TCTI_TABRMD_PIPE_RECEIVE (context),
                            response + TPM_HEADER_SIZE,
                            TSS2_TCTI_TABRMD_HEADER (context).size - \
                                TPM_HEADER_SIZE);
    rc = errno_to_tcti_rc (ret);
    if (rc == TSS2_RC_SUCCESS) {
        g_debug ("tss2_tcti_tabrmd_receive: read returned: %d", ret);
        *size = TSS2_TCTI_TABRMD_HEADER (context).size;
        TSS2_TCTI_TABRMD_STATE (context) = TABRMD_STATE_TRANSMIT;
        g_debug_bytes (response, *size, 16, 4);
    }
    return rc;
}

static void
tss2_tcti_tabrmd_finalize (TSS2_TCTI_CONTEXT *context)
{
    int ret = 0;

    g_debug ("tss2_tcti_tabrmd_finalize");
    if (TSS2_TCTI_TABRMD_PIPE_RECEIVE (context) != 0) {
        ret = close (TSS2_TCTI_TABRMD_PIPE_RECEIVE (context));
        TSS2_TCTI_TABRMD_PIPE_RECEIVE (context) = 0;
    }
    if (ret != 0 && ret != EBADF) {
        g_warning ("Failed to close receive pipe: %s", strerror (errno));
    }
    if (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (context) != 0) {
        ret = close (TSS2_TCTI_TABRMD_PIPE_TRANSMIT (context));
        TSS2_TCTI_TABRMD_PIPE_TRANSMIT (context) = 0;
    }
    if (ret != 0 && ret != EBADF) {
        g_warning ("Failed to close send pipe: %s", strerror (errno));
    }
    TSS2_TCTI_TABRMD_STATE (context) = TABRMD_STATE_FINAL;
    g_object_unref (TSS2_TCTI_TABRMD_PROXY (context));
}

static TSS2_RC
tss2_tcti_tabrmd_cancel (TSS2_TCTI_CONTEXT *context)
{
    TSS2_RC ret = TSS2_RC_SUCCESS;
    GError *error = NULL;
    gboolean cancel_ret;

    g_info("tss2_tcti_tabrmd_cancel: id 0x%" PRIx64,
           TSS2_TCTI_TABRMD_ID (context));
    if (TSS2_TCTI_TABRMD_STATE (context) != TABRMD_STATE_RECEIVE) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    cancel_ret = tcti_tabrmd_call_cancel_sync (TSS2_TCTI_TABRMD_PROXY (context),
                                                      TSS2_TCTI_TABRMD_ID (context),
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
tss2_tcti_tabrmd_get_poll_handles (TSS2_TCTI_CONTEXT *context,
                                 TSS2_TCTI_POLL_HANDLE *handles,
                                 size_t *num_handles)
{
    if (num_handles == NULL) {
        return TSS2_TCTI_RC_BAD_REFERENCE;
    }
    if (handles != NULL && *num_handles < 1) {
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    *num_handles = 1;
    if (handles != NULL) {
        handles [0].fd = TSS2_TCTI_TABRMD_PIPE_RECEIVE (context);
    }
    return TSS2_RC_SUCCESS;
}

static TSS2_RC
tss2_tcti_tabrmd_set_locality (TSS2_TCTI_CONTEXT *context,
                             guint8             locality)
{
    gboolean status;
    TSS2_RC ret;
    GError *error = NULL;

    g_info ("tss2_tcti_tabrmd_set_locality: id 0x%" PRIx64,
            TSS2_TCTI_TABRMD_ID (context));
    if (TSS2_TCTI_TABRMD_STATE (context) != TABRMD_STATE_TRANSMIT) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    status = tcti_tabrmd_call_set_locality_sync (TSS2_TCTI_TABRMD_PROXY (context),
                                                        TSS2_TCTI_TABRMD_ID (context),
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

TSS2_RC
tss2_tcti_tabrmd_dump_trans_state (TSS2_TCTI_CONTEXT *context)
{
    gboolean status;
    TSS2_RC ret;
    GError *error = NULL;

    g_info ("tss2_tcti_tabrmd_dump_state: id 0x%" PRIx64,
            TSS2_TCTI_TABRMD_ID (context));
    status = tcti_tabrmd_call_dump_trans_state_sync (TSS2_TCTI_TABRMD_PROXY (context),
                                                     TSS2_TCTI_TABRMD_ID (context),
                                                     &ret,
                                                     NULL,
                                                     &error);
    if (status == FALSE) {
        g_warning ("dump_trans_state command failed: %s", error->message);
        g_error_free (error);
        return TSS2_TCTI_RC_GENERAL_FAILURE;
    }

    return ret;
}

void
init_function_pointers (TSS2_TCTI_CONTEXT *context)
{
    /* data */
    TSS2_TCTI_MAGIC (context)            = TSS2_TCTI_TABRMD_MAGIC;
    TSS2_TCTI_VERSION (context)          = TSS2_TCTI_TABRMD_VERSION;
    TSS2_TCTI_TABRMD_STATE (context)     = TABRMD_STATE_TRANSMIT;
    /* function pointers */
    TSS2_TCTI_TRANSMIT (context)         = tss2_tcti_tabrmd_transmit;
    TSS2_TCTI_RECEIVE (context)          = tss2_tcti_tabrmd_receive;
    TSS2_TCTI_FINALIZE (context)         = tss2_tcti_tabrmd_finalize;
    TSS2_TCTI_CANCEL (context)           = tss2_tcti_tabrmd_cancel;
    TSS2_TCTI_GET_POLL_HANDLES (context) = tss2_tcti_tabrmd_get_poll_handles;
    TSS2_TCTI_SET_LOCALITY (context)     = tss2_tcti_tabrmd_set_locality;
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
    if (_ret == NULL) {
        goto _out;
    }
    g_variant_get (_ret, "(@aht)", out_fds, out_id);
    g_variant_unref (_ret);
_out:
    return _ret != NULL;
}

TSS2_RC
tss2_tcti_tabrmd_init (TSS2_TCTI_CONTEXT *context,
                       size_t            *size)
{
    GError *error = NULL;
    GVariant *fds_variant;
    guint64 id;
    GUnixFDList *fd_list;
    gboolean call_ret;

    if (context == NULL && size == NULL) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (context == NULL && size != NULL) {
        *size = sizeof (TSS2_TCTI_TABRMD_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    init_function_pointers (context);
    TSS2_TCTI_TABRMD_PROXY (context) =
        tcti_tabrmd_proxy_new_for_bus_sync (
            G_BUS_TYPE_SYSTEM,
            G_DBUS_PROXY_FLAGS_NONE,
            TABRMD_DBUS_NAME,              /* bus name */
            TABRMD_DBUS_PATH, /* object */
            NULL,                          /* GCancellable* */
            &error);
    if (TSS2_TCTI_TABRMD_PROXY (context) == NULL) {
        g_error ("failed to allocate dbus proxy object: %s", error->message);
    }
    call_ret = tcti_tabrmd_call_create_connection_sync_fdlist (
        TSS2_TCTI_TABRMD_PROXY (context),
        &fds_variant,
        &id,
        &fd_list,
        NULL,
        &error);
    if (call_ret == FALSE) {
        g_error ("Failed to create connection with service: %s",
                 error->message);
    }
    if (fd_list == NULL) {
        g_error ("call to CreateConnection returned a NULL GUnixFDList");
    }
    gint num_handles = g_unix_fd_list_get_length (fd_list);
    if (num_handles != 2) {
        g_error ("CreateConnection expected to return 2 handles, received %d",
                 num_handles);
    }
    gint fd = g_unix_fd_list_get (fd_list, 0, &error);
    if (fd == -1) {
        g_error ("unable to get receive handle from GUnixFDList: %s",
                 error->message);
    }
    TSS2_TCTI_TABRMD_PIPE_RECEIVE (context) = fd;
    fd = g_unix_fd_list_get (fd_list, 1, &error);
    if (fd == -1) {
        g_error ("failed to get transmit handle from GUnixFDList: %s",
                 error->message);
    }
    TSS2_TCTI_TABRMD_PIPE_TRANSMIT (context) = fd;
    TSS2_TCTI_TABRMD_ID (context) = id;
    g_debug ("initialized tabrmd TCTI context with id: 0x%" PRIx64,
             TSS2_TCTI_TABRMD_ID (context));

    return TSS2_RC_SUCCESS;
}
