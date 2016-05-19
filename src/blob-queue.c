#include <errno.h>
#include <glib.h>
#include <stdlib.h>

#include "blob-queue.h"

struct blob_queue {
    const gchar *name;
    GAsyncQueue *queue;
};

/** Allocate a new blob_queue_t object.
 * The caller owns the returned pointer and must free it.
 */
blob_queue_t*
blob_queue_new (const char *name)
{
    blob_queue_t *blob_queue;
    
    blob_queue = calloc (1, sizeof (blob_queue_t));
    if (blob_queue == NULL) {
        g_error ("failed to allocate memory for blob_queue_t: %s", strerror (errno));
        return NULL;
    }
    blob_queue->name = name;
    blob_queue->queue = g_async_queue_new_full (blob_free);

    return blob_queue;
}
/** Enqueue a blob in the blob_queue_t.
 * This function is a thin wrapper around the GQueue. When we enqueue blobs
 * we push them to the head of the queue.
 */
void
blob_queue_enqueue       (blob_queue_t    *blob_queue,
                          blob_t          *blob)
{
    g_async_queue_push (blob_queue->queue, blob);
}
/** Dequeue a blob from the blob_queue_t.
 * This function is a thin wrapper around the GQueue. When we dequeue blobs
 * we pop them from the tail of the queue.
 */
blob_t*
blob_queue_dequeue       (blob_queue_t    *blob_queue)
{
    blob_t *blob;

    if (blob_queue == NULL)
        g_error ("blob_queue_dequeue passed NULL blob_queue_t");
    /** This bit of ref / unref magic is to cope with a specific edge case:
     * If there's a thread blocked waiting for something to pop off of this
     * queue when the blob-queue is freed then the refcount on the queue will
     * reach 0 while there's still a thread interacting with the GAsyncQueue.
     * This whole situation is our fault since we're not implementing ref
     * counted gobjects. Hack around this now by wrapping the blocking call in
     * ref/unref calls.
     */
    g_async_queue_ref (blob_queue->queue);
    blob = g_async_queue_pop (blob_queue->queue);
    g_async_queue_unref (blob_queue->queue);

    return blob;
}
/** Free all blob_t objects in the blob_queue.
 */
void
blob_queue_free (blob_queue_t *blob_queue)
{
    if (blob_queue == NULL)
        g_error ("blob_queue_free passed NULL blob_queue_t");
    g_async_queue_unref (blob_queue->queue);
    free (blob_queue);
}
blob_t*
blob_queue_timeout_dequeue (blob_queue_t *blob_queue,
                            guint64       timeout)
{
    blob_t *blob;

    /* Same situation as in blob_queue_dequeue above */
    g_async_queue_ref (blob_queue->queue);
    blob = g_async_queue_timeout_pop (blob_queue->queue, timeout);
    g_async_queue_unref (blob_queue->queue);

    return blob;

}
