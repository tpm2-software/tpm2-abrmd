#ifndef BLOB_QUEUE_H
#define BLOB_QUEUE_H

#include <glib.h>

#include "blob.h"
#include "session-data.h"

typedef struct blob_queue blob_queue_t;

blob_queue_t*
blob_queue_new           (const gchar     *name);
void
blob_queue_enqueue       (blob_queue_t    *blob_queue,
                          blob_t          *blob);
blob_t*
blob_queue_dequeue       (blob_queue_t    *blob_queue);
void
blob_queue_free          (blob_queue_t    *blob_queue);

#endif /* BLOB_QUEUE_H */
