#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <glib.h>

#include "session-data.h"

typedef struct session_manager {
    pthread_mutex_t mutex;
    GHashTable *session_from_fd_table;
    GHashTable *session_from_id_table;
} session_manager_t;


session_manager_t* session_manager_new     (void);
void               session_manager_free    (session_manager_t *manager);
gint               session_manager_insert  (session_manager_t *manager,
                                            SessionData       *session);
gint               session_manager_remove  (session_manager_t *manager,
                                            SessionData       *session);
SessionData*       session_manager_lookup_fd (session_manager_t *manager,
                                              gint fd_in);
SessionData*       session_manager_lookup_id (session_manager_t *manager,
                                              gint64             id_in);
void               session_manager_set_fds (session_manager_t *manager,
                                            fd_set *fds);
guint              session_manager_size    (session_manager_t *manager);
#endif /* SESSION_MANAGER_H */
