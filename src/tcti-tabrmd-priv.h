/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TSS2TCTI_TABRMD_PRIV_H
#define TSS2TCTI_TABRMD_PRIV_H

#include <glib.h>
#include <pthread.h>
#include <tss2/tss2_tcti.h>

#include "tabrmd-defaults.h"
#include "tabrmd-generated.h"
#include "tpm2-header.h"
#include "util.h"

#define TSS2_TCTI_TABRMD_MAGIC 0x1c8e03ff00db0f92
#define TSS2_TCTI_TABRMD_VERSION 2

#define TSS2_TCTI_TABRMD_ID(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->id
#define TSS2_TCTI_TABRMD_IOSTREAM(context) \
    G_IO_STREAM (TSS2_TCTI_TABRMD_SOCK_CONNECT (context))
#define TSS2_TCTI_TABRMD_PROXY(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->proxy
#define TSS2_TCTI_TABRMD_HEADER(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->header
#define TSS2_TCTI_TABRMD_STATE(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->state

/*
 * Macros for accessing the internals of the I/O stream. These are helpers
 * for getting at the underlying GSocket and raw fds that we need to
 * implement polling etc.
 */
#define TSS2_TCTI_TABRMD_ISTREAM(context) \
    g_io_stream_get_input_stream (TSS2_TCTI_TABRMD_IOSTREAM(context))
#define TSS2_TCTI_TABRMD_SOCK_CONNECT(context) \
    ((TSS2_TCTI_TABRMD_CONTEXT*)context)->sock_connect
#define TSS2_TCTI_TABRMD_SOCKET(context) \
    g_socket_connection_get_socket (TSS2_TCTI_TABRMD_SOCK_CONNECT (context))
#define TSS2_TCTI_TABRMD_FD(context) \
    g_socket_get_fd (TSS2_TCTI_TABRMD_SOCKET (context))

/*
 * The elements in this enumeration represent the possible states that the
 * tabrmd TCTI can be in. The state machine is as follows:
 * An instantiated TCTI context begins in the TRANSMIT state:
 *   TRANSMIT:
 *     transmit:    success transitions the state machine to RECEIVE
 *                  failure leaves the state unchanged
 *     receive:     produces TSS2_TCTI_RC_BAD_SEQUENCE
 *     finalize:    transitions state machine to FINAL state
 *     cancel:      produces TSS2_TCTI_RC_BAD_SEQUENCE
 *     setLocality: success or failure leaves state unchanged
 *   RECEIVE:
 *     transmit:    produces TSS2_TCTI_RC_BAD_SEQUENCE
 *     receive:     success transitions the state machine to TRANSMIT
 *                  failure with the following RCs leave the state unchanged:
 *                    TRY_AGAIN, INSUFFICIENT_BUFFER, BAD_CONTEXT,
 *                    BAD_REFERENCE, BAD_VALUE, BAD_SEQUENCE
 *                  all other failures transition state machine to
 *                    TRANSMIT (not recoverable)
 *     finalize:    transitions state machine to FINAL state
 *     cancel:      success transitions state machine to READY_TRANSMIT
 *                  failure leaves state unchanged
 *     setLocality: produces TSS2_TCTI_RC_BAD_SEQUENCE
 *   FINAL:
 *     all function calls produce TSS2_TCTI_RC_BAD_SEQUENCE
 */
typedef enum {
    TABRMD_STATE_FINAL,
    TABRMD_STATE_RECEIVE,
    TABRMD_STATE_TRANSMIT,
} tcti_tabrmd_state_t;

/* This is our private TCTI structure. We're required by the spec to have
 * the same structure as the non-opaque area defined by the
 * TSS2_TCTI_CONTEXT_COMMON_V1 structure. Anything after this data is opaque
 * and private to our implementation. See section 7.3 of the SAPI / TCTI spec
 * for the details.
 */
typedef struct {
    TSS2_TCTI_CONTEXT_COMMON_V1    common;
    guint64                        id;
    GSocketConnection             *sock_connect;
    TctiTabrmd                    *proxy;
    tpm_header_t                   header;
    tcti_tabrmd_state_t            state;
    size_t                         index;
    uint8_t                        header_buf [TPM_HEADER_SIZE];
} TSS2_TCTI_TABRMD_CONTEXT;

#define TABRMD_CONF_INIT_DEFAULT { \
    .bus_name = TABRMD_DBUS_NAME_DEFAULT, \
    .bus_type = TABRMD_DBUS_TYPE_DEFAULT, \
}

typedef struct {
    const char *bus_name;
    GBusType bus_type;
} tabrmd_conf_t;

/*
 * This function is not exposed through the public header. It is meant to be
 * dynamically discovered using dlopen at run time. We include it here in the
 * private header so that we can invoke it in the test harness.
 */
const TSS2_TCTI_INFO* Tss2_Tcti_Info (void);
GBusType tabrmd_bus_type_from_str (const char* const bus_type);
TSS2_RC tabrmd_kv_callback (const key_value_t *key_value,
                            gpointer user_data);
TSS2_RC tss2_tcti_tabrmd_transmit (TSS2_TCTI_CONTEXT *context,
                                   size_t size,
                                   const uint8_t *command);
TSS2_RC tss2_tcti_tabrmd_receive (TSS2_TCTI_CONTEXT *context,
                                  size_t *size,
                                  uint8_t *response,
                                  int32_t timeout);
void tss2_tcti_tabrmd_finalize (TSS2_TCTI_CONTEXT *context);
TSS2_RC tss2_tcti_tabrmd_cancel (TSS2_TCTI_CONTEXT *context);
TSS2_RC tss2_tcti_tabrmd_get_poll_handles (TSS2_TCTI_CONTEXT *context,
                                           TSS2_TCTI_POLL_HANDLE *handles,
                                           size_t *num_handles);
TSS2_RC tss2_tcti_tabrmd_set_locality (TSS2_TCTI_CONTEXT *context,
                                       guint8 locality);
int tcti_tabrmd_poll (int fd, int32_t timeout);
TSS2_RC tcti_tabrmd_read (TSS2_TCTI_TABRMD_CONTEXT *ctx,
                          uint8_t *buf,
                          size_t size,
                          int32_t timeout);

#endif /* TSS2TCTI_TABRMD_PRIV_H */
