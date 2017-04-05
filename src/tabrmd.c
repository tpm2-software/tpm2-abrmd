#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sapi/tpm20.h>
#include "tabrmd.h"
#include "access-broker.h"
#include "connection.h"
#include "connection-manager.h"
#include "tabrmd-priv.h"
#include "logging.h"
#include "thread-interface.h"
#include "command-source.h"
#include "random.h"
#include "resource-manager.h"
#include "response-sink.h"
#include "source-interface.h"
#include "tabrmd-generated.h"
#include "tcti-options.h"

#ifdef G_OS_UNIX
#include <gio/gunixfdlist.h>
#endif

/* Structure to hold data that we pass to the gmain loop as 'user_data'.
 * This data will be available to events from gmain including events from
 * the DBus.
 */
typedef struct gmain_data {
    tabrmd_options_t        options;
    GMainLoop              *loop;
    AccessBroker           *access_broker;
    ResourceManager        *resource_manager;
    TctiTabrmd             *skeleton;
    ConnectionManager         *manager;
    CommandSource         *command_source;
    Random                 *random;
    Sink                   *response_sink;
    GMutex                  init_mutex;
    Tcti                   *tcti;
} gmain_data_t;

/* This global pointer to the GMainLoop is necessary so that we can react to
 * unix signals. Only the signal handler should touch this.
 */
static GMainLoop *g_loop;

#define TABRMD_ERROR tabrmd_error_quark ()
typedef enum {
    TABRMD_ERROR_MAX_CONNECTIONS,
    TABRMD_ERROR_ID_GENERATION,
} TabrmdErrorEnum;

/*
 */
GQuark
tabrmd_error_quark (void)
{
    return g_quark_from_static_string ("tabrmd-error");
}

/**
 * This is a utility function that builds an array of handles as a
 * GVariant object. The handles that make up the array are passed in
 * as a GUnixFDList.
 */
static GVariant*
handle_array_variant_from_fdlist (GUnixFDList *fdlist)
{
    GVariant *tuple;
    GVariantBuilder *builder;
    gint i = 0;

    /* build array of handles as GVariant */
    builder = g_variant_builder_new (G_VARIANT_TYPE ("ah"));
    for (i = 0; i < g_unix_fd_list_get_length (fdlist); ++i)
        g_variant_builder_add (builder, "h", i);
    /* create tuple variant from builder */
    tuple = g_variant_new ("ah", builder);
    g_variant_builder_unref (builder);

    return tuple;
}

/**
 * This is a signal handler for the handle-create-connection signal from
 * the Tpm2AccessBroker DBus interface. This signal is triggered by a
 * request from a client to create a new connection with the tabrmd. This
 * requires a few things be done:
 * - Create a new ID (uint64) for the connection.
 * - Create a new Connection object getting the FDs that must be returned
 *   to the client.
 * - Build up a dbus response to the client with their connection ID and
 *   send / receive FDs.
 * - Send the response message back to the client.
 * - Insert the new Connection object into the ConnectionManager.
 * - Notify the CommandSource of the new Connection that it needs to
 *   watch by writing a magic value to the wakeup_send_fd.
 */
static gboolean
on_handle_create_connection (TctiTabrmd            *skeleton,
                             GDBusMethodInvocation *invocation,
                             gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    HandleMap   *handle_map = NULL;
    Connection *connection = NULL;
    gint client_fds[2] = { 0, 0 }, ret = 0;
    GVariant *response_variants[2], *response_tuple;
    GUnixFDList *fd_list = NULL;
    guint64 id;

    /* make sure the init thread is done before we create new connections
     */
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    if (connection_manager_is_full (data->manager)) {
        g_dbus_method_invocation_return_error (invocation,
                                               TABRMD_ERROR,
                                               TABRMD_ERROR_MAX_CONNECTIONS,
                                               "MAX_COMMANDS exceeded. Try again later.");
        return TRUE;
    }
    id = random_get_uint64 (data->random);
    if (connection_manager_contains_id (data->manager, id)) {
        g_warning ("ID collision in ConnectionManager: %" PRIu64, id);
        g_dbus_method_invocation_return_error (
            invocation,
            TABRMD_ERROR,
            TABRMD_ERROR_ID_GENERATION,
            "Failed to allocate connection ID. Try again later.");
        return TRUE;
    }
    handle_map = handle_map_new (TPM_HT_TRANSIENT, data->options.max_transient_objects);
    if (handle_map == NULL)
        g_error ("Failed to allocate new HandleMap");
    connection = connection_new (&client_fds[0], &client_fds[1], id, handle_map);
    g_object_unref (handle_map);
    if (connection == NULL)
        g_error ("Failed to allocate new connection.");
    g_debug ("Created connection with fds: %d, %d and id: 0x%" PRIx64,
             client_fds[0], client_fds[1], id);
    /* prepare tuple variant for response message */
    fd_list = g_unix_fd_list_new_from_array (client_fds, 2);
    response_variants[0] = handle_array_variant_from_fdlist (fd_list);
    response_variants[1] = g_variant_new_uint64 (id);
    response_tuple = g_variant_new_tuple (response_variants, 2);
    /* add Connection to manager */
    ret = connection_manager_insert (data->manager, connection);
    if (ret != 0) {
        g_warning ("Failed to add new connection to connection_manager.");
    }
    /* send response */
    g_dbus_method_invocation_return_value_with_unix_fd_list (
        invocation,
        response_tuple,
        fd_list);
    g_object_unref (fd_list);
    g_object_unref (connection);

    return TRUE;
}
/**
 * This is a signal handler for the Cancel event emitted by the
 * Tpm2AcessBroker. It is invoked by a signal generated by a user
 * requesting that an outstanding TPM command should be canceled. It is
 * registered with the Tabrmd in response to acquiring a name
 * on the dbus (on_name_acquired). It does X things:
 * - Ensure the init thread has completed successfully by locking and then
 *   unlocking the init mutex.
 * - Locate the Connection object associted with the 'id' parameter in
 *   the ConnectionManager.
 * - If the connection has a command being processed by the tabrmd then it's
 *   removed from the processing queue.
 * - If the connection has a command being processed by the TPM then the
 *   request to cancel the command will be sent down to the TPM.
 * - If the connection has no commands outstanding then an error is
 *   returned.
 */
static gboolean
on_handle_cancel (TctiTabrmd           *skeleton,
                  GDBusMethodInvocation *invocation,
                  gint64                 id,
                  gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    Connection *connection = NULL;
    GVariant *uint32_variant, *tuple_variant;

    g_info ("on_handle_cancel for id 0x%" PRIx64, id);
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    connection = connection_manager_lookup_id (data->manager, id);
    if (connection == NULL) {
        g_warning ("no active connection for id: 0x%" PRIx64, id);
        return FALSE;
    }
    g_info ("canceling command for connection 0x%" PRIxPTR, (uintptr_t)connection);
    /* cancel any existing commands for the connection */
    g_object_unref (connection);
    /* setup and send return value */
    uint32_variant = g_variant_new_uint32 (TSS2_RC_SUCCESS);
    tuple_variant = g_variant_new_tuple (&uint32_variant, 1);
    g_dbus_method_invocation_return_value (invocation, tuple_variant);

    return TRUE;
}
/**
 * This is a signal handler for the handle-set-locality signal from the
 * Tabrmd DBus interface. This signal is triggered by a request
 * from a client to set the locality for TPM commands associated with the
 * connection (the 'id' parameter). This requires a few things be done:
 * - Ensure the initialization thread has completed successfully by
 *   locking and unlocking the init_mutex.
 * - Find the Connection object associated with the 'id' parameter.
 * - Set the locality for the Connection object.
 * - Pass result of the operation back to the user.
 */
static gboolean
on_handle_set_locality (TctiTabrmd            *skeleton,
                        GDBusMethodInvocation *invocation,
                        gint64                 id,
                        guint8                 locality,
                        gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    Connection *connection = NULL;
    GVariant *uint32_variant, *tuple_variant;

    g_info ("on_handle_set_locality for id 0x%" PRIx64, id);
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    connection = connection_manager_lookup_id (data->manager, id);
    if (connection == NULL) {
        g_warning ("no active connection for id: 0x%" PRIx64, id);
        return FALSE;
    }
    g_info ("setting locality for connection 0x%" PRIxPTR " to: %" PRIx8,
            (uintptr_t)connection, locality);
    /* set locality for an existing connection */
    g_object_unref (connection);
    /* setup and send return value */
    uint32_variant = g_variant_new_uint32 (TSS2_RC_SUCCESS);
    tuple_variant = g_variant_new_tuple (&uint32_variant, 1);
    g_dbus_method_invocation_return_value (invocation, tuple_variant);

    return TRUE;
}
void
dump_trans_state_callback (gpointer key,
                           gpointer value,
                           gpointer user_data)
{
    TPM_HANDLE phandle;
    HandleMapEntry *entry;

    g_debug ("dump_trans_state_callback: key: 0x%" PRIxPTR " value: 0x%"
             PRIxPTR " user_data: 0x%" PRIxPTR,
             (uintptr_t)key, (uintptr_t)value, (uintptr_t)user_data);
    phandle = (TPM_HANDLE)(uintptr_t)key;
    if (value == NULL) {
        g_warning ("got NULL entry for key: 0x%" PRIx32, phandle);
        return;
    }
    entry = HANDLE_MAP_ENTRY (value);

    g_debug ("  dump_trans_state_callback entry:   0x%" PRIxPTR,
             (uintptr_t)entry);
    g_debug ("  dump_trans_state_callback phandle: 0x%" PRIx32, phandle);
    g_debug ("  dump_trans_state_callback vhandle: 0x%" PRIx32,
             handle_map_entry_get_vhandle (entry));
    g_debug ("  dump_trans_state_callback_context: 0x%" PRIxPTR,
             (uintptr_t)handle_map_entry_get_context (entry));
}
/*
 */
static gboolean
on_handle_dump_trans_state (TctiTabrmd            *skeleton,
                            GDBusMethodInvocation *invocation,
                            gint64                 id,
                            gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    HandleMap      *map     = NULL;
    Connection   *connection = NULL;
    GVariant *uint32_variant, *tuple_variant;

    g_info ("on_handle_dump_trans_state for id 0x%" PRIx64, id);
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    connection = connection_manager_lookup_id (data->manager, id);
    if (connection == NULL)
        g_error ("no active connection for id: 0x%" PRIx64, id);
    g_info ("dumping transient handle map for for connection 0x%" PRIxPTR,
            (uintptr_t)connection);
    map = connection_get_trans_map (connection);
    g_object_unref (connection);
    g_info ("  number of entries in map: %" PRIu32, handle_map_size (map));
    handle_map_foreach (map, dump_trans_state_callback, NULL);
    g_object_unref (map);
    /* setup and send return value */
    uint32_variant = g_variant_new_uint32 (TSS2_RC_SUCCESS);
    tuple_variant = g_variant_new_tuple (&uint32_variant, 1);
    g_dbus_method_invocation_return_value (invocation, tuple_variant);

    return TRUE;
}
/**
 * This is a signal handler of type GBusAcquiredCallback. It is registered
 * by the g_bus_own_name function and invoked then a connectiont to a bus
 * is acquired in response to a request for the parameter 'name'.
 */
static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
    g_info ("on_bus_acquired: %s", name);
}
/**
 * This is a signal handler of type GBusNameAcquiredCallback. It is
 * registered by the g_bus_own_name function and invoked when the parameter
 * 'name' is acquired on the requested bus. It does 3 things:
 * - Obtains a new TctiTabrmd instance and stores a reference in
 *   the 'user_data' parameter (which is a reference to the gmain_data_t.
 * - Register signal handlers for the CreateConnection, Cancel and
 *   SetLocality signals.
 * - Export the TctiTabrmd interface (skeleton) on the DBus
 *   connection.
 */
static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
    g_info ("on_name_acquired: %s", name);
    gmain_data_t *gmain_data;
    GError *error = NULL;
    gboolean ret;

    if (user_data == NULL)
        g_error ("bus_acquired but user_data is NULL");
    gmain_data = (gmain_data_t*)user_data;
    if (gmain_data->skeleton == NULL)
        gmain_data->skeleton = tcti_tabrmd_skeleton_new ();
    g_signal_connect (gmain_data->skeleton,
                      "handle-create-connection",
                      G_CALLBACK (on_handle_create_connection),
                      user_data);
    g_signal_connect (gmain_data->skeleton,
                      "handle-cancel",
                      G_CALLBACK (on_handle_cancel),
                      user_data);
    g_signal_connect (gmain_data->skeleton,
                      "handle-set-locality",
                      G_CALLBACK (on_handle_set_locality),
                      user_data);
    g_signal_connect (gmain_data->skeleton,
                      "handle-dump-trans-state",
                      G_CALLBACK (on_handle_dump_trans_state),
                      user_data);
    ret = g_dbus_interface_skeleton_export (
        G_DBUS_INTERFACE_SKELETON (gmain_data->skeleton),
        connection,
        TABRMD_DBUS_PATH,
        &error);
    if (ret == FALSE)
        g_warning ("failed to export interface: %s", error->message);
}
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
 * This is a signal handler of type GBusNameLostCallback. It is
 * registered with the g_dbus_own_name function and is invoked when the
 * parameter 'name' is lost on the requested bus. It does one thing:
 * - Ends the GMainLoop.
 */
static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
    g_info ("on_name_lost: %s", name);

    gmain_data_t *data = (gmain_data_t*)user_data;
    main_loop_quit (data->loop);
}
/**
 * This is a very poorly named signal handling function. It is invoked in
 * response to a Unix signal. It does one thing:
 * - Shuts down the GMainLoop.
 */
static void
signal_handler (int signum)
{
    g_info ("handling signal");
    /* this is the only place the global poiner to the GMainLoop is accessed */
    main_loop_quit (g_loop);
}
/**
 * This function initializes and configures all of the long-lived objects
 * in the tabrmd system. It is invoked on a thread separate from the main
 * thread as a way to get the main thread listening for connections on
 * DBus as quickly as possible. Any incomming DBus requests will block
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
    uint32_t loaded_trans_objs;
    TSS2_RC rc;
    CommandAttrs *command_attrs;
    struct sigaction action = {
        .sa_handler = signal_handler,
        .sa_flags   = 0,
    };

    g_info ("init_thread_func start");
    g_mutex_lock (&data->init_mutex);
    /* Setup program signals */
    sigaction (SIGINT,  &action, NULL);
    sigaction (SIGTERM, &action, NULL);

    data->random = random_new();
    ret = random_seed_from_file (data->random, RANDOM_ENTROPY_FILE_DEFAULT);
    if (ret != 0)
        g_error ("failed to seed Random object");

    data->manager = connection_manager_new(data->options.max_connections);
    if (data->manager == NULL)
        g_error ("failed to allocate connection_manager");
    g_debug ("ConnectionManager: 0x%" PRIxPTR, (uintptr_t)data->manager);

    /**
     * this isn't strictly necessary but it allows us to detect a failure in
     * the TCTI before we start communicating with clients
     */
    rc = tcti_initialize (data->tcti);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to initialize TCTI: 0x%x", rc);

    data->access_broker = access_broker_new (data->tcti);
    g_debug ("created AccessBroker: 0x%" PRIxPTR,
             (uintptr_t)data->access_broker);
    rc = access_broker_init_tpm (data->access_broker);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to initialize AccessBroker: 0x%" PRIx32, rc);
    /*
     * Ensure the TPM is in a state in which we can use it w/o stepping all
     * over someone else.
     */
    rc = access_broker_get_trans_object_count (data->access_broker,
                                               &loaded_trans_objs);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to get number of loaded transient objects from "
                 "access broker 0x%" PRIxPTR " RC: 0x%" PRIx32,
                 (uintptr_t)data->access_broker,
                 rc);
    if ((loaded_trans_objs > 0) & data->options.fail_on_loaded_trans)
        g_error ("TPM reports 0x%" PRIx32 " loaded transient objects, "
                 "aborting", loaded_trans_objs);
    /**
     * Instantiate and the objects that make up the TPM command processing
     * pipeline.
     */
    command_attrs = command_attrs_new ();
    g_debug ("created CommandAttrs: 0x%" PRIxPTR, (uintptr_t)command_attrs);
    ret = command_attrs_init (command_attrs, data->access_broker);
    if (ret != 0)
        g_error ("failed to initialize CommandAttribute object: 0x%" PRIxPTR,
                 (uintptr_t)command_attrs);

    data->command_source =
        command_source_new (data->manager, command_attrs);
    g_debug ("created command source: 0x%" PRIxPTR,
             (uintptr_t)data->command_source);
    data->resource_manager = resource_manager_new (data->access_broker);
    g_debug ("created ResourceManager: 0x%" PRIxPTR,
             (uintptr_t)data->resource_manager);
    data->response_sink = SINK (response_sink_new ());
    g_debug ("created response source: 0x%" PRIxPTR,
             (uintptr_t)data->response_sink);
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

    return (void*)0;
}
/**
 * This function parses the parameter argument vector and populates the
 * parameter 'options' structure with data needed to configure the tabrmd.
 * We rely heavily on the GOption module here and we get our GOptionEntry
 * array from two places:
 * - The TctiOption module.
 * - The local application options specified here.
 * Then we do a bit of sanity checking and setting up default values if
 * none were supplied.
 */
void
parse_opts (gint            argc,
            gchar          *argv[],
            tabrmd_options_t *options)
{
    gchar *logger_name = "stdout";
    GOptionContext *ctx;
    GError *err = NULL;
    gboolean session_bus = FALSE;

    options->max_connections = MAX_CONNECTIONS_DEFAULT;
    options->max_transient_objects = MAX_TRANSIENT_OBJECTS_DEFAULT;

    GOptionEntry entries[] = {
        { "logger", 'l', 0, G_OPTION_ARG_STRING, &logger_name,
          "The name of desired logger, stdout is default.", "[stdout|syslog]"},
        { "session", 's', 0, G_OPTION_ARG_NONE, &session_bus,
          "Connect to the session bus (system bus is default)." },
        { "fail-on-loaded-trans", 't', 0, G_OPTION_ARG_NONE,
          &options->fail_on_loaded_trans,
          "Fail initialization if the TPM reports loaded transient objects" },
        { "max-connections", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &options->max_connections, "Maximum number of client connections." },
        { "max-transient-objects", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &options->max_transient_objects,
          "Maximum number of loaded transient objects per client." },
        { NULL },
    };

    g_debug ("creating tcti_options object");
    options->tcti_options = tcti_options_new ();
    ctx = g_option_context_new (" - TPM2 software stack Access Broker Daemon (tabrmd)");
    g_option_context_add_main_entries (ctx, entries, NULL);
    g_option_context_add_group (ctx, tcti_options_get_group (options->tcti_options));
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
        g_error ("Failed to parse options. %s", err->message);
    }
    /* select the bus type, default to G_BUS_TYPE_SESSION */
    options->bus = session_bus ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM;
    if (set_logger (logger_name) == -1) {
        g_error ("Unknown logger: %s, try --help\n", logger_name);
    }
    if (options->max_connections < 1 ||
        options->max_connections > MAX_CONNECTIONS)
    {
        g_error ("MAX_CONNECTIONS must be between 1 and %d", MAX_CONNECTIONS);
    }
    if (options->max_transient_objects < 1 ||
        options->max_transient_objects > MAX_TRANSIENT_OBJECTS)
    {
        g_error ("max-trans-obj parameter must be between 1 and %d",
                 MAX_TRANSIENT_OBJECTS);
    }
    g_option_context_free (ctx);
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
    guint owner_id;
    gmain_data_t gmain_data = { 0 };
    GThread *init_thread;

    g_info ("tabrmd startup");
    parse_opts (argc, argv, &gmain_data.options);
    gmain_data.tcti = tcti_options_get_tcti (gmain_data.options.tcti_options);
    if (gmain_data.tcti == NULL)
        g_error ("Invalid TCTI selected.");

    g_mutex_init (&gmain_data.init_mutex);
    g_loop = gmain_data.loop = g_main_loop_new (NULL, FALSE);
    /**
     * Initialize program data on a separate thread. The main thread needs to
     * acquire the dbus name and get into the GMainLoop ASAP to be responsive to
     * bus clients.
     */
    init_thread = g_thread_new (TABD_INIT_THREAD_NAME,
                                init_thread_func,
                                &gmain_data);
    owner_id = g_bus_own_name (gmain_data.options.bus,
                               TABRMD_DBUS_NAME,
                               G_BUS_NAME_OWNER_FLAGS_NONE,
                               on_bus_acquired,
                               on_name_acquired,
                               on_name_lost,
                               &gmain_data,
                               NULL);
    /**
     * If we call this for the first time from a thread other than the main
     * thread we get a segfault. Not sure why.
     */
    thread_get_type ();
    sink_get_type ();
    source_get_type ();
    g_info ("entering g_main_loop");
    g_main_loop_run (gmain_data.loop);
    g_info ("g_main_loop_run done, cleaning up");
    g_thread_join (init_thread);
    /* cleanup glib stuff first so we stop getting events */
    g_bus_unown_name (owner_id);
    if (gmain_data.skeleton != NULL)
        g_object_unref (gmain_data.skeleton);
    /* tear down the command processing pipeline */
    thread_cleanup (THREAD (gmain_data.command_source));
    thread_cleanup (THREAD (gmain_data.resource_manager));
    thread_cleanup (THREAD (gmain_data.response_sink));
    /* clean up what remains */
    g_object_unref (gmain_data.manager);
    g_object_unref (gmain_data.options.tcti_options);
    g_object_unref (gmain_data.random);
    g_object_unref (gmain_data.tcti);
    return 0;
}
