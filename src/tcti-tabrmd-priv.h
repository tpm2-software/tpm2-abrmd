/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
#ifndef TSS2TCTI_TABRMD_PRIV_H
#define TSS2TCTI_TABRMD_PRIV_H

#include <glib.h>
#include <pthread.h>
#include <tss2/tss2_tcti.h>

#include "tabrmd-generated.h"
#include "tpm2-header.h"

#define TSS2_TCTI_TABRMD_MAGIC 0x1c8e03ff00db0f92
#define TSS2_TCTI_TABRMD_VERSION 1

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
 *     receieve:    produces TSS2_TCTI_RC_BAD_SEQUENCE
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

typedef struct {
    TCTI_TABRMD_DBUS_TYPE bus_type;
    const char *bus_name;
} tabrmd_conf_t;

/*
 * This function is not exposed through the public header. It is meant to be
 * dynamically discovered using dlopen at run time. We include it here in the
 * private header so that we can invoke it in the test harness.
 */
const TSS2_TCTI_INFO* Tss2_Tcti_Info (void);
TCTI_TABRMD_DBUS_TYPE tabrmd_bus_type_from_str (const char* const bus_type);
TSS2_RC tabrmd_conf_parse_kv (const char *key,
                              const char *value,
                              tabrmd_conf_t * const tabrmd_conf);
TSS2_RC tabrmd_conf_parse (char *conf_str,
                           tabrmd_conf_t * const tabrmd_conf);

#endif /* TSS2TCTI_TABRMD_PRIV_H */
