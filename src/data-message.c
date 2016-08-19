#include "data-message.h"

/* Boiler-plate gobject code.
 * NOTE: I tried to use the G_DEFINE_TYPE macro to take care of this boiler
 * plate for us but ended up with weird segfaults in the type checking macros.
 * Going back to doing this by hand resolved the issue thankfully.
 */
static gpointer data_message_parent_class = NULL;
/* override the parent finalize method so we can free the data associated with
 * the DataMessage instance.
 */
static void
data_message_finalize (GObject *obj)
{
    DataMessage *msg = DATA_MESSAGE (obj);

    if (msg->data)
        g_free (msg->data);
    G_OBJECT_CLASS (data_message_parent_class)->finalize (obj);
}
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
data_message_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (data_message_parent_class == NULL)
        data_message_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize = data_message_finalize;
}
/* Upon first call to *_get_type we register the type with the GType system.
 * We keep a static GType around to speed up future calls.
 */
GType
data_message_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "DataMessage",
                                              sizeof (DataMessageClass),
                                              (GClassInitFunc) data_message_class_init,
                                              sizeof (DataMessage),
                                              NULL,
                                              0);
    }
    return type;
}

DataMessage*
data_message_new (SessionData     *session,
                  guint8          *data,
                  size_t           size)
{
    DataMessage *msg = NULL;

    msg = DATA_MESSAGE (g_object_new (TYPE_DATA_MESSAGE, NULL));
    msg->session = session;
    msg->data    = data;
    msg->size    = size;

    return msg;
}

void
data_message_print (DataMessage *message)
{
    g_debug ("  DataMessage->session: 0x%x", message->session);
    g_debug ("  DataMessage->data: 0x%x",    message->data);
    g_debug ("  DataMessage->size: 0x%x",    message->size);
}
