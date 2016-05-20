#ifndef BLOB_H
#define BLOB_H

#include <glib.h>
#include "session-data.h"

typedef struct blob {
    guint8          *data;
    session_data_t  *session;
    size_t           size;
} blob_t;

blob_t*
blob_new          (session_data_t  *session,
                   guint8          *data,
                   size_t           size);
void
blob_free         (gpointer         blob);
session_data_t*
blob_get_session  (const blob_t    *blob);
guint8*
blob_get_data     (const blob_t    *blob);
size_t
blob_get_size     (const blob_t    *blob);

#endif /* BLOB_H */
