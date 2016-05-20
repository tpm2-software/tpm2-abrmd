#ifndef RESPONSE_WATCHER_H
#define RESPONSE_WATCHER_H

#include <glib.h>
#include "tab.h"

/** DON'T TOUCH!
 * Direct access to data in this structure his highly discouraged. Use the
 * functions in this header to interact with this "object". If you're tempted
 * to access the structure directly you probably need to update the API.
 */
typedef struct response_watcher {
    tab_t             *tab;
    pthread_t          thread;
} response_watcher_t;

response_watcher_t*
response_watcher_new     (tab_t               *tab);
gint
response_watcher_start   (response_watcher_t  *watcher);
gint
response_watcher_cancel  (response_watcher_t  *watcher);
gint
response_watcher_join    (response_watcher_t  *watcher);
void
response_watcher_free    (response_watcher_t  *watcher);

#endif /* RESPONSE_WATCHER_H */
