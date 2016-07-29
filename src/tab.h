#ifndef TAB_H
#define TAB_H

#include <glib.h>
#include <pthread.h>
#include <sapi/tpm20.h>
#include "message-queue.h"
#include "session-data.h"
#include "tcti-interface.h"

typedef struct tab {
    MessageQueue *in_queue, *out_queue;
    pthread_t thread;
    Tcti      *tcti;
} tab_t;


tab_t*
tab_new                  (Tcti              *tcti);
void
tab_send_command         (tab_t             *tab,
                          GObject           *obj);
GObject*
tab_get_timeout_command (tab_t              *tab,
                         guint64             timeout);
GObject*
tab_get_response        (tab_t              *tab);
GObject*
tab_get_timeout_response (tab_t             *tab,
                          guint64            timeout);
gint
tab_cancel_commands      (tab_t             *tab,
                          session_data_t    *session);

#endif /* TAB_H */
