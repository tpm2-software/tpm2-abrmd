#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "session-data.h"

static gpointer session_data_parent_class = NULL;

enum {
    PROP_0,
    PROP_ID,
    PROP_RECEIVE_FD,
    PROP_SEND_FD,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };

static void
session_data_set_property (GObject        *object,
                           guint          property_id,
                           const GValue  *value,
                           GParamSpec    *pspec)
{
    SessionData *self = SESSION_DATA (object);

    g_debug ("session_data_set_property");
    switch (property_id) {
    case PROP_ID:
        self->id = g_value_get_uint (value);
        g_debug ("SessionData 0x%x set id to 0x%x", self, self->id);
        break;
    case PROP_RECEIVE_FD:
        self->receive_fd = g_value_get_int (value);
        g_debug ("SessionData 0x%x set receive_fd to 0x%x",
                 self, self->receive_fd);
        break;
    case PROP_SEND_FD:
        self->send_fd = g_value_get_int (value);
        g_debug ("SessionData 0x%x set send_fd to 0x%x", self, self->send_fd);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
session_data_get_property (GObject     *object,
                           guint        property_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
    SessionData *self = SESSION_DATA (object);

    g_debug ("session_data_get_property");
    switch (property_id) {
    case PROP_ID:
        g_value_set_uint (value, self->id);
        break;
    case PROP_RECEIVE_FD:
        g_value_set_int (value, self->receive_fd);
        break;
    case PROP_SEND_FD:
        g_value_set_int (value, self->send_fd);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/* GObject instance initialization. */
static void
session_data_init (GTypeInstance *instance,
                   gpointer       klass)
{
    SessionData *session = SESSION_DATA (instance);

    g_debug ("session_data_init");
    session->handle_map = handle_map_new (TPM_HT_TRANSIENT);
}

static void
session_data_finalize (GObject *obj)
{
    SessionData *session = SESSION_DATA (obj);

    g_debug ("session_data_finalize: 0x%x", session);
    if (session == NULL)
        return;
    close (session->receive_fd);
    close (session->send_fd);
    g_object_unref (session->handle_map);
    if (session_data_parent_class)
        G_OBJECT_CLASS (session_data_parent_class)->finalize (obj);
}

static void
session_data_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("session_data_class_init");
    if (session_data_parent_class == NULL)
        session_data_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = session_data_finalize;
    object_class->get_property = session_data_get_property;
    object_class->set_property = session_data_set_property;

    obj_properties [PROP_ID] =
        g_param_spec_uint ("id",
                           "session identifier",
                           "Unique identifier for the session",
                           0,
                           UINT_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_RECEIVE_FD] =
        g_param_spec_int ("receive_fd",
                          "Receie File Descriptor",
                          "File descriptor for receiving data",
                          0,
                          INT_MAX,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SEND_FD] =
        g_param_spec_int ("send_fd",
                          "Send file descriptor",
                          "File descriptor for sending data",
                          0,
                          INT_MAX,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
GType
session_data_get_type (void)
{
    static GType type = 0;

    g_debug ("session_data_get_type");
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "SessionData",
                                              sizeof (SessionDataClass),
                                              (GClassInitFunc) session_data_class_init,
                                              sizeof (SessionData),
                                              session_data_init,
                                              0);
    }
    return type;
}

/* Create a pipe and return the recv and send fds. */
int
create_pipe_pair (int *recv,
                  int *send,
                  int flags)
{
    int ret, fds[2] = { 0, };

    ret = pipe2 (fds, flags);
    if (ret == -1)
        return ret;
    *recv = fds[0];
    *send = fds[1];
    return 0;
}
/* Create a pipe of receive / send pipe pairs. The parameter integer arrays
 * will both be given a receive / send end of two separate pipes. This allows
 * a sender and receiver to communicate over the two pipes. The flags parameter
 * is the same as passed to the pipe2 system call.
 */
int
create_pipe_pairs (int pipe_fds_a[],
                   int pipe_fds_b[],
                   int flags)
{
    int ret = 0;

    /* b -> a */
    ret = create_pipe_pair (&pipe_fds_a[0], &pipe_fds_b[1], flags);
    if (ret == -1)
        return ret;
    /* a -> b */
    ret = create_pipe_pair (&pipe_fds_b[0], &pipe_fds_a[1], flags);
    if (ret == -1)
        return ret;
    return 0;
}

int
set_flags (const int fd,
           const int flags)
{
    int local_flags, ret;

    local_flags = fcntl(fd, F_GETFL, 0);
    if (!(local_flags && flags)) {
        g_debug ("session_data: setting flags for fd %d to %d",
                 fd, local_flags | flags);
        ret = fcntl(fd, F_SETFL, local_flags | flags);
    }
    return ret;
}

/* CreateConnection builds two pipes for communicating with client
 * applications. It's provided with an array of two integers by the caller
 * and it returns this array populated with the receiving and sending pipe fds
 * respectively.
 */
SessionData*
session_data_new (gint *receive_fd,
                  gint *send_fd,
                  guint64 id)
{

    g_info ("CreateConnection");
    int ret, session_fds[2], client_fds[2];

    g_debug ("session_data_new creating pipe pairs");
    ret = create_pipe_pairs (session_fds, client_fds, O_CLOEXEC);
    if (ret == -1)
        g_error ("CreateConnection failed to make pipe pair %s", strerror (errno));
    /* Make the fds used by the server non-blocking, the client will have to
     * set its own flags.
     */
    ret = set_flags (session_fds [0], O_NONBLOCK);
    if (ret == -1)
        g_error ("Failed to set O_NONBLOCK for server receive fd %d: %s",
                 session_fds [0]);
    ret = set_flags (session_fds [1], O_NONBLOCK);
    if (ret == -1)
        g_error ("Failed to set O_NONBLOCK for server send fd %d: %s",
                 session_fds [1], strerror (errno));
    /* return a receive & send end back for the client */
    *receive_fd = client_fds[0];
    *send_fd = client_fds[1];

    return SESSION_DATA (g_object_new (TYPE_SESSION_DATA,
                                       "id", id,
                                       "receive_fd", session_fds [0],
                                       "send_fd", session_fds [1],
                                       NULL));
}

gpointer
session_data_key_fd (SessionData *session)
{
    return &session->receive_fd;
}

gpointer
session_data_key_id (SessionData *session)
{
    return &session->id;
}

gboolean
session_data_equal_fd (gconstpointer a,
                       gconstpointer b)
{
    return g_int_equal (a, b);
}

gboolean
session_data_equal_id (gconstpointer a,
                       gconstpointer b)
{
    return g_int_equal (a, b);
}
gint
session_data_receive_fd (SessionData *session)
{
    GValue value = G_VALUE_INIT;

    g_value_init (&value, G_TYPE_INT);
    g_object_get_property (G_OBJECT (session),
                           "receive_fd",
                           &value);
    return g_value_get_int (&value);
}
gint
session_data_send_fd (SessionData *session)
{
    GValue value = G_VALUE_INIT;

    g_value_init (&value, G_TYPE_INT);
    g_object_get_property (G_OBJECT (session),
                           "send_fd",
                           &value);
    return g_value_get_int (&value);
}
/*
 * Return a reference to the HandleMap for transient handles to the caller.
 * We increment the reference count on this object before returning it. The
 * caller *must* decrement the reference count when they're done using the
 * object.
 */
HandleMap*
session_data_get_trans_map (SessionData *session)
{
    g_object_ref (session->handle_map);
    return session->handle_map;
}
