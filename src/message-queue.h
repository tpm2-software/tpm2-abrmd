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
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _MessageQueueClass {
    GObjectClass parent;
} MessageQueueClass;

typedef struct _MessageQueue {
    GObject       parent_instance;
    const gchar  *name;
    GAsyncQueue  *queue;
} MessageQueue;

#define TYPE_MESSAGE_QUEUE           (message_queue_get_type             ())
#define MESSAGE_QUEUE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MESSAGE_QUEUE, MessageQueue))
#define MESSAGE_QUEUE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST    ((cls), TYPE_MESSAGE_QUEUE, MessageQueueClass))
#define IS_MESSAGE_QUEUE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MESSAGE_QUEUE))
#define IS_MESSAGE_QUEUE_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE    ((cls), TYPE_MESSAGE_QUEUE))
#define MESSAGE_QUEUE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS  ((obj), TYPE_MESSAGE_QUEUE, MessageQueueClass))

gpointer    message_queue_new              (const gchar    *name);
void        message_queue_enqueue          (MessageQueue   *message_queue,
                                            GObject        *obj);
GObject*    message_queue_dequeue          (MessageQueue   *message_queue);
GObject*    message_queue_timeout_dequeue  (MessageQueue   *message_queue,
                                            guint64         timeout);

G_END_DECLS
#endif /* MESSAGE_QUEUE_H */
