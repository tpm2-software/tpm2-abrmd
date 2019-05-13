/* SPDX-License-Identifier: BSD-2 */
/*
 * Copyright (c) 2017 - 2018 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib-unix.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tss2/tss2_tpm2_types.h>

#include "tabrmd-options.h"
#include "tabrmd.h"
#include "access-broker.h"
#include "connection.h"
#include "connection-manager.h"
#include "logging.h"
#include "thread.h"
#include "command-source.h"
#include "ipc-frontend.h"
#include "ipc-frontend-dbus.h"
#include "random.h"
#include "resource-manager.h"
#include "response-sink.h"
#include "source-interface.h"
#include "tcti-factory.h"
#include "tcti.h"
#include "util.h"

/* Structure to hold data that we pass to the gmain loop as 'user_data'.
 * This data will be available to events from gmain including events from
 * the DBus.
 */
typedef struct gmain_data {
    tabrmd_options_t        options;
    GMainLoop              *loop;
    AccessBroker           *access_broker;
    ResourceManager        *resource_manager;
    CommandSource          *command_source;
    Random                 *random;
    ResponseSink           *response_sink;
    GMutex                  init_mutex;
    IpcFrontend            *ipc_frontend;
} gmain_data_t;

/**
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
/**
 * This is a very poorly named signal handling function. It is invoked in
 * response to a Unix signal. It does one thing:
 * - Shuts down the GMainLoop.
 */
static gboolean
signal_handler (gpointer user_data)
{
    g_info ("handling signal");
    /* this is the only place the global pointer to the GMainLoop is accessed */
    main_loop_quit ((GMainLoop*)user_data);

    return G_SOURCE_CONTINUE;
}

static void
on_ipc_frontend_disconnect (IpcFrontend *ipc_frontend,
                            GMainLoop   *loop)
{
    g_info ("%s: IpcFrontend %p disconnected", __func__, objid (ipc_frontend));
    main_loop_quit (loop);
}
/**
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
 * - Creates an access broker and verify the current state of the TPM.
 * - Creates and wires up the objects that make up the TPM command
 *   processing pipeline.
 * - Starts all of the threads in the command processing pipeline.
 * - Unlocks the init_mutex.
 */
static gpointer
init_thread_func (gpointer user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    gint ret;
    TSS2_RC rc;
    CommandAttrs *command_attrs;
    ConnectionManager *connection_manager = NULL;
    SessionList *session_list;
    Tcti *tcti = NULL;
    TctiFactory *tcti_factory = NULL;

    g_info ("init_thread_func start");
    g_mutex_lock (&data->init_mutex);
    /* Setup program signals */
    if (g_unix_signal_add(SIGINT, signal_handler, data->loop) <= 0 ||
        g_unix_signal_add(SIGTERM, signal_handler, data->loop) <= 0)
    {
        g_error("failed to setup signal handlers");
    }

    data->random = random_new();
    ret = random_seed_from_file (data->random, data->options.prng_seed_file);
    if (ret != 0)
        g_error ("failed to seed Random object");

    connection_manager = connection_manager_new(data->options.max_connections);
    if (connection_manager == NULL)
        g_error ("failed to allocate connection_manager");
    g_debug ("%s: ConnectionManager: %p", __func__, objid (connection_manager));
    /* setup IpcFrontend */
    data->ipc_frontend =
        IPC_FRONTEND (ipc_frontend_dbus_new (data->options.bus,
                                             data->options.dbus_name,
                                             connection_manager,
                                             data->options.max_transients,
                                             data->random));
    if (data->ipc_frontend == NULL) {
        g_error ("failed to allocate IpcFrontend object");
    }
    g_signal_connect (data->ipc_frontend,
                      "disconnected",
                      (GCallback) on_ipc_frontend_disconnect,
                      data->loop);
    ipc_frontend_connect (data->ipc_frontend,
                          &data->init_mutex);

    tcti_factory = tcti_factory_new (data->options.tcti_filename,
                                     data->options.tcti_conf);
    tcti = tcti_factory_create (tcti_factory);
    if (tcti == NULL) {
        tabrmd_critical ("%s: failed to create TCTI with name \"%s\" and conf "
                         "\"%s\"", __func__, data->options.tcti_filename,
                         data->options.tcti_conf);
    }
    g_clear_object (&tcti_factory);
    data->access_broker = access_broker_new (tcti);
    g_debug ("%s: created AccessBroker: %p", __func__,
             objid (data->access_broker));
    g_clear_object (&tcti);
    rc = access_broker_init_tpm (data->access_broker);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to initialize AccessBroker: 0x%" PRIx32, rc);
    if (data->options.flush_all) {
        access_broker_flush_all_context (data->access_broker);
    }
    /**
     * Instantiate and the objects that make up the TPM command processing
     * pipeline.
     */
    command_attrs = command_attrs_new ();
    g_debug ("%s: created CommandAttrs: %p", __func__, objid (command_attrs));
    ret = command_attrs_init_tpm (command_attrs, data->access_broker);
    if (ret != 0)
        g_error ("%s: failed to initialize CommandAttribute object: %p",
                 __func__, objid (command_attrs));

    data->command_source =
        command_source_new (connection_manager, command_attrs);
    g_object_unref (connection_manager);
    g_debug ("%s: created command source: %p", __func__,
             objid (data->command_source));
    session_list = session_list_new (data->options.max_sessions,
                                     SESSION_LIST_MAX_ABANDONED_DEFAULT);
    data->resource_manager = resource_manager_new (data->access_broker,
                                                   session_list);
    g_clear_object (&session_list);
    g_debug ("%s: created ResourceManager: %p", __func__,
             objid (data->resource_manager));
    data->response_sink = response_sink_new ();
    g_debug ("%s: created response source: %p", __func__,
             objid (data->response_sink));
    g_object_unref (command_attrs);
    g_object_unref (data->access_broker);
    /**
     * Wire up the TPM command processing pipeline. TPM command buffers
     * flow from the CommandSource, to the Tab then finally back to the
     * caller through the ResponseSink.
     */
    source_add_sink (SOURCE (data->command_source),
                     SINK   (data->resource_manager));
    source_add_sink (SOURCE (data->resource_manager),
                     SINK   (data->response_sink));
    /**
     * Start the TPM command processing pipeline.
     */
    ret = thread_start (THREAD (data->command_source));
    if (ret != 0)
        g_error ("failed to start connection_source");
    ret = thread_start (THREAD (data->resource_manager));
    if (ret != 0)
        g_error ("failed to start ResourceManager: %s", strerror (errno));
    ret = thread_start (THREAD (data->response_sink));
    if (ret != 0)
        g_error ("failed to start response_source");

    g_mutex_unlock (&data->init_mutex);
    g_info ("init_thread_func done");

    return GINT_TO_POINTER (0);
}
void
thread_cleanup (Thread *thread)
{
    thread_cancel (thread);
    thread_join (thread);
    g_object_unref (thread);
}
/**
 * This is the entry point for the TPM2 Access Broker and Resource Manager
 * daemon. It is responsible for the top most initialization and
 * coordination before blocking on the GMainLoop (g_main_loop_run):
 * - Collects / parses command line options.
 * - Creates the initialization thread and kicks it off.
 * - Registers / owns a name on a DBus.
 * - Blocks on the main loop.
 * At this point all of the tabrmd processing is being done on other threads.
 * When the daemon shutsdown (for any reason) we do cleanup here:
 * - Join / cleanup the initialization thread.
 * - Release the name on the DBus.
 * - Cancel and join all of the threads started by the init thread.
 * - Cleanup all of the objects created by the init thread.
 */
int
main (int argc, char *argv[])
{
    gmain_data_t gmain_data = { .options = TABRMD_OPTIONS_INIT_DEFAULT };
    GThread *init_thread;
    gint init_ret;

    util_init ();
    g_info ("tabrmd startup");
    if (!parse_opts (argc, argv, &gmain_data.options)) {
        return 1;
    }
    if (geteuid() == 0 && ! gmain_data.options.allow_root) {
        g_print ("Refusing to run as root. Pass --allow-root if you know what you are doing.\n");
        return 1;
    }

    g_mutex_init (&gmain_data.init_mutex);
    gmain_data.loop = g_main_loop_new (NULL, FALSE);
    /*
     * Initialize program data on a separate thread. The main thread needs to
     * get into the GMainLoop ASAP to acquire a dbus name and become
     * responsive to clients.
     */
    init_thread = g_thread_new (TABD_INIT_THREAD_NAME,
                                init_thread_func,
                                &gmain_data);
    g_info ("entering g_main_loop");
    g_main_loop_run (gmain_data.loop);
    g_info ("g_main_loop_run done, cleaning up");
    init_ret = GPOINTER_TO_INT (g_thread_join (init_thread));
    /* cleanup glib stuff first so we stop getting events */
    ipc_frontend_disconnect (gmain_data.ipc_frontend);
    g_object_unref (gmain_data.ipc_frontend);
    /* tear down the command processing pipeline */
    thread_cleanup (THREAD (gmain_data.command_source));
    thread_cleanup (THREAD (gmain_data.resource_manager));
    thread_cleanup (THREAD (gmain_data.response_sink));
    /* clean up what remains */
    g_object_unref (gmain_data.random);
    return init_ret;
}
