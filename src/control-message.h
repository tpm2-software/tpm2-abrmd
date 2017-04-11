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
#ifndef CONTROL_MESSAGE_H
#define CONTROL_MESSAGE_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Control codes.
 */
typedef enum {
    CHECK_CANCEL    = 1 << 0,
} ControlCode;

typedef struct _ControlMessageClass {
    GObjectClass     parent;
} ControlMessageClass;

typedef struct _ControlMessage
{
    GObject          parent_instance;
    ControlCode      code;
} ControlMessage;

GType control_message_get_type (void);

#define TYPE_CONTROL_MESSAGE            (control_message_get_type      ())
#define CONTROL_MESSAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_CONTROL_MESSAGE, ControlMessage))
#define CONTROL_MESSAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_CONTROL_MESSAGE, ControlMessageClass))
#define IS_CONTROL_MESSAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_CONTROL_MESSAGE))
#define IS_CONTROL_MESSAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_CONTROL_MESSAGE))
#define CONTROL_MESSAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_CONTROL_MESSAGE, ControlMessageClass))

ControlMessage*    control_message_new    (ControlCode code);
ControlCode        control_message_get_code (ControlMessage *msg);

G_END_DECLS
#endif /* CONTROL_MESSAGE_H */
