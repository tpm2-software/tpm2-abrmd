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
#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "message-queue.h"

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
    g_debug ("message_queue_enqueue 0x%" PRIxPTR " : message 0x%" PRIxPTR,
             (uintptr_t)message_queue, (uintptr_t)object);
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
    g_debug ("message_queue_dequeue 0x%" PRIxPTR, (uintptr_t)message_queue);
    obj = g_async_queue_pop (message_queue->queue);
    g_debug ("  got obj: 0x%" PRIxPTR, (uintptr_t)obj);
    return obj;
}
