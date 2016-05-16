#include <glib.h>
#include <pthread.h>
#include <sys/select.h>

#include "session-manager.h"

#define BUF_SIZE 4096
#define WAKEUP_DATA "hi"
#define WAKEUP_SIZE 2

typedef struct session_watcher session_watcher_t;

typedef int (*session_callback_t) (session_watcher_t *watcher,
                                   gint fd,
                                   gpointer user_data);
typedef int (*wakeup_callback_t) (session_watcher_t *watcher,
                                  gpointer user_data);

session_watcher_t*
session_watcher_new (session_manager_t *connection_manager,
                     gint wakeup_receive_fd);
session_watcher_t*
session_watcher_new_full (session_manager_t *connection_manager,
                          gint wakeup_receive_fd,
                          session_callback_t session_cb,
                          wakeup_callback_t wakeup_cb,
                          gpointer user_data);
gint session_watcher_start (session_watcher_t *watcher);
gint session_watcher_cancel (session_watcher_t *watcher);
gint session_watcher_join (session_watcher_t *watcher);
void session_watcher_free (session_watcher_t *watcher);
