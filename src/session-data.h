#ifndef SESSION_H
#define SESSION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SessionDataClass {
    GObjectClass        parent;
} SessionDataClass;

typedef struct _SessionData {
    GObject             parent_instance;
    gint                receive_fd;
    gint                send_fd;
    guint64             id;
} SessionData;

#define TYPE_SESSION_DATA              (session_data_get_type ())
#define SESSION_DATA(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),  TYPE_SESSION_DATA, SessionData))
#define SESSION_DATA_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST  ((klass), TYPE_SESSION_DATA, SessionDataClass))
#define IS_SESSION_DATA(obj)           (G_TYPE_CHECK_INSTANCE_TYPE  ((obj), TYPE_SESSION_DATA))
#define IS_SESSION_DATA_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SESSION_DATA))
#define SESSION_DATA_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SESSION_DATA, SessionDataClass))

GType            session_data_get_type     (void);
SessionData*     session_data_new          (gint             *receive_fd,
                                            gint             *send_fd,
                                            guint64           id);
gboolean         session_data_equal_fd     (gconstpointer     a,
                                            gconstpointer     b);
gboolean         session_data_equal_id     (gconstpointer     a,
                                            gconstpointer     b);
gpointer         session_data_key_fd       (SessionData      *session);
gpointer         session_data_key_id       (SessionData      *session);
gint             session_data_receive_fd   (SessionData      *session);
gint             session_data_send_fd      (SessionData      *session);
void             session_data_free         (SessionData      *session);

#endif /* SESSION_H */
