#ifndef SESSION_WATCHER_H
#define SESSION_WATCHER_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sys/select.h>

#include "session-manager.h"
#include "tab.h"

G_BEGIN_DECLS

/* Chunk size for allocations to hold data from clients. */
#define BUF_SIZE 4096
/* Maximum buffer size for client data. Connections that send a single
 * command larger than this size will be closed.
 */
#define BUF_MAX  4*BUF_SIZE
#define WAKEUP_DATA "hi"
#define WAKEUP_SIZE 2

typedef struct _SessionWatcherClass {
    GObjectClass       parent;
} SessionWatcherClass;

typedef struct _SessionWatcher {
    GObject            parent_instance;
    SessionManager    *session_manager;
    pthread_t          thread;
    gint               wakeup_receive_fd;
    gboolean           running;
    fd_set             session_fdset;
    Tab               *tab;
} SessionWatcher;

#define TYPE_SESSION_WATCHER              (session_watcher_get_type   ())
#define SESSION_WATCHER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_SESSION_WATCHER, SessionWatcher))
#define SESSION_WATCHER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_SESSION_WATCHER, SessionWatcherClass))
#define IS_SESSION_WATCHER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_SESSION_WATCHER))
#define IS_SESSION_WATCHER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_SESSION_WATCHER))
#define SESSION_WATCHER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_SESSION_WATCHER, SessionWatcherClass))

GType            session_watcher_get_type (void);
SessionWatcher*  session_watcher_new      (SessionManager     *session_manager,
                                           gint                wakeup_receive_fd,
                                           Tab                *tab);
void             session_watcher_free     (SessionWatcher     *watcher);

G_END_DECLS
#endif /* SESSION_WATCHER_H */
