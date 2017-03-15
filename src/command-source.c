#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connection-manager.h"
#include "command-source.h"
#include "source-interface.h"
#include "tpm2-command.h"
#include "thread-interface.h"
#include "util.h"

#define WAKEUP_DATA "hi"
#define WAKEUP_SIZE 2

static gpointer command_source_parent_class = NULL;

enum {
    PROP_0,
    PROP_COMMAND_ATTRS,
    PROP_CONNECTION_MANAGER,
    PROP_SINK,
    PROP_WAKEUP_RECEIVE_FD,
    PROP_WAKEUP_SEND_FD,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * Forward declare the functions for the ThreadInterface.
 */
gint command_source_cancel (Thread *self);
gint command_source_join   (Thread *self);
gint command_source_start  (Thread *self);
/**
 * Function implementing the Source interface. Adds a sink for the source
 * to pass data to.
 */
void
command_source_add_sink (Source      *self,
                         Sink        *sink)
{
    CommandSource *src = COMMAND_SOURCE (self);
    GValue value = G_VALUE_INIT;

    g_debug ("command_soruce_add_sink: CommandSource: 0x%" PRIxPTR
             " , Sink: 0x%" PRIxPTR, (uintptr_t)src, (uintptr_t)sink);
    g_value_init (&value, G_TYPE_OBJECT);
    g_value_set_object (&value, sink);
    g_object_set_property (G_OBJECT (src), "sink", &value);
    g_value_unset (&value);
}

static void
command_source_set_property (GObject       *object,
                              guint          property_id,
                              GValue const  *value,
                              GParamSpec    *pspec)
{
    CommandSource *self = COMMAND_SOURCE (object);

    g_debug ("command_source_set_properties: 0x%" PRIxPTR, (uintptr_t)self);
    switch (property_id) {
    case PROP_COMMAND_ATTRS:
        self->command_attrs = COMMAND_ATTRS (g_value_dup_object (value));
        g_debug ("  command_attrs: 0x%" PRIxPTR, (uintptr_t)self->command_attrs);
        break;
    case PROP_CONNECTION_MANAGER:
        self->connection_manager = CONNECTION_MANAGER (g_value_get_object (value));
        break;
    case PROP_SINK:
        /* be rigid intially, add flexiblity later if we need it */
        if (self->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        self->sink = SINK (g_value_get_object (value));
        g_object_ref (self->sink);
        g_debug ("  sink: 0x%" PRIxPTR, (uintptr_t)self->sink);
        break;
    case PROP_WAKEUP_RECEIVE_FD:
        self->wakeup_receive_fd = g_value_get_int (value);
        g_debug ("  wakeup_receive_fd: %d", self->wakeup_receive_fd);
        break;
    case PROP_WAKEUP_SEND_FD:
        self->wakeup_send_fd = g_value_get_int (value);
        g_debug ("  wakeup_send_fd: %d", self->wakeup_send_fd);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
command_source_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
    CommandSource *self = COMMAND_SOURCE (object);

    g_debug ("command_source_get_properties: 0x%" PRIxPTR, (uintptr_t)self);
    switch (property_id) {
    case PROP_COMMAND_ATTRS:
        g_value_set_object (value, self->command_attrs);
        break;
    case PROP_CONNECTION_MANAGER:
        g_value_set_object (value, self->connection_manager);
        break;
    case PROP_SINK:
        g_value_set_object (value, self->sink);
        break;
    case PROP_WAKEUP_RECEIVE_FD:
        g_value_set_int (value, self->wakeup_receive_fd);
        break;
    case PROP_WAKEUP_SEND_FD:
        g_value_set_int (value, self->wakeup_send_fd);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
gint
command_source_on_new_connection (ConnectionManager   *connection_manager,
                                  Connection          *connection,
                                  CommandSource       *command_source)
{
    ssize_t ret;

    g_debug ("command_source_on_new_connection: writing \"%s\" to fd: %d",
             WAKEUP_DATA, command_source->wakeup_send_fd);
    ret = write (command_source->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    if (ret < 1)
        g_error ("failed to send wakeup signal");

    return 0;
}

/* Not doing any sanity checks here. Be sure to shut things downon your own
 * first.
 */
static void
command_source_finalize (GObject  *object)
{
    CommandSource *source = COMMAND_SOURCE (object);

    if (source->sink)
        g_object_unref (source->sink);
    if (source->connection_manager)
        g_object_unref (source->connection_manager);
    if (source->command_attrs) {
        g_object_unref (source->command_attrs);
        source->command_attrs = NULL;
    }
    close (source->wakeup_send_fd);
    close (source->wakeup_receive_fd);
    if (command_source_parent_class)
        G_OBJECT_CLASS (command_source_parent_class)->finalize (object);
}

static void
command_source_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("command_source_class_init");
    if (command_source_parent_class == NULL)
        command_source_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = command_source_finalize;
    object_class->get_property = command_source_get_property;
    object_class->set_property = command_source_set_property;

    obj_properties [PROP_COMMAND_ATTRS] =
        g_param_spec_object ("command-attrs",
                             "CommandAttrs object",
                             "CommandAttrs instance.",
                             TYPE_COMMAND_ATTRS,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_CONNECTION_MANAGER] =
        g_param_spec_object ("connection-manager",
                             "ConnectionManager object",
                             "ConnectionManager instance.",
                             TYPE_CONNECTION_MANAGER,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SINK] =
        g_param_spec_object ("sink",
                             "Sink",
                             "Reference to a Sink object.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE);
    obj_properties [PROP_WAKEUP_RECEIVE_FD] =
        g_param_spec_int ("wakeup-receive-fd",
                          "wakeup receive file descriptor",
                          "File descriptor to receive wakeup signal.",
                          0,
                          INT_MAX,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_WAKEUP_SEND_FD] =
        g_param_spec_int ("wakeup-send-fd",
                          "wakeup send file descriptor",
                          "File descriptor to send wakeup signal.",
                          0,
                          INT_MAX,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 */
static void
command_source_thread_interface_init (gpointer g_iface)
{
    ThreadInterface *thread = (ThreadInterface*)g_iface;
    thread->cancel = command_source_cancel;
    thread->join   = command_source_join;
    thread->start  = command_source_start;
}
static void
command_source_source_interface_init (gpointer g_iface)
{
    SourceInterface *source = (SourceInterface*)g_iface;
    source->add_sink = command_source_add_sink;
}
GType
command_source_get_type (void)
{
    static GType type = 0;

    g_debug ("command_source_get_type");
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "CommandSource",
                                              sizeof (CommandSourceClass),
                                              (GClassInitFunc) command_source_class_init,
                                              sizeof (CommandSource),
                                              NULL,
                                              0);
        const GInterfaceInfo interface_info_thread = {
            (GInterfaceInitFunc) command_source_thread_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_THREAD, &interface_info_thread);

        const GInterfaceInfo interface_info_source = {
            (GInterfaceInitFunc) command_source_source_interface_init,
            NULL,
            NULL,
        };
        g_type_add_interface_static (type, TYPE_SOURCE, &interface_info_source);
    }
    return type;
}
void
command_source_thread_cleanup (void *data)
{
    CommandSource *source = COMMAND_SOURCE (data);

    source->running = FALSE;
}

gboolean
command_source_connection_responder (CommandSource      *source,
                                     gint                fd,
                                     Sink               *sink)
{
    Tpm2Command *command;
    Connection  *connection;

    g_debug ("command_source_connection_responder 0x%" PRIxPTR, (uintptr_t)source);
    connection = connection_manager_lookup_fd (source->connection_manager, fd);
    if (connection == NULL)
        g_error ("failed to get connection associated with fd: %d", fd);
    else
        g_debug ("connection_manager_lookup_fd for fd %d: 0x%" PRIxPTR, fd,
                 (uintptr_t)connection);
    command = tpm2_command_new_from_fd (connection, fd, source->command_attrs);
    if (command != NULL) {
        sink_enqueue (sink, G_OBJECT (command));
        /* the sink now owns this message */
        g_object_unref (command);
    } else {
        /* command will be NULL when read error on fd, or fd is closed (EOF)
         * In either case we remove the connection and free it.
         */
        g_debug ("removing connection 0x%" PRIxPTR " from connection_manager "
                 "0x%" PRIxPTR,
                 (uintptr_t)connection,
                 (uintptr_t)source->connection_manager);
        connection_manager_remove (source->connection_manager,
                                   connection);
    }
    g_object_unref (connection);
    return TRUE;
}

ssize_t
wakeup_responder (CommandSource *source)
{
    g_debug ("Got new connection, updating fd_set");
    char buf[3] = { 0 };
    ssize_t ret;

    ret = read (source->wakeup_receive_fd, buf, WAKEUP_SIZE);
    if (ret != WAKEUP_SIZE)
        g_error ("read on wakeup_receive_fd returned %zd, was expecting %d",
                 ret, WAKEUP_SIZE);
    return ret;
}

/* This function is run as a separate thread dedicated to monitoring the
 * active connections with clients. It also monitors an additional file
 * descriptor that we call the wakeup_receive_fd. This pipe is the mechanism used
 * by the code that handles dbus calls to notify the command_source that
 * it's added a new connection to the connection_manager. When this happens the
 * command_source will begin to watch for data from this new connection.
 */
void*
command_source_thread (void *data)
{
    CommandSource *source;
    gint ret, i;

    source = COMMAND_SOURCE (data);
    pthread_cleanup_push (command_source_thread_cleanup, source);
    do {
        FD_ZERO (&source->connection_fdset);
        connection_manager_set_fds (source->connection_manager,
                                    &source->connection_fdset);
        FD_SET (source->wakeup_receive_fd, &source->connection_fdset);
        ret = select (FD_SETSIZE, &source->connection_fdset, NULL, NULL, NULL);
        if (ret == -1) {
            g_debug ("Error selecting on pipes: %s", strerror (errno));
            break;
        }
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &source->connection_fdset) &&
                i != source->wakeup_receive_fd)
            {
                g_debug ("data ready on connection fd: %d", i);
                command_source_connection_responder (source, i, source->sink);
            }
            if (FD_ISSET (i, &source->connection_fdset) &&
                i == source->wakeup_receive_fd)
            {
                g_debug ("data ready on wakeup_receive_fd");
                wakeup_responder (source);
            }
        }
    } while (TRUE);
    pthread_cleanup_pop (1);

    return (void*)0;
}

CommandSource*
command_source_new (ConnectionManager    *connection_manager,
                    CommandAttrs         *command_attrs)
{
    CommandSource *source;
    gint wakeup_fds [2] = { 0, };

    if (connection_manager == NULL)
        g_error ("command_source_new passed NULL ConnectionManager");
    g_object_ref (connection_manager);
    if (pipe2 (wakeup_fds, O_CLOEXEC) != 0)
        g_error ("failed to make wakeup pipe: %s", strerror (errno));
    source = COMMAND_SOURCE (g_object_new (TYPE_COMMAND_SOURCE,
                                             "command-attrs", command_attrs,
                                             "connection-manager", connection_manager,
                                             "wakeup-receive-fd", wakeup_fds [0],
                                             "wakeup-send-fd", wakeup_fds [1],
                                             NULL));
    source->running = FALSE;
    g_signal_connect (connection_manager,
                      "new-connection",
                      (GCallback) command_source_on_new_connection,
                      source);
    return source;
}

gint
command_source_start (Thread *self)
{
    CommandSource *source = COMMAND_SOURCE (self);

    if (source->thread != 0) {
        g_warning ("command_source already started");
        return -1;
    }
    source->running = TRUE;
    return pthread_create (&source->thread, NULL, command_source_thread, source);
}

gint
command_source_cancel (Thread *self)
{
    CommandSource *source = COMMAND_SOURCE (self);

    if (source == NULL) {
        g_warning ("command_source_cancel passed NULL source");
        return -1;
    }
    return pthread_cancel (source->thread);
}

gint
command_source_join (Thread *self)
{
    CommandSource *source = COMMAND_SOURCE (self);

    if (source == NULL) {
        g_warning ("command_source_join passed null source");
        return -1;
    }
    return pthread_join (source->thread, NULL);
}
