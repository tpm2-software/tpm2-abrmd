#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data-message.h"
#include "session-data.h"
#include "command-source.h"
#include "thread-interface.h"
#include "util.h"

#define WAKEUP_DATA "hi"
#define WAKEUP_SIZE 2

static gpointer command_source_parent_class = NULL;

enum {
    PROP_0,
    PROP_SESSION_MANAGER,
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

static void
command_source_set_property (GObject       *object,
                              guint          property_id,
                              GValue const  *value,
                              GParamSpec    *pspec)
{
    CommandSource *self = COMMAND_SOURCE (object);

    g_debug ("command_source_set_properties: 0x%x", self);
    switch (property_id) {
    case PROP_SESSION_MANAGER:
        self->session_manager = SESSION_MANAGER (g_value_get_object (value));
        break;
    case PROP_SINK:
        self->sink = SINK (g_value_get_object (value));
        g_debug ("  sink: 0x%x", self->sink);
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

    g_debug ("command_source_get_properties: 0x%x", self);
    switch (property_id) {
    case PROP_SESSION_MANAGER:
        g_value_set_object (value, self->session_manager);
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
command_source_on_new_session (SessionManager   *session_manager,
                               SessionData      *session_data,
                               CommandSource    *command_source)
{
    gint ret;

    if (!IS_SESSION_MANAGER (session_manager))
        g_error ("command_source_on_new_session: first parameter 0x%x is "
                 "*not* a SessionManager!", session_manager);
    if (!IS_SESSION_DATA (session_data))
        g_error ("command_source_on_new_session: second parameter 0x%x is "
                 "*not* a SessionData!", session_data);
    if (!IS_COMMAND_SOURCE (command_source))
        g_error ("command_source_on_new_session: third parameter 0x%x is "
                 "*not* a CommandSource!");

    g_debug ("command_source_on_new_session: writing \"%s\" to fd: %d",
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

    g_object_unref (source->sink);
    g_object_unref (source->session_manager);
    close (source->wakeup_send_fd);
    close (source->wakeup_receive_fd);
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

    obj_properties [PROP_SESSION_MANAGER] =
        g_param_spec_object ("session-manager",
                             "SessionManager object",
                             "SessionManager instance.",
                             TYPE_SESSION_MANAGER,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SINK] =
        g_param_spec_object ("sink",
                             "Sink",
                             "Reference to a Sink object.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
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
command_source_interface_init (gpointer g_iface)
{
    ThreadInterface *thread = (ThreadInterface*)g_iface;
    thread->cancel = command_source_cancel;
    thread->join   = command_source_join;
    thread->start  = command_source_start;
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
        const GInterfaceInfo interface_info = {
            (GInterfaceInitFunc) command_source_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_THREAD, &interface_info);
    }
    return type;
}

DataMessage*
data_message_from_fd (SessionData     *session,
                      gint             fd)
{
    guint8 *command_buf = NULL;
    DataMessage *msg;
    UINT32 command_size = 0;

    g_debug ("data_message_from_fd: %d", fd);
    command_buf = read_tpm_command_from_fd (fd, &command_size);
    if (command_buf == NULL)
        return NULL;
    return data_message_new (session, command_buf, command_size);
}

void
command_source_thread_cleanup (void *data)
{
    CommandSource *source = COMMAND_SOURCE (data);

    source->running = FALSE;
}

gboolean
command_source_session_responder (CommandSource      *source,
                                  gint                fd,
                                  Sink               *sink)
{
    DataMessage *msg;
    SessionData *session;
    gint ret;

    g_debug ("command_source_session_responder 0x%x", source);
    session = session_manager_lookup_fd (source->session_manager, fd);
    if (session == NULL)
        g_error ("failed to get session associated with fd: %d", fd);
    else
        g_debug ("session_manager_lookup_fd for fd %d: 0x%x", fd, session);
    msg = data_message_from_fd (session, fd);
    if (msg == NULL) {
        /* msg will be NULL when read error on fd, or fd is closed (EOF)
         * In either case we remove the session and free it.
         */
        g_debug ("removing session 0x%x from session_manager 0x%x",
                 session, source->session_manager);
        session_manager_remove (source->session_manager,
                                session);
        return TRUE;
    }
    sink_enqueue (sink, G_OBJECT (msg));
    /* the sink now owns this message */
    g_object_unref (msg);
    return TRUE;
}

int
wakeup_responder (CommandSource *source)
{
    g_debug ("Got new session, updating fd_set");
    char buf[3] = { 0 };
    gint ret = read (source->wakeup_receive_fd, buf, WAKEUP_SIZE);
    if (ret != WAKEUP_SIZE)
        g_error ("read on wakeup_receive_fd returned %d, was expecting %d",
                 ret, WAKEUP_SIZE);
    return ret;
}

/* This function is run as a separate thread dedicated to monitoring the
 * active sessions with clients. It also monitors an additional file
 * descriptor that we call the wakeup_receive_fd. This pipe is the mechanism used
 * by the code that handles dbus calls to notify the command_source that
 * it's added a new session to the session_manager. When this happens the
 * command_source will begin to watch for data from this new session.
 */
void*
command_source_thread (void *data)
{
    CommandSource *source;
    gint ret, i;

    source = COMMAND_SOURCE (data);
    pthread_cleanup_push (command_source_thread_cleanup, source);
    do {
        FD_ZERO (&source->session_fdset);
        session_manager_set_fds (source->session_manager,
                                 &source->session_fdset);
        FD_SET (source->wakeup_receive_fd, &source->session_fdset);
        ret = select (FD_SETSIZE, &source->session_fdset, NULL, NULL, NULL);
        if (ret == -1) {
            g_debug ("Error selecting on pipes: %s", strerror (errno));
            break;
        }
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &source->session_fdset) &&
                i != source->wakeup_receive_fd)
            {
                g_debug ("data ready on session fd: %d", i);
                command_source_session_responder (source, i, source->sink);
            }
            if (FD_ISSET (i, &source->session_fdset) &&
                i == source->wakeup_receive_fd)
            {
                g_debug ("data ready on wakeup_receive_fd");
                wakeup_responder (source);
            }
        }
    } while (TRUE);
    pthread_cleanup_pop (1);
}

CommandSource*
command_source_new (SessionManager    *session_manager,
                    Sink              *sink)
{
    CommandSource *source;
    gint wakeup_fds [2] = { 0, };

    if (session_manager == NULL)
        g_error ("command_source_new passed NULL SessionManager");
    if (sink == NULL)
        g_error ("command_source_new passed NULL Tab");
    g_object_ref (sink);
    g_object_ref (session_manager);
    if (pipe2 (wakeup_fds, O_CLOEXEC) != 0)
        g_error ("failed to make wakeup pipe: %s", strerror (errno));
    source = COMMAND_SOURCE (g_object_new (TYPE_COMMAND_SOURCE,
                                             "session-manager", session_manager,
                                             "sink", sink,
                                             "wakeup-receive-fd", wakeup_fds [0],
                                             "wakeup-send-fd", wakeup_fds [1],
                                             NULL));
    source->running = FALSE;
    g_signal_connect (session_manager,
                      "new-session",
                      (GCallback) command_source_on_new_session,
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
