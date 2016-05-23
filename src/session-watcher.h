#ifndef SESSION_WATCHER_H
#define SESSION_WATCHER_H

#include <glib.h>
#include <pthread.h>
#include <sys/select.h>

#include "session-manager.h"
#include "tab.h"

/* Chunk size for allocations to hold data from clients. */
#define BUF_SIZE 4096
/* Maximum buffer size for client data. Connections that send a single
 * command larger than this size will be closed.
 */
#define BUF_MAX  4*BUF_SIZE
#define WAKEUP_DATA "hi"
#define WAKEUP_SIZE 2

typedef struct session_watcher {
    session_manager_t *session_manager;
    pthread_t thread;
    gint wakeup_receive_fd;
    gboolean running;
    fd_set session_fdset;
    tab_t *tab;
} session_watcher_t;

session_watcher_t*
session_watcher_new (session_manager_t *connection_manager,
                     gint wakeup_receive_fd,
                     tab_t             *tab);
gint session_watcher_start (session_watcher_t *watcher);
gint session_watcher_cancel (session_watcher_t *watcher);
gint session_watcher_join (session_watcher_t *watcher);
void session_watcher_free (session_watcher_t *watcher);

#endif /* SESSION_WATCHER_H */
