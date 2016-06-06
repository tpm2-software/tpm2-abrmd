#ifndef DATA_MESSAGE_H
#define DATA_MESSAGE_H

#include <glib-object.h>

#include "session-data.h"

G_BEGIN_DECLS

typedef struct _DataMessageClass {
    GObjectClass parent;
} DataMessageClass;

typedef struct _DataMessage
{
    GObject          parent_instance;
    session_data_t  *session;
    guint8          *data;
    size_t           size;
} DataMessage;

GType
data_message_get_type (void);

#define TYPE_DATA_MESSAGE            (data_message_get_type      ())
#define DATA_MESSAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_DATA_MESSAGE, DataMessage))
#define DATA_MESSAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_DATA_MESSAGE, DataMessageClass))
#define IS_DATA_MESSAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_DATA_MESSAGE))
#define IS_DATA_MESSAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_DATA_MESSAGE))
#define DATA_MESSAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_DATA_MESSAGE, DataMessageClass))

DataMessage*
data_message_new (session_data_t  *session,
                  guint8          *data,
                  size_t           size);
void
data_message_print (DataMessage *message);

G_END_DECLS
#endif /* MESSAGE_H */
