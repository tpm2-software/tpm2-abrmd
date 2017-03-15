#ifndef CONNECTION_H
#define CONNECTION_H

#include <glib.h>
#include <glib-object.h>

#include "handle-map.h"

G_BEGIN_DECLS

typedef struct _ConnectionClass {
    GObjectClass        parent;
} ConnectionClass;

typedef struct _Connection {
    GObject             parent_instance;
    gint                receive_fd;
    gint                send_fd;
    guint64             id;
    HandleMap          *transient_handle_map;
} Connection;

#define TYPE_CONNECTION              (connection_get_type ())
#define CONNECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CONNECTION, Connection))
#define CONNECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST  ((klass), TYPE_CONNECTION, ConnectionClass))
#define IS_CONNECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CONNECTION))
#define IS_CONNECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_CONNECTION))
#define CONNECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_CONNECTION, ConnectionClass))

GType            connection_get_type     (void);
Connection*      connection_new          (gint            *receive_fd,
                                          gint            *send_fd,
                                          guint64          id,
                                          HandleMap       *transient_handle_map);
gboolean         connection_equal_fd     (gconstpointer    a,
                                          gconstpointer    b);
gboolean         connection_equal_id     (gconstpointer    a,
                                          gconstpointer    b);
gpointer         connection_key_fd       (Connection      *session);
gpointer         connection_key_id       (Connection      *session);
gint             connection_receive_fd   (Connection      *session);
gint             connection_send_fd      (Connection      *session);
HandleMap*       connection_get_trans_map(Connection      *session);
/* not part of the public API but included here for testing */
int              create_pipe_pair  (int *recv,
                                    int *send,
                                    int flags);
int              create_pipe_pairs (int pipe_fds_a[],
                                    int pipe_fds_b[],
                                    int flags);

#endif /* CONNECTION_H */
