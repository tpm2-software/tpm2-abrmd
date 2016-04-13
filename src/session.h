#ifndef SESSION_H
#define SESSION_H

#include <glib.h>

typedef struct session {
    gint receive_fd;
    gint send_fd;
} session_t;

session_t*    session_new      (gint *receive_fd,
                                gint *send_fd);
gboolean      session_equal    (gconstpointer a, gconstpointer b);
gpointer      session_key      (session_t *session);
void          session_free     (session_t *session);

#endif /* SESSION_H */
