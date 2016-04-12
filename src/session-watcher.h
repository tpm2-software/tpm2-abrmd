#include <glib.h>
#include <pthread.h>
#include <sys/select.h>

#include "session-manager.h"

typedef struct session_watcher {
    session_manager_t *session_manager;
    pthread_t thread;
    gint wakeup_receive_fd;
    gboolean running;
    char *buf;
    fd_set session_fdset;
} session_watcher_t;

session_watcher_t*
session_watcher_new (session_manager_t *connection_manager,
                     gint wakeup_receive_fd);
gint session_watcher_start (session_watcher_t *watcher);
gint session_watcher_cancel (session_watcher_t *watcher);
gint session_watcher_join (session_watcher_t *watcher);
void session_watcher_free (session_watcher_t *watcher);
