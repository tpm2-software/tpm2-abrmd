#include <glib.h>

typedef struct session {
    gint receive_fd;
    gint send_fd;
} session_t;

session_t*    session_new     (void);
gboolean      session_equal   (gconstpointer a, gconstpointer b);
gconstpointer session_key (session_t *session);
void          session_free    (session_t *session);
