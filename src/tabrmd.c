/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
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
#include "tabrmd.h"
#include "logging.h"
#include "thread.h"
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
    ResponseSink           *response_sink;
    GMutex                  init_mutex;
    Tcti                   *tcti;
    GDBusProxy             *dbus_daemon_proxy;
    guint                   dbus_name_owner_id;
} gmain_data_t;

typedef enum {
    TABRMD_ERROR_INTERNAL         = TSS2_RESMGR_RC_INTERNAL_ERROR,
    TABRMD_ERROR_MAX_CONNECTIONS  = TSS2_RESMGR_RC_GENERAL_FAILURE,
    TABRMD_ERROR_ID_GENERATION    = TSS2_RESMGR_RC_GENERAL_FAILURE,
    TABRMD_ERROR_NOT_IMPLEMENTED  = TSS2_RESMGR_RC_NOT_IMPLEMENTED,
    TABRMD_ERROR_NOT_PERMITTED    = TSS2_RESMGR_RC_NOT_PERMITTED,
} TabrmdErrorEnum;
/* This global pointer to the GMainLoop is necessary so that we can react to
 * unix signals. Only the signal handler should touch this.
 */
static GMainLoop *g_loop;

/*
 * Callback handling the acquisition of a GDBusProxy object for communication
 * with the well known org.freedesktop.DBus object. This is an object exposed
 * by the dbus daemon.
 */
static void
on_get_dbus_daemon_proxy (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
    GError *error = NULL;
    gmain_data_t *data = (gmain_data_t*)user_data;

    data->dbus_daemon_proxy = g_dbus_proxy_new_finish (result, &error);
    if (error) {
        g_warning ("Failed to get proxy for DBus daemon "
                   "(org.freedesktop.DBus): %s", error->message);
        g_error_free (error);
        data->dbus_daemon_proxy = NULL;
    } else {
        g_debug ("Got proxy object for DBus daemon.");
    }
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
/*
 * Give this function a dbus proxy and invocation object from a method
 * invocation and it will get the PID of the process associated with the
 * invocation. If an error occurs this function returns false.
 */
static gboolean
get_pid_from_dbus_invocation (GDBusProxy            *proxy,
                              GDBusMethodInvocation *invocation,
                              guint32               *pid)
{
    const gchar *name   = NULL;
    GError      *error  = NULL;
    GVariant    *result = NULL;

    if (proxy == NULL || invocation == NULL || pid == NULL)
        return FALSE;

    name = g_dbus_method_invocation_get_sender (invocation);
    result = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                     "GetConnectionUnixProcessID",
                                     g_variant_new("(s)", name),
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     &error);
    if (error) {
        g_error ("Unable to get PID for %s: %s", name, error->message);
        g_error_free (error);
        error = NULL;
        return FALSE;
    } else {
        g_variant_get (result, "(u)", pid);
        g_variant_unref (result);
        return TRUE;
    }
}
/*
 * Generate a random uint64 returned in the id out paramter.
 * Mix this random ID with the PID from the caller. This is obtained
 * through the invocation parameter. Mix the two together using xor and
 * return the result through the id_pid_mix out parameter.
 * NOTE: if an error occurs then a response is sent through the invocation
 * to the client and FALSE is returned to the caller.
 *
 * Returns FALSE on error, TRUE otherwise.
 */
static gboolean
generate_id_pid_mix_from_invocation (GDBusMethodInvocation *invocation,
                                     gmain_data_t          *data,
                                     guint64               *id,
                                     guint64               *id_pid_mix)
{
    gboolean pid_ret = FALSE;
    guint32  pid = 0;

    pid_ret = get_pid_from_dbus_invocation (data->dbus_daemon_proxy,
                                            invocation,
                                            &pid);
    if (pid_ret == TRUE) {
        *id = random_get_uint64 (data->random);
        *id_pid_mix = *id ^ pid;
    } else {
        g_dbus_method_invocation_return_error (invocation,
                                               TABRMD_ERROR,
                                               TABRMD_ERROR_INTERNAL,
                                               "Failed to get client PID");
    }

    return pid_ret;
}
/*
 * Mix PID into provide id. Returns mixed value in id_pid_mix out parameter.
 * NOTE: if an error occurs then an error response is sent through the
 * invocation to the client and FALSE is returned to the caller
 *
 * Returns FALSE on error, TRUE otherwise.
 */
static gboolean
get_id_pid_mix_from_invocation (GDBusProxy            *proxy,
                                GDBusMethodInvocation *invocation,
                                guint64                id,
                                guint64               *id_pid_mix)
{
    guint32 pid = 0;
    gboolean pid_ret = FALSE;

    g_debug ("get_id_pid_mix_from_invocation");
    pid_ret = get_pid_from_dbus_invocation (proxy,
                                            invocation,
                                            &pid);
    g_debug ("id 0x%" PRIx64 " pid: 0x%" PRIx32, id, pid);
    if (pid_ret == TRUE) {
        *id_pid_mix = id ^ pid;
        g_debug ("mixed: 0x%" PRIx64, *id_pid_mix);
    } else {
        g_dbus_method_invocation_return_error (
            invocation,
            TABRMD_ERROR,
            TABRMD_ERROR_INTERNAL,
            "Failed to get client PID");
    }

    return pid_ret;
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
    guint64 id = 0, id_pid_mix = 0;
    gboolean id_ret = FALSE;

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
    id_ret = generate_id_pid_mix_from_invocation (invocation,
                                                  data,
                                                  &id,
                                                  &id_pid_mix);
    /* error already returned to caller over dbus */
    if (id_ret == FALSE) {
        return TRUE;
    }
    g_debug ("Creating connection with id: 0x%" PRIx64, id_pid_mix);
    if (connection_manager_contains_id (data->manager, id_pid_mix)) {
        g_warning ("ID collision in ConnectionManager: %" PRIu64, id_pid_mix);
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
    connection = connection_new (&client_fds[0], &client_fds[1], id_pid_mix, handle_map);
    g_object_unref (handle_map);
    if (connection == NULL)
        g_error ("Failed to allocate new connection.");
    g_debug ("Created connection with fds: %d, %d and id: 0x%" PRIx64,
             client_fds[0], client_fds[1], id_pid_mix);
    /* prepare tuple variant for response message */
    fd_list = g_unix_fd_list_new_from_array (client_fds, 2);
    response_variants[0] = handle_array_variant_from_fdlist (fd_list);
    /* return the random id to client, *not* xor'd with PID */
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
    guint64   id_pid_mix = 0;
    gboolean mix_ret = FALSE;

    g_info ("on_handle_cancel for id 0x%" PRIx64, id);
    mix_ret = get_id_pid_mix_from_invocation (data->dbus_daemon_proxy,
                                              invocation,
                                              id,
                                              &id_pid_mix);
    /* error already sent over dbus */
    if (mix_ret == FALSE) {
        return TRUE;
    }
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    connection = connection_manager_lookup_id (data->manager, id_pid_mix);
    if (connection == NULL) {
        g_warning ("no active connection for id_pid_mix: 0x%" PRIx64,
                   id_pid_mix);
        g_dbus_method_invocation_return_error (invocation,
                                               TABRMD_ERROR,
                                               TABRMD_ERROR_NOT_PERMITTED,
                                               "No connection.");
        return TRUE;
    }
    g_info ("canceling command for connection 0x%" PRIxPTR " with "
            "id_pid_mix: 0x%" PRIx64, (uintptr_t)connection, id_pid_mix);
    /* cancel any existing commands for the connection */
    g_dbus_method_invocation_return_error (invocation,
                                           TABRMD_ERROR,
                                           TABRMD_ERROR_NOT_IMPLEMENTED,
                                           "Cancel function not implemented.");
    g_object_unref (connection);

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
    guint64   id_pid_mix = 0;
    gboolean mix_ret = FALSE;

    g_info ("on_handle_set_locality for id 0x%" PRIx64, id);
    mix_ret = get_id_pid_mix_from_invocation (data->dbus_daemon_proxy,
                                              invocation,
                                              id,
                                              &id_pid_mix);
    /* error already sent over dbus */
    if (mix_ret == FALSE) {
        return TRUE;
    }
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    connection = connection_manager_lookup_id (data->manager, id_pid_mix);
    if (connection == NULL) {
        g_warning ("no active connection for id_pid_mix: 0x%" PRIx64,
                   id_pid_mix);
        g_dbus_method_invocation_return_error (invocation,
                                               TABRMD_ERROR,
                                               TABRMD_ERROR_NOT_PERMITTED,
                                               "No connection.");
        return TRUE;
    }
    g_info ("setting locality for connection 0x%" PRIxPTR " with "
            "id_pid_mix: 0x%" PRIx64 " to: %" PRIx8,
            (uintptr_t)connection, id_pid_mix, locality);
    /* set locality for an existing connection */
    g_dbus_method_invocation_return_error (invocation,
                                           TABRMD_ERROR,
                                           TABRMD_ERROR_NOT_IMPLEMENTED,
                                           "setLocality function not implemented.");
    g_object_unref (connection);

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

    data->dbus_name_owner_id = g_bus_own_name (data->options.bus,
                                               data->options.dbus_name,
                                               G_BUS_NAME_OWNER_FLAGS_NONE,
                                               on_bus_acquired,
                                               on_name_acquired,
                                               on_name_lost,
                                               data,
                                               NULL);
    g_dbus_proxy_new_for_bus (data->options.bus,
                              G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                              NULL,
                              "org.freedesktop.DBus",
                              "/org/freedesktop/DBus",
                              "org.freedesktop.DBus",
                              NULL,
                              (GAsyncReadyCallback)on_get_dbus_daemon_proxy,
                              user_data);
    data->random = random_new();
    ret = random_seed_from_file (data->random, data->options.prng_seed_file);
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
    if (rc != TSS2_RC_SUCCESS) {
        tabrmd_critical ("TCTI initialization failed: 0x%x", rc);
    }

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
    if ((loaded_trans_objs > 0) & data->options.fail_on_loaded_trans) {
        tabrmd_critical ("TPM reports 0x%" PRIx32 " loaded transient objects, "
                         "aborting", loaded_trans_objs);
    }
    /**
     * Instantiate and the objects that make up the TPM command processing
     * pipeline.
     */
    command_attrs = command_attrs_new ();
    g_debug ("created CommandAttrs: 0x%" PRIxPTR, (uintptr_t)command_attrs);
    ret = command_attrs_init_tpm (command_attrs, data->access_broker);
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
    data->response_sink = response_sink_new ();
    g_debug ("created response source: 0x%" PRIxPTR,
             (uintptr_t)data->response_sink);
    g_object_unref (command_attrs);
    g_object_unref (data->access_broker);
    /*
     * Connect the ResourceManager to the ConnectionManager
     * 'connection-removed' signal.
     */
    g_signal_connect (data->manager,
                      "connection-removed",
                      G_CALLBACK (resource_manager_on_connection_removed),
                      data->resource_manager);
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

    return NULL;
}
/*
 * This is a GOptionArgFunc callback invoked from the GOption processor from
 * the parse_opts function below. It will be called when the daemon is
 * invoked with the -v/--version option. This will cause the daemon to
 * display a version string and exit.
 */
gboolean
show_version (const gchar  *option_name,
              const gchar  *value,
              gpointer      data,
              GError      **error)
{
    g_print ("tpm2-abrmd version %s\n", VERSION);
    exit (0);
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
    options->dbus_name = TABRMD_DBUS_NAME_DEFAULT;
    options->prng_seed_file = RANDOM_ENTROPY_FILE_DEFAULT;

    GOptionEntry entries[] = {
        { "dbus-name", 'n', 0, G_OPTION_ARG_STRING, &options->dbus_name,
          "Name for daemon to \"own\" on the D-Bus",
          TABRMD_DBUS_NAME_DEFAULT },
        { "logger", 'l', 0, G_OPTION_ARG_STRING, &logger_name,
          "The name of desired logger, stdout is default.", "[stdout|syslog]"},
        { "session", 's', 0, G_OPTION_ARG_NONE, &session_bus,
          "Connect to the session bus (system bus is default)." },
        { "fail-on-loaded-trans", 'i', 0, G_OPTION_ARG_NONE,
          &options->fail_on_loaded_trans,
          "Fail initialization if the TPM reports loaded transient objects" },
        { "max-connections", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &options->max_connections, "Maximum number of client connections." },
        { "max-transient-objects", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &options->max_transient_objects,
          "Maximum number of loaded transient objects per client." },
        { "prng-seed-file", 'g', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
          &options->prng_seed_file, "File to read seed value for PRNG",
          options->prng_seed_file },
        { "version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          show_version, "Show version string" },
        { NULL },
    };

    ctx = g_option_context_new (" - TPM2 software stack Access Broker Daemon (tabrmd)");
    g_option_context_add_main_entries (ctx, entries, NULL);
    g_option_context_add_group (ctx, tcti_options_get_group (options->tcti_options));
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
        tabrmd_critical ("Failed to parse options: %s", err->message);
    }
    /* select the bus type, default to G_BUS_TYPE_SESSION */
    options->bus = session_bus ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM;
    if (set_logger (logger_name) == -1) {
        tabrmd_critical ("Unknown logger: %s, try --help\n", logger_name);
    }
    if (options->max_connections < 1 ||
        options->max_connections > MAX_CONNECTIONS)
    {
        tabrmd_critical ("MAX_CONNECTIONS must be between 1 and %d",
                         MAX_CONNECTIONS);
    }
    if (options->max_transient_objects < 1 ||
        options->max_transient_objects > MAX_TRANSIENT_OBJECTS)
    {
        tabrmd_critical ("max-trans-obj parameter must be between 1 and %d",
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
    gmain_data_t gmain_data = { 0 };
    GThread *init_thread;

    g_info ("tabrmd startup");
    /* instantiate a TctiOptions object for the parse_opts function to use */
    gmain_data.options.tcti_options = tcti_options_new ();
    parse_opts (argc, argv, &gmain_data.options);
    gmain_data.tcti = tcti_options_get_tcti (gmain_data.options.tcti_options);

    g_mutex_init (&gmain_data.init_mutex);
    g_loop = gmain_data.loop = g_main_loop_new (NULL, FALSE);
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
    g_thread_join (init_thread);
    /* cleanup glib stuff first so we stop getting events */
    g_bus_unown_name (gmain_data.dbus_name_owner_id);
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
