#include <errno.h>
#include <stdlib.h>

#include "msg.h"

inline msg_type_t
msg_get_type (msg_t *msg)
{
    return ((msg_t*)msg)->type;
}

check_cancel_msg_t*
check_cancel_msg_new ()
{
    check_cancel_msg_t *msg;

    msg = calloc (1, sizeof (check_cancel_msg_t));
    if (msg == NULL)
        g_error ("failed to allocate check_cancel_msg_t: %s",
                 strerror (errno));
    MSG_TYPE (msg) = CHECK_CANCEL_MSG_TYPE;

    return msg;
}

void
check_cancel_msg_free (check_cancel_msg_t *msg)
{
    if (msg == NULL) {
        g_warning ("check_cancel_msg_free: cannot free NULL msg");
        return;
    }
    free (msg);
}

data_msg_t*
data_msg_new (session_data_t  *session,
              guint8          *data,
              size_t           size)
{
    data_msg_t *msg;

    msg = calloc (1, sizeof (data_msg_t));
    MSG_TYPE (msg) = DATA_MSG_TYPE;
    msg->session = session;
    msg->data = data;
    msg->size = size;

    return msg;
}

void
data_msg_free (data_msg_t *msg)
{
    if (msg == NULL) {
        g_warning ("data_msg_free: can't free a NULL msg");
        return;
    }
    if (msg->data != NULL)
        free (msg->data);
    free (msg);
}
inline session_data_t*
data_msg_get_session (const data_msg_t *msg)
{
    return msg->session;
}
inline guint8*
data_msg_get_data (const data_msg_t *msg)
{
    return msg->data;
}
inline size_t
data_msg_get_sie (const data_msg_t *msg)
{
    return msg->size;
}
