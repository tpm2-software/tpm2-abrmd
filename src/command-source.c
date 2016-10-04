#include <errno.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data-message.h"
#include "session-data.h"
#include "command-source.h"
#include "tab.h"
#include "thread-interface.h"
#include "util.h"

static gpointer command_source_parent_class = NULL;

enum {
    PROP_0,
    PROP_SESSION_MANAGER,
    PROP_TAB,
    PROP_WAKEUP_RECEIVE_FD,
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
    case PROP_TAB:
        self->tab = TAB (g_value_get_object (value));
        g_debug ("  tab: 0x%x", self->tab);
        break;
    case PROP_WAKEUP_RECEIVE_FD:
        self->wakeup_receive_fd = g_value_get_int (value);
        g_debug ("  wakeup_receive_fd: %d", self->wakeup_receive_fd);
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
    case PROP_TAB:
        g_value_set_object (value, self->tab);
        break;
    case PROP_WAKEUP_RECEIVE_FD:
        g_value_set_int (value, self->wakeup_receive_fd);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/* Not doing any sanity checks here. Be sure to shut things downon your own
 * first.
 */
static void
command_source_finalize (GObject  *object)
{
    CommandSource *source = COMMAND_SOURCE (object);

    g_object_unref (source->tab);
    g_object_unref (source->session_manager);
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
    obj_properties [PROP_WAKEUP_RECEIVE_FD] =
        g_param_spec_int ("wakeup-receive-fd",
                          "wakeup file descriptor",
                          "File descriptor to receive wakeup signal.",
                          0,
                          INT_MAX,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_TAB] =
        g_param_spec_object ("tab",
                             "Tab object",
                             "Tab instance.",
                             TYPE_TAB,
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
command_source_session_responder (CommandSource    *source,
                                   gint               fd,
                                   Tab               *tab)
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
    tab_send_command (tab, G_OBJECT (msg));
    /* the tab now owns this message */
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
                command_source_session_responder (source, i, source->tab);
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
                     gint wakeup_receive_fd,
                     Tab               *tab)
{
    CommandSource *source;

    if (session_manager == NULL)
        g_error ("command_source_new passed NULL SessionManager");
    if (tab == NULL)
        g_error ("command_source_new passed NULL Tab");
    g_object_ref (tab);
    g_object_ref (session_manager);
    source = COMMAND_SOURCE (g_object_new (TYPE_COMMAND_SOURCE,
                                             "session-manager", session_manager,
                                             "tab", tab,
                                             "wakeup-receive-fd", wakeup_receive_fd,
                                             NULL));
    source->running = FALSE;
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
