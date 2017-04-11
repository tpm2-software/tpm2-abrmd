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
#ifndef TCTI_ECHO_H
#define TCTI_ECHO_H

#include <sapi/tpm20.h>
#include <glib-object.h>

#include "tcti.h"

G_BEGIN_DECLS

typedef struct _TctiEchoClass {
    TctiClass      parent;
} TctiEchoClass;

typedef struct _TctiEcho {
    Tcti           parent_instance;
    guint          size;
} TctiEcho;

#define TYPE_TCTI_ECHO             (tcti_echo_get_type         ())
#define TCTI_ECHO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_ECHO, TctiEcho))
#define TCTI_ECHO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_ECHO, TctiEchoClass))
#define IS_TCTI_ECHO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_ECHO))
#define IS_TCTI_ECHO_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_ECHO))
#define TCTI_ECHO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_ECHO, TctiEchoClass))

GType                tcti_echo_get_type       (void);
TctiEcho*            tcti_echo_new            (guint        size);
TSS2_RC              tcti_echo_initialize     (TctiEcho    *tcti);

G_END_DECLS

#endif /* TCTI_ECHO_H */
