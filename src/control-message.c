#include "control-message.h"

G_DEFINE_TYPE (ControlMessage, control_message, G_TYPE_OBJECT);
/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
control_message_init (ControlMessage *obj)
{ /* noop */ }
/* Boiler-plate gobject code.
 */
static void
control_message_class_init (ControlMessageClass *klass)
{
    if (control_message_parent_class == NULL)
        control_message_parent_class = g_type_class_peek_parent (klass);
}
/**
 * Boilerplate constructor.
 */
ControlMessage*
control_message_new (ControlCode code)
{
    ControlMessage *msg;

    msg = CONTROL_MESSAGE (g_object_new (TYPE_CONTROL_MESSAGE, NULL));
    msg->code = code;
    return msg;
}
/*
 * Simple getter to expose the ControlCode in the ControlMessage object.
 */
ControlCode
control_message_get_code (ControlMessage *msg)
{
    return msg->code;
}
