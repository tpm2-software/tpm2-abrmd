#ifndef TABD_TCTI_SOCKET_H
#define TABD_TCTI_SOCKET_H

#include <arpa/inet.h>
#include <sapi/tpm20.h>
#include <tcti/tcti_socket.h>
#include <glib-object.h>

#include "tcti-interface.h"

G_BEGIN_DECLS

typedef struct _TctiSocketClass {
    GObjectClass parent;
} TctiSocketClass;

typedef struct _TctiSocket
{
    GObject            parent_instance;
    gchar             *address;
    guint16            port;
} TctiSocket;

#define TYPE_TCTI_SOCKET             (tcti_socket_get_type       ())
#define TCTI_SOCKET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_SOCKET, TctiSocket))
#define TCTI_SOCKET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_SOCKET, TctiSocketClass))
#define IS_TCTI_SOCKET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_SOCKET))
#define IS_TCTI_SOCKET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_SOCKET))
#define TCTI_SOCKET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_SOCKET, TctiSocketClass))

GType                tcti_socket_get_type       (void);
TctiSocket*          tcti_socket_new            (gchar const   *address,
                                                 guint16        port);
TSS2_RC              tcti_socket_initialize     (Tcti          *tcti);

G_END_DECLS

#endif /* TABD_TCTI_SOCKET_H */
