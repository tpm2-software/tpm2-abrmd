/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
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
    GAsyncQueue  *queue;
} MessageQueue;

#define TYPE_MESSAGE_QUEUE           (message_queue_get_type             ())
#define MESSAGE_QUEUE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MESSAGE_QUEUE, MessageQueue))
#define MESSAGE_QUEUE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST    ((cls), TYPE_MESSAGE_QUEUE, MessageQueueClass))
#define IS_MESSAGE_QUEUE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MESSAGE_QUEUE))
#define IS_MESSAGE_QUEUE_CLASS(cls)  (G_TYPE_CHECK_CLASS_TYPE    ((cls), TYPE_MESSAGE_QUEUE))
#define MESSAGE_QUEUE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS  ((obj), TYPE_MESSAGE_QUEUE, MessageQueueClass))

GType           message_queue_get_type     (void);
MessageQueue*   message_queue_new          (void);
void        message_queue_enqueue          (MessageQueue   *message_queue,
                                            GObject        *obj);
GObject*    message_queue_dequeue          (MessageQueue   *message_queue);

G_END_DECLS
#endif /* MESSAGE_QUEUE_H */
