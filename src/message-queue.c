#include <errno.h>
#include <glib.h>
#include <stdlib.h>

#include "message-queue.h"

G_DEFINE_TYPE (MessageQueue, message_queue, G_TYPE_OBJECT);

static void
message_queue_init (MessageQueue *self) {}
static void
message_queue_dispose (GObject *obj)
{
    MessageQueue *message_queue = MESSAGE_QUEUE (obj);

    g_clear_pointer (&message_queue->queue, g_async_queue_unref);
}
static void
message_queue_class_init (MessageQueueClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = message_queue_dispose;
}
/** Allocate a new message_queue_t object.
 * The caller owns the returned pointer and must free it.
 */
gpointer
message_queue_new (const char *name)
{
    MessageQueue *message_queue;

    message_queue = g_object_new (TYPE_MESSAGE_QUEUE, NULL);
    if (message_queue == NULL) {
        g_error ("failed to allocate memory for blob_queue_t: %s", strerror (errno));
        return NULL;
    }
    message_queue->name = name;
    message_queue->queue = g_async_queue_new_full (g_object_unref);

    return message_queue;
}
/** Enqueue a blob in the blob_queue_t.
 * This function is a thin wrapper around the GQueue. When we enqueue blobs
 * we push them to the head of the queue.
 */
void
message_queue_enqueue (MessageQueue  *message_queue,
                       GObject       *object)
{
    g_assert (message_queue != NULL);
    g_debug ("message_queue_enqueue 0x%x: message 0x%x", message_queue, object);
    g_async_queue_push (message_queue->queue, object);
}
/** Dequeue a blob from the blob_queue_t.
 * This function is a thin wrapper around the GQueue. When we dequeue blobs
 * we pop them from the tail of the queue.
 */
GObject*
message_queue_dequeue (MessageQueue *message_queue)
{
    GObject *obj;

    g_assert (message_queue != NULL);
    g_debug ("message_queue_dequeue 0x%x", message_queue);
    obj = g_async_queue_pop (message_queue->queue);
    g_debug ("  got obj: 0x%x", obj);
    return obj;
}
GObject*
message_queue_timeout_dequeue (MessageQueue *message_queue,
                               guint64       timeout)
{
    GObject *obj;

    g_assert (message_queue != NULL);
    g_debug ("message_queue_timeout_dequeue 0x%x", message_queue);
    obj = g_async_queue_timeout_pop (message_queue->queue, timeout);
    g_debug ("  got obj: 0x%x", obj);
    return obj;
}
