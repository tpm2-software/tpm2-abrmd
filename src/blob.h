#include <glib.h>
#include "session-data.h"

typedef struct blob blob_t;

blob_t*
blob_new          (session_data_t  *session,
                   guint8          *data,
                   size_t           size);
void
blob_free         (blob_t          *blob);
session_data_t*
blob_get_session  (blob_t          *blob);
guint8*
blob_get_data     (blob_t          *blob);
size_t
blob_get_size     (blob_t          *blob);
