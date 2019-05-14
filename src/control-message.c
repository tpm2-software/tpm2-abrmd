/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include "util.h"
#include "control-message.h"

G_DEFINE_TYPE (ControlMessage, control_message, G_TYPE_OBJECT);
/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
control_message_init (ControlMessage *obj)
{
    UNUSED_PARAM(obj);
    /* noop */
}
/*
 * Dispose of object references and chain up to parent object.
 */
static void
control_message_dispose (GObject *obj)
{
    ControlMessage *msg = CONTROL_MESSAGE (obj);
    g_clear_object (&msg->object);
    G_OBJECT_CLASS (control_message_parent_class)->dispose (obj);
}
/* Boiler-plate gobject code.
 */
static void
control_message_class_init (ControlMessageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (control_message_parent_class == NULL)
        control_message_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose = control_message_dispose;
}
/*
 * Create new ControlMessage with the provided ControlCode and object.
 * Passing NULL in place of the object reference is the same as calling
 * the 'control_message_new' function. The created ControlMessage will take
 * a reference to the provided object.
 */
ControlMessage*
control_message_new_with_object (ControlCode code,
                                 GObject *obj)
{
    ControlMessage *msg;

    if (obj != NULL) {
        g_object_ref (obj);
    }
    msg = CONTROL_MESSAGE (g_object_new (TYPE_CONTROL_MESSAGE, NULL));
    msg->code = code;
    msg->object = obj;

    return msg;
}
/**
 * Boilerplate constructor.
 */
ControlMessage*
control_message_new (ControlCode code)
{
    return control_message_new_with_object (code, NULL);
}
/*
 * Simple getter to expose the ControlCode in the ControlMessage object.
 */
ControlCode
control_message_get_code (ControlMessage *msg)
{
    return msg->code;
}
GObject*
control_message_get_object (ControlMessage *msg)
{
    return msg->object;
}
