#include <errno.h>
#include <glib.h>
#include <pthread.h>

#include "response-watcher.h"
#include "control-message.h"
#include "data-message.h"

#define RESPONSE_WATCHER_TIMEOUT 1e6

static gpointer response_watcher_parent_class = NULL;

enum {
    PROP_0,
    PROP_TAB,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * GObject property setter.
 */
static void
response_watcher_set_property (GObject        *object,
                               guint           property_id,
                               GValue const   *value,
                               GParamSpec     *pspec)
{
    ResponseWatcher *self = RESPONSE_WATCHER (object);

    g_debug ("response_watcher_set_property");
    switch (property_id) {
    case PROP_TAB:
        self->tab = TAB (g_value_get_object (value));
        g_debug ("  tab: 0x%x", self->tab);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * GObject property getter.
 */
static void
response_watcher_get_property (GObject     *object,
                               guint        property_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
    ResponseWatcher *self = RESPONSE_WATCHER (object);

    g_debug ("response_watcher_get_property: 0x%x", self);
    switch (property_id) {
    case PROP_TAB:
        g_value_set_object (value, self->tab);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * Bring down the ResponseWatcher as gracefully as we can.
 */
static void
response_watcher_finalize (GObject *obj)
{
    ResponseWatcher *watcher = RESPONSE_WATCHER (obj);

    g_debug ("response_watcher_finalize");
    if (watcher == NULL)
        g_error ("  response_watcher_free passed NULL pointer");
    if (watcher->thread != 0)
        g_error ("  response_watcher finalized with running thread, cancel first");
    if (watcher->tab)
        g_object_unref (watcher->tab);
    if (response_watcher_parent_class)
        G_OBJECT_CLASS (response_watcher_parent_class)->finalize (obj);
}
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set finalize, property get/set.
 * - Install properties.
 */
static void
response_watcher_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (response_watcher_parent_class == NULL)
        response_watcher_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = response_watcher_finalize;
    object_class->get_property = response_watcher_get_property;
    object_class->set_property = response_watcher_set_property;

    obj_properties [PROP_TAB] =
        g_param_spec_object ("tab",
                             "Tab",
                             "TPM2 Access Broker.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * GObject boilerplate get_type.
 */
GType
response_watcher_get_type (void)
{
    static GType type = 0;

    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "ResponseWatcher",
                                              sizeof (ResponseWatcherClass),
                                              (GClassInitFunc) response_watcher_class_init,
                                              sizeof (ResponseWatcher),
                                              NULL,
                                              0);
    }
    return type;
}
/**
 */
ResponseWatcher*
response_watcher_new (Tab             *tab)
{
    if (tab == NULL)
        g_error ("response_watcher_new passed NULL Tab");
    return RESPONSE_WATCHER (g_object_new (TYPE_RESPONSE_WATCHER,
                                           "tab", tab,
                                           NULL));
}

gint
response_watcher_write_response (const gint fd,
                                 const DataMessage *msg)
{
    ssize_t written = 0;

    g_debug ("response_watcher_write_response");
    g_debug ("  writing 0x%x bytes", msg->size);
    g_debug_bytes (msg->data, msg->size, 16, 4);
    written = write_all (fd, msg->data, msg->size);
    if (written <= 0)
        g_warning ("write failed (%d) on fd %d for session 0x%x: %s",
                   written, fd, msg->session, strerror (errno));

    return written;
}

ssize_t
response_watcher_process_data_message (DataMessage *msg)
{
    gint fd;
    ssize_t written;

    g_debug ("response_watcher_thread got response message: 0x%x size %d",
             msg, msg->size);
    fd = session_data_send_fd (msg->session);
    written = response_watcher_write_response (fd, msg);
    if (written <= 0) {
        /* should have a reference to the session manager so we can remove
         * session after an error like this
         */
        g_warning ("Failed to write response to fd: %d", fd);
    }
}


void*
response_watcher_thread (void *data)
{
    ResponseWatcher *watcher = RESPONSE_WATCHER (data);
    GObject *obj;

    do {
        g_debug ("response_watcher_thread waiting for response on tab: 0x%x",
                 watcher->tab);
        obj = tab_get_response (watcher->tab);
        g_debug ("response_watcher_thread got obj: 0x%x", obj);
        if (IS_CONTROL_MESSAGE (obj))
            process_control_message (CONTROL_MESSAGE (obj));
        if (IS_DATA_MESSAGE (obj))
            response_watcher_process_data_message (DATA_MESSAGE (obj));
        g_object_unref (obj);
    } while (TRUE);
}

gint
response_watcher_start (ResponseWatcher *watcher)
{
    if (watcher == NULL)
        g_error ("response_watcher_start: passed NULL watcher");
    if (watcher->thread != 0)
        g_error ("response_watcher already started");
    return pthread_create (&watcher->thread, NULL, response_watcher_thread, watcher);
}
gint
response_watcher_cancel (ResponseWatcher *watcher)
{
    if (watcher == NULL)
        g_error ("response_watcher_cancel passed NULL watcher");
    if (watcher->thread == 0)
        g_error ("response_watcher_cancel: cannot cancel thread with id 0");
    return pthread_cancel (watcher->thread);
}
gint
response_watcher_join (ResponseWatcher *watcher)
{
    gint ret;

    if (watcher == NULL)
        g_error ("response_watcher_join passed NULL watcher");
    if (watcher->thread == 0)
        g_error ("response_watcher_join: cannot join thread with id 0");
    ret = pthread_join (watcher->thread, NULL);
    if (ret == 0)
        watcher->thread = 0;

    return ret;
}
