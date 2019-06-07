/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "message-queue.h"
#include "util.h"

G_DEFINE_TYPE (MessageQueue, message_queue, G_TYPE_OBJECT);

/**
 * The init function is a noop but it's required by the G_DEFINE_TYPE
 * macro.
 */
static void
message_queue_init (MessageQueue *self)
{
    self->queue = g_async_queue_new_full (g_object_unref);
}
/*
 * To finalize the MessageQueue we need only to unref the internal
 * GAsyncQueue object.
 */
static void
message_queue_dispose (GObject *obj)
{
    MessageQueue *message_queue = MESSAGE_QUEUE (obj);

    g_clear_pointer (&message_queue->queue, g_async_queue_unref);
    G_OBJECT_CLASS (message_queue_parent_class)->dispose (obj);
}
/**
 * Boilerplate GObject class init with custom dispose function.
 */
static void
message_queue_class_init (MessageQueueClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = message_queue_dispose;
}
/**
 * Allocate a new message_queue_t object.
 * The caller owns the returned object reference and must unref it.
 * When the MessageQueue object is destroyed each object in its internal
 * queue will be unref'd as well.
 */
MessageQueue*
message_queue_new ()
{
    return MESSAGE_QUEUE (g_object_new (TYPE_MESSAGE_QUEUE, NULL));
}
/**
 * Enqueue a blob in the blob_queue_t.
 * This function is a thin wrapper around the GQueue. When we enqueue blobs
 * we push them to the head of the queue.
 */
void
message_queue_enqueue (MessageQueue  *message_queue,
                       GObject       *object)
{
    g_assert (message_queue != NULL);
    g_debug ("%s", __func__);
    g_object_ref (object);
    g_async_queue_push (message_queue->queue, object);
}
/**
 * Dequeue a blob from the blob_queue_t.
 * This function is a thin wrapper around the GQueue. When we dequeue blobs
 * we pop them from the tail of the queue.
 */
GObject*
message_queue_dequeue (MessageQueue *message_queue)
{
    GObject *obj;

    g_assert (message_queue != NULL);
    g_debug ("%s", __func__);
    obj = g_async_queue_pop (message_queue->queue);
    return obj;
}
