#include <glib-unix.h>
#include <glib.h>
#include <inttypes.h>
#include <string.h>
#include <sysexits.h>

#include <tss2/tss2_tctildr.h>

#include <tss2/tss2_tctildr.h>

#include "tpm2.h"
#include "command-source.h"
#include "logging.h"
#include "ipc-frontend.h"
#include "ipc-frontend-dbus.h"
#include "random.h"
#include "resource-manager.h"
#include "response-sink.h"
#include "source-interface.h"
#include "tabrmd-init.h"
#include "tabrmd-options.h"
#include "tabrmd.h"
#include "util.h"

/*
 * This is a simple function to do sanity checks before calling
 * g_main_loop_quit.
 */
static void
main_loop_quit (GMainLoop *loop)
{
    g_info ("main_loop_quit");
    if (loop && g_main_loop_is_running (loop))
        g_main_loop_quit (loop);
}
/*
 * This is a very poorly named signal handling function. It is invoked in
 * response to a Unix signal. It does one thing:
 * - Shuts down the GMainLoop.
 */
static gboolean
signal_handler (gpointer user_data)
{
    g_info ("handling signal");
    main_loop_quit ((GMainLoop*)user_data);

    return G_SOURCE_CONTINUE;
}

/*
 * This function is a callback invoked by the IpcFrontend object
 * when a 'disconnect' event occurs. It sets a flag in the gmain_data_t
 * structure before killing off the gmain loop. This flag is checked by the
 * 'main' function that started the gmain loop and, if set, an exit code
 * is returned to indicate a failure.
 * Currently the only known reasons for the IPC disconnecting are not
 * recoverable.
 */
void
on_ipc_frontend_disconnect (IpcFrontend *ipc_frontend,
                            gmain_data_t *data)
{
    data->ipc_disconnected = TRUE;
    if (data->loop)
        main_loop_quit (data->loop);
}
static void
thread_cleanup (Thread **thread)
{
    thread_cancel (*thread);
    thread_join (*thread);
    g_clear_object (thread);
}
void
gmain_data_cleanup (gmain_data_t *data)
{
    g_debug ("%s", __func__);
    Thread* thread;

    if (data->command_source != NULL) {
        thread = THREAD (data->command_source);
        thread_cleanup (&thread);
    }
    if (data->resource_manager != NULL) {
        thread = THREAD (data->resource_manager);
        thread_cleanup (&thread);
    }
    if (data->response_sink != NULL) {
        thread = THREAD (data->response_sink);
        thread_cleanup (&thread);
    }
    if (data->ipc_frontend != NULL) {
        ipc_frontend_disconnect (data->ipc_frontend);
        g_clear_object (&data->ipc_frontend);
    }
    if (data->random != NULL) {
        g_clear_object (&data->random);
    }
    if (data->loop != NULL) {
        main_loop_quit (data->loop);
    }
}
/*
 * This function initializes and configures all of the long-lived objects
 * in the tabrmd system. It is invoked on a thread separate from the main
 * thread as a way to get the main thread listening for connections on
 * DBus as quickly as possible. Any incoming DBus requests will block
 * on the 'init_mutex' until this thread completes but they won't be
 * timing etc. This function does X things:
 * - Locks the init_mutex.
 * - Registers a handler for UNIX signals for SIGINT and SIGTERM.
 * - Seeds the RNG state from an entropy source.
 * - Creates the ConnectionManager.
 * - Creates the TCTI instance used by the Tab.
 * - Creates an tpm2 and verify the current state of the TPM.
 * - Creates and wires up the objects that make up the TPM command
 *   processing pipeline.
 * - Starts all of the threads in the command processing pipeline.
 * - Unlocks the init_mutex.
 */
gpointer
init_thread_func (gpointer user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    gint ret;
    TSS2_RC rc;
    CommandAttrs *command_attrs;
    ConnectionManager *connection_manager = NULL;
    SessionList *session_list;
    Tcti *tcti = NULL;
    TSS2_TCTI_CONTEXT *tcti_ctx = NULL;

    g_info ("init_thread_func start");
    g_mutex_lock (&data->init_mutex);
    /* Setup program signals */
    if (g_unix_signal_add(SIGINT, signal_handler, data->loop) <= 0 ||
        g_unix_signal_add(SIGTERM, signal_handler, data->loop) <= 0)
    {
        g_critical ("failed to setup signal handlers");
        ret = EX_OSERR;
        goto err_out;
    }

    data->random = random_new();
    ret = random_seed_from_file (data->random, data->options.prng_seed_file);
    if (ret != 0) {
        g_critical ("failed to seed Random object from seed source: %s",
                    data->options.prng_seed_file);
        ret = EX_OSERR;
        goto err_out;
    }

    connection_manager = connection_manager_new(data->options.max_connections);
    /* setup IpcFrontend */
    data->ipc_frontend =
        IPC_FRONTEND (ipc_frontend_dbus_new (data->options.bus,
                                             data->options.dbus_name,
                                             connection_manager,
                                             data->options.max_transients,
                                             data->random));
    g_signal_connect (data->ipc_frontend,
                      "disconnected",
                      (GCallback) on_ipc_frontend_disconnect,
                      data);
    ipc_frontend_connect (data->ipc_frontend,
                          &data->init_mutex);

    rc = Tss2_TctiLdr_Initialize (data->options.tcti_conf, &tcti_ctx);
    if (rc != TSS2_RC_SUCCESS || tcti_ctx == NULL) {
        g_critical ("%s: failed to create TCTI with conf \"%s\", got RC: 0x%x",
                    __func__, data->options.tcti_conf, rc);
        ret = EX_IOERR;
        goto err_out;
    }
    tcti = tcti_new (tcti_ctx);
    data->tpm2 = tpm2_new (tcti);
    g_clear_object (&tcti);
    rc = tpm2_init_tpm (data->tpm2);
    if (rc != TSS2_RC_SUCCESS) {
        ret = EX_UNAVAILABLE;
        g_critical ("failed to initialize Tpm2: 0x%" PRIx32, rc);
        goto err_out;
    }
    if (data->options.flush_all) {
        tpm2_flush_all_context (data->tpm2);
    }
    /*
     * Instantiate and the objects that make up the TPM command processing
     * pipeline.
     */
    command_attrs = command_attrs_new ();
    ret = command_attrs_init_tpm (command_attrs, data->tpm2);
    if (ret != 0) {
        g_critical ("%s: failed to initialize CommandAttribute object", __func__);
        ret = EX_UNAVAILABLE;
        goto err_out;
    }

    data->command_source =
        command_source_new (connection_manager, command_attrs);
    g_object_unref (connection_manager);
    session_list = session_list_new (data->options.max_sessions,
                                     SESSION_LIST_MAX_ABANDONED_DEFAULT);
    data->resource_manager = resource_manager_new (data->tpm2,
                                                   session_list);
    g_clear_object (&session_list);
    data->response_sink = response_sink_new ();
    g_object_unref (command_attrs);
    g_object_unref (data->tpm2);
    /*
     * Wire up the TPM command processing pipeline. TPM command buffers
     * flow from the CommandSource, to the Tab then finally back to the
     * caller through the ResponseSink.
     */
    source_add_sink (SOURCE (data->command_source),
                     SINK   (data->resource_manager));
    source_add_sink (SOURCE (data->resource_manager),
                     SINK   (data->response_sink));
    /*
     * Start the TPM command processing pipeline.
     */
    ret = thread_start (THREAD (data->command_source));
    if (ret != 0) {
        g_critical ("failed to start connection_source");
        ret = EX_OSERR;
        goto err_out;
    }
    ret = thread_start (THREAD (data->resource_manager));
    if (ret != 0) {
        g_critical ("failed to start ResourceManager: %s", strerror (errno));
        ret = EX_OSERR;
        goto err_out;
    }
    ret = thread_start (THREAD (data->response_sink));
    if (ret != 0) {
        g_critical ("failed to start response_source");
        ret = EX_OSERR;
        goto err_out;
    }

    g_mutex_unlock (&data->init_mutex);
    g_info ("init_thread_func done");

    return GINT_TO_POINTER (0);

err_out:
    g_debug ("%s: calling gmain_data_cleanup", __func__);
    gmain_data_cleanup (data);
    return GINT_TO_POINTER (ret);
}
