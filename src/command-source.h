#ifndef COMMAND_SOURCE_H
#define COMMAND_SOURCE_H

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

typedef struct _CommandSourceClass {
    GObjectClass       parent;
} CommandSourceClass;

typedef struct _CommandSource {
    GObject            parent_instance;
    SessionManager    *session_manager;
    pthread_t          thread;
    gint               wakeup_receive_fd;
    gint               wakeup_send_fd;
    gboolean           running;
    fd_set             session_fdset;
    Tab               *tab;
} CommandSource;

#define TYPE_COMMAND_SOURCE              (command_source_get_type   ())
#define COMMAND_SOURCE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_COMMAND_SOURCE, CommandSource))
#define COMMAND_SOURCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_COMMAND_SOURCE, CommandSourceClass))
#define IS_COMMAND_SOURCE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_COMMAND_SOURCE))
#define IS_COMMAND_SOURCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_COMMAND_SOURCE))
#define COMMAND_SOURCE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_COMMAND_SOURCE, CommandSourceClass))

GType            command_source_get_type (void);
CommandSource*   command_source_new      (SessionManager     *session_manager,
                                          Tab                *tab);
void             command_source_free     (CommandSource     *watcher);
gint            command_source_on_new_session (SessionManager  *session_manager,
                                               SessionData     *session_data,
                                               CommandSource   *command_source);

G_END_DECLS
#endif /* COMMAND_SOURCE_H */
