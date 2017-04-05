#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"

G_DEFINE_TYPE (Connection, connection, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_ID,
    PROP_RECEIVE_FD,
    PROP_SEND_FD,
    PROP_TRANSIENT_HANDLE_MAP,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };

static void
connection_set_property (GObject       *object,
                         guint          property_id,
                         const GValue  *value,
                         GParamSpec    *pspec)
{
    Connection *self = CONNECTION (object);

    g_debug ("connection_set_property");
    switch (property_id) {
    case PROP_ID:
        self->id = g_value_get_uint64 (value);
        g_debug ("Connection 0x%" PRIxPTR " set id to 0x%" PRIx64,
                 (uintptr_t)self, self->id);
        break;
    case PROP_RECEIVE_FD:
        self->receive_fd = g_value_get_int (value);
        g_debug ("Connection 0x%" PRIxPTR " set receive_fd to %d",
                 (uintptr_t)self, self->receive_fd);
        break;
    case PROP_SEND_FD:
        self->send_fd = g_value_get_int (value);
        g_debug ("Connection 0x%" PRIxPTR " set send_fd to %d",
                 (uintptr_t)self, self->send_fd);
        break;
    case PROP_TRANSIENT_HANDLE_MAP:
        self->transient_handle_map = g_value_get_object (value);
        g_object_ref (self->transient_handle_map);
        g_debug ("Connection 0x%" PRIxPTR " set trans_handel_map to 0x%"
                  PRIxPTR, (uintptr_t)self,
                  (uintptr_t)self->transient_handle_map);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
connection_get_property (GObject     *object,
                         guint        property_id,
                         GValue      *value,
                         GParamSpec  *pspec)
{
    Connection *self = CONNECTION (object);

    g_debug ("connection_get_property");
    switch (property_id) {
    case PROP_ID:
        g_value_set_uint64 (value, self->id);
        break;
    case PROP_RECEIVE_FD:
        g_value_set_int (value, self->receive_fd);
        break;
    case PROP_SEND_FD:
        g_value_set_int (value, self->send_fd);
        break;
    case PROP_TRANSIENT_HANDLE_MAP:
        g_value_set_object (value, self->transient_handle_map);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
connection_init (Connection *connection)
{ /* noop */ }

static void
connection_finalize (GObject *obj)
{
    Connection *connection = CONNECTION (obj);

    g_debug ("connection_finalize: 0x%" PRIxPTR, (uintptr_t)connection);
    if (connection == NULL)
        return;
    close (connection->receive_fd);
    close (connection->send_fd);
    g_object_unref (connection->transient_handle_map);
    if (connection_parent_class)
        G_OBJECT_CLASS (connection_parent_class)->finalize (obj);
}

static void
connection_class_init (ConnectionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("connection_class_init");
    if (connection_parent_class == NULL)
        connection_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = connection_finalize;
    object_class->get_property = connection_get_property;
    object_class->set_property = connection_set_property;

    obj_properties [PROP_ID] =
        g_param_spec_uint64 ("id",
                             "connection identifier",
                             "Unique identifier for the connection",
                             0,
                             UINT64_MAX,
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
    obj_properties [PROP_TRANSIENT_HANDLE_MAP] =
        g_param_spec_object ("transient-handle-map",
                             "HandleMap",
                             "HandleMap object to map handles to transient object contexts",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
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
    int local_flags, ret = 0;

    local_flags = fcntl(fd, F_GETFL, 0);
    if (!(local_flags && flags)) {
        g_debug ("connection: setting flags for fd %d to %d",
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
Connection*
connection_new (gint       *receive_fd,
                gint       *send_fd,
                guint64     id,
                HandleMap  *transient_handle_map)
{

    g_info ("CreateConnection");
    int ret, connection_fds[2], client_fds[2];

    g_debug ("connection_new creating pipe pairs");
    ret = create_pipe_pairs (connection_fds, client_fds, O_CLOEXEC);
    if (ret == -1)
        g_error ("CreateConnection failed to make pipe pair %s", strerror (errno));
    /* Make the fds used by the server non-blocking, the client will have to
     * set its own flags.
     */
    ret = set_flags (connection_fds [0], O_NONBLOCK);
    if (ret == -1)
        g_error ("Failed to set O_NONBLOCK for server receive fd %d: %s",
                 connection_fds [0], strerror (errno));
    ret = set_flags (connection_fds [1], O_NONBLOCK);
    if (ret == -1)
        g_error ("Failed to set O_NONBLOCK for server send fd %d: %s",
                 connection_fds [1], strerror (errno));
    /* return a receive & send end back for the client */
    *receive_fd = client_fds[0];
    *send_fd = client_fds[1];

    return CONNECTION (g_object_new (TYPE_CONNECTION,
                                     "id", id,
                                     "receive_fd", connection_fds [0],
                                     "send_fd", connection_fds [1],
                                     "transient-handle-map", transient_handle_map,
                                     NULL));
}

gpointer
connection_key_fd (Connection *connection)
{
    return &connection->receive_fd;
}

gpointer
connection_key_id (Connection *connection)
{
    return &connection->id;
}

gboolean
connection_equal_fd (gconstpointer a,
                       gconstpointer b)
{
    return g_int_equal (a, b);
}

gboolean
connection_equal_id (gconstpointer a,
                       gconstpointer b)
{
    return g_int_equal (a, b);
}
gint
connection_receive_fd (Connection *connection)
{
    GValue value = G_VALUE_INIT;

    g_value_init (&value, G_TYPE_INT);
    g_object_get_property (G_OBJECT (connection),
                           "receive_fd",
                           &value);
    return g_value_get_int (&value);
}
gint
connection_send_fd (Connection *connection)
{
    GValue value = G_VALUE_INIT;

    g_value_init (&value, G_TYPE_INT);
    g_object_get_property (G_OBJECT (connection),
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
connection_get_trans_map (Connection *connection)
{
    g_object_ref (connection->transient_handle_map);
    return connection->transient_handle_map;
}
