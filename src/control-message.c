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
