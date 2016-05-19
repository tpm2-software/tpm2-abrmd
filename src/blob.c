#include <errno.h>
#include <glib.h>
#include <stdlib.h>

#include "blob.h"
#include "session-data.h"

/** Create a new blob_t object.
 */
blob_t*
blob_new (session_data_t  *session,
          guint8          *data,
          size_t           size)
{
    blob_t *blob = calloc (1, sizeof (blob_t));
    if (blob == NULL) {
        g_error ("failed to allocate memory for blob_t: %s", strerror (errno));
        return NULL;
    }
    blob->data = data;
    blob->session = session;
    blob->size = size;
    return blob;
}
/** Free a blob_t object.
 * This function frees the blob data member and the blob_t structure. The
 * session_data_t pointer is left unchanged.
 */
void
blob_free (gpointer data)
{
    blob_t *blob = (blob_t*)data;
    if (blob == NULL) {
        g_info ("blob_free: can't free a NULL blob");
        return;
    }
    if (blob->data != NULL)
        free (blob->data);
    blob->data = NULL;
    free (blob);
}
/** Simple accessors.
 */
session_data_t*
blob_get_session (blob_t *blob)
{
    return blob->session;
}
guint8*
blob_get_data (blob_t *blob)
{
    return blob->data;
}
size_t
blob_get_size (blob_t *blob)
{
    return blob->size;
}
