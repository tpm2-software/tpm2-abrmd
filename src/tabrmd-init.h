#ifndef TABRMD_INIT_H
#define TABRMD_INIT_H

#include <glib.h>

#include "tpm2.h"
#include "command-source.h"
#include "ipc-frontend.h"
#include "random.h"
#include "resource-manager.h"
#include "response-sink.h"
#include "tabrmd-options.h"

/*
 * Structure to hold data that we pass to the gmain loop as 'user_data'.
 * This data will be available to events from gmain including events from
 * the DBus.
 */
typedef struct gmain_data {
    tabrmd_options_t        options;
    GMainLoop              *loop;
    Tpm2           *tpm2;
    ResourceManager        *resource_manager;
    CommandSource          *command_source;
    Random                 *random;
    ResponseSink           *response_sink;
    GMutex                  init_mutex;
    IpcFrontend            *ipc_frontend;
    gboolean                ipc_disconnected;
} gmain_data_t;

gpointer
init_thread_func (gpointer user_data);
void
gmain_data_cleanup (gmain_data_t *data);
void
on_ipc_frontend_disconnect (IpcFrontend *ipc_frontend,
                            gmain_data_t *data);

#endif /* TABRMD_INIT_H */
