#include "data-message.h"

G_DEFINE_TYPE (DataMessage, data_message, G_TYPE_OBJECT);

static void
data_message_finalize (GObject *obj)
{
    DataMessage *msg = DATA_MESSAGE (obj);

    if (msg->data)
        g_free (msg->data);
    G_OBJECT_CLASS (data_message_parent_class)->finalize (obj);
}

static void
data_message_init       (DataMessage *self)     {}

static void
data_message_class_init (DataMessageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = data_message_finalize;
}

DataMessage*
data_message_new (session_data_t  *session,
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
