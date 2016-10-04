#ifndef TAB_H
#define TAB_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sapi/tpm20.h>

#include "message-queue.h"
#include "response-sink.h"
#include "session-data.h"
#include "tcti-interface.h"

G_BEGIN_DECLS

typedef struct _TabClass {
    GObjectClass parent;
} TabClass;

typedef struct _Tab {
    GObject           parent_instance;
    MessageQueue     *in_queue;
    pthread_t         thread;
    ResponseSink     *sink;
    Tcti             *tcti;
} Tab;

#define TYPE_TAB              (tab_get_type ())
#define TAB(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TAB, Tab))
#define TAB_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TAB, TabClass))
#define IS_TAB(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TAB))
#define IS_TAB_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TAB))
#define TAB_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TAB, TabClass))

GType       tab_get_type               (void);
Tab*        tab_new                    (Tcti              *tcti,
                                        ResponseSink      *sink);
void        tab_send_command           (Tab               *tab,
                                        GObject           *obj);
GObject*    tab_get_timeout_command    (Tab               *tab,
                                        guint64            timeout);
gint        tab_cancel_commands        (Tab               *tab,
                                        SessionData       *session);

G_END_DECLS
#endif /* TAB_H */
