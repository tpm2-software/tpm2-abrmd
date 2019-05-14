/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef CONTROL_MESSAGE_H
#define CONTROL_MESSAGE_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Control codes.
 */
typedef enum {
    CHECK_CANCEL    = 1 << 0,
    CONNECTION_REMOVED = 1 << 1,
} ControlCode;

typedef struct _ControlMessageClass {
    GObjectClass     parent;
} ControlMessageClass;

typedef struct _ControlMessage
{
    GObject          parent_instance;
    ControlCode      code;
    GObject         *object;
} ControlMessage;

GType control_message_get_type (void);

#define TYPE_CONTROL_MESSAGE            (control_message_get_type      ())
#define CONTROL_MESSAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_CONTROL_MESSAGE, ControlMessage))
#define CONTROL_MESSAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_CONTROL_MESSAGE, ControlMessageClass))
#define IS_CONTROL_MESSAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_CONTROL_MESSAGE))
#define IS_CONTROL_MESSAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_CONTROL_MESSAGE))
#define CONTROL_MESSAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_CONTROL_MESSAGE, ControlMessageClass))

ControlMessage*    control_message_new    (ControlCode code);
ControlMessage*    control_message_new_with_object (ControlCode code,
                                                    GObject *obj);
ControlCode        control_message_get_code (ControlMessage *msg);
GObject*           control_message_get_object (ControlMessage* msg);

G_END_DECLS
#endif /* CONTROL_MESSAGE_H */
