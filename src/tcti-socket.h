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
#ifndef TABD_TCTI_SOCKET_H
#define TABD_TCTI_SOCKET_H

#include <arpa/inet.h>
#include <sapi/tpm20.h>
#include <tcti/tcti_socket.h>
#include <glib-object.h>

#include "tcti.h"

G_BEGIN_DECLS

#define TCTI_SOCKET_DEFAULT_HOST "127.0.0.1"
#define TCTI_SOCKET_DEFAULT_PORT 2321

typedef struct _TctiSocketClass {
    TctiClass          parent;
} TctiSocketClass;

typedef struct _TctiSocket
{
    Tcti               parent_instance;
    gchar             *address;
    guint              port;
} TctiSocket;

#define TYPE_TCTI_SOCKET             (tcti_socket_get_type       ())
#define TCTI_SOCKET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_SOCKET, TctiSocket))
#define TCTI_SOCKET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_SOCKET, TctiSocketClass))
#define IS_TCTI_SOCKET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_SOCKET))
#define IS_TCTI_SOCKET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_SOCKET))
#define TCTI_SOCKET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_SOCKET, TctiSocketClass))

GType                tcti_socket_get_type       (void);
TctiSocket*          tcti_socket_new            (gchar const   *address,
                                                 guint          port);
TSS2_RC              tcti_socket_initialize     (TctiSocket    *tcti);

G_END_DECLS
#endif /* TABD_TCTI_SOCKET_H */
