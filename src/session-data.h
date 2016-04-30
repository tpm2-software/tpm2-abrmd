#ifndef SESSION_H
#define SESSION_H

#include <glib.h>

typedef struct session_data {
    gint receive_fd;
    gint send_fd;
    guint64 id;
} session_data_t;

session_data_t*  session_data_new    (gint *receive_fd,
                                      gint *send_fd,
                                      guint64 id);
gboolean         session_data_equal_fd  (gconstpointer   a,
                                         gconstpointer   b);
gboolean         session_data_equal_id  (gconstpointer   a,
                                         gconstpointer   b);
gpointer         session_data_key_fd    (session_data_t *session);
gpointer         session_data_key_id    (session_data_t *session);
void             session_data_free   (session_data_t *session);

#endif /* SESSION_H */
