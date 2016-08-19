#ifndef MSG_H
#define MSG_H

#include <glib.h>
#include "session-data.h"

#define MSG_TYPE(msg) ((msg_t*)msg)->type

typedef enum msg_type {
    /* The thread that pulls a message of this type must reach a cancelation
     * point before blocking again. This is to allow the thread to be shutdown
     * cleanly.
     */
    CHECK_CANCEL_MSG_TYPE,
    /* A message from or to a session */
    DATA_MSG_TYPE,
} msg_type_t;

typedef struct msg {
    msg_type_t type;
} msg_t;

msg_type_t
msg_get_type (msg_t *msg);

/* check_cancel_msg_t */
typedef struct check_cancel_msg {
    msg_t msg;
} check_cancel_msg_t;

check_cancel_msg_t*
check_cancel_msg_new ();
void
check_cancel_msg_free (check_cancel_msg_t *msg);

/* data_msg_t */
typedef struct data_msg {
    msg_t            msg;
    SessionData     *session;
    guint8          *data;
    size_t           size;
} data_msg_t;

data_msg_t*
data_msg_new (SessionData    *session,
              guint8         *data,
              size_t          size);
void
data_msg_free (data_msg_t *msg);
SessionData*
data_msg_get_session (const data_msg_t *msg);
guint8*
data_msg_get_data (const data_msg_t *msg);
size_t
data_msg_get_size (const data_msg_t *msg);

#endif /* MSG_H */
