#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tss2/tpm20.h>
#include <tss2-tabd.h>
#include "tss2-tabd-priv.h"
#include "tss2-tabd-logging.h"
#include "data-message.h"
#include "session-manager.h"
#include "session-watcher.h"
#include "session-data.h"
#include "response-watcher.h"
#include "tss2-tabd-generated.h"

#ifdef G_OS_UNIX
#include <gio/gunixfdlist.h>
#endif

/* Structure to hold data that we pass to the gmain loop as 'user_data'.
 * This data will be available to events from gmain including events from
 * the DBus.
 */
typedef struct gmain_data {
    GMainLoop *loop;
    Tab *skeleton;
    session_manager_t *manager;
    session_watcher_t *session_watcher;
    response_watcher_t *response_watcher;
    tab_t *tab;
    gint wakeup_send_fd;
    struct drand48_data rand_data;
    GMutex init_mutex;
} gmain_data_t;

/* This global pointer to the GMainLoop is necessary so that we can react to
 * unix signals. Only the signal handler should touch this.
 */
static GMainLoop *g_loop;

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

static gboolean
on_handle_create_connection (Tab                   *skeleton,
                             GDBusMethodInvocation *invocation,
                             gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    session_data_t *session = NULL;
    gint client_fds[2] = { 0, 0 }, ret = 0;
    GVariant *response_variants[2], *response_tuple;
    GUnixFDList *fd_list = NULL;
    guint64 id;

    /* make sure the init thread is done before we create new connections
     */
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    lrand48_r (&data->rand_data, &id);
    session = session_data_new (&client_fds[0], &client_fds[1], id);
    if (session == NULL)
        g_error ("Failed to allocate new session.");
    g_debug ("Created connection with fds: %d, %d and id: %ld",
             client_fds[0], client_fds[1], id);
    /* prepare tuple variant for response message */
    fd_list = g_unix_fd_list_new_from_array (client_fds, 2);
    response_variants[0] = handle_array_variant_from_fdlist (fd_list);
    response_variants[1] = g_variant_new_uint64 (id);
    response_tuple = g_variant_new_tuple (response_variants, 2);
    /* send response */
    g_dbus_method_invocation_return_value_with_unix_fd_list (
        invocation,
        response_tuple,
        fd_list);
    g_object_unref (fd_list);
    /* add session to manager and signal manager to wakeup! */
    ret = session_manager_insert (data->manager, session);
    if (ret != 0)
        g_error ("Failed to add new session to session_manager.");
    ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    if (ret < 1)
        g_error ("failed to wakeup watcher");

    return TRUE;
}

static gboolean
on_handle_cancel (Tab                   *skeleton,
                  GDBusMethodInvocation *invocation,
                  gint64                 id,
                  gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    session_data_t *session;
    GVariant *uint32_variant, *tuple_variant;

    g_info ("on_handle_cancel for id 0x%x", id);
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    session = session_manager_lookup_id (data->manager, id);
    if (session == NULL) {
        g_warning ("no active session for id: 0x%x", id);
        return FALSE;
    }
    g_info ("canceling command for session 0x%x", session);
    /* cancel any existing commands for the session */
    /* setup and send return value */
    uint32_variant = g_variant_new_uint32 (TSS2_RC_SUCCESS);
    tuple_variant = g_variant_new_tuple (&uint32_variant, 1);
    g_dbus_method_invocation_return_value (invocation, tuple_variant);

    return TRUE;
}

static gboolean
on_handle_set_locality (Tab                   *skeleton,
                        GDBusMethodInvocation *invocation,
                        gint64                 id,
                        guint8                 locality,
                        gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    session_data_t *session;
    GVariant *uint32_variant, *tuple_variant;

    g_info ("on_handle_set_locality for id 0x%x", id);
    g_mutex_lock (&data->init_mutex);
    g_mutex_unlock (&data->init_mutex);
    session = session_manager_lookup_id (data->manager, id);
    if (session == NULL) {
        g_warning ("no active session for id: 0x%x", id);
        return FALSE;
    }
    g_info ("setting locality for session 0x%x to: 0x%x",
            session, locality);
    /* cancel any existing commands for the session */
    /* setup and send return value */
    uint32_variant = g_variant_new_uint32 (TSS2_RC_SUCCESS);
    tuple_variant = g_variant_new_tuple (&uint32_variant, 1);
    g_dbus_method_invocation_return_value (invocation, tuple_variant);

    return TRUE;
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
    g_info ("on_bus_acquired: %s", name);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
    g_info ("on_name_acquired: %s", name);
    guint registration_id;
    gmain_data_t *gmain_data;
    GError *error = NULL;
    gboolean ret;

    if (user_data == NULL)
        g_error ("bus_acquired but user_data is NULL");
    gmain_data = (gmain_data_t*)user_data;
    if (gmain_data->skeleton == NULL)
        gmain_data->skeleton = tab_skeleton_new ();
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
        TAB_DBUS_PATH,
        &error);
    if (ret == FALSE)
        g_warning ("failed to export interface: %s", error->message);
}

static void
main_loop_quit (GMainLoop *loop)
{
  g_info ("main_loop_quit");
  if (loop && g_main_loop_is_running (loop))
    g_main_loop_quit (loop);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  g_info ("on_name_lost: %s", name);

  gmain_data_t *data = (gmain_data_t*)user_data;
  main_loop_quit (data->loop);
}

static void
signal_handler (int signum)
{
  g_info ("handling signal");
  /* this is the only place the global poiner to the GMainLoop is accessed */
  main_loop_quit (g_loop);
}

static int
seed_rand_data (const char *fname, struct drand48_data *rand_data)
{
    gint rand_fd;
    long int rand_seed;
    ssize_t read_ret;

    /* seed rand with some entropy from TABD_RAND_FILE */
    g_debug ("opening entropy source: %s", fname);
    rand_fd = open (fname, O_RDONLY);
    if (rand_fd == -1) {
        g_warning ("failed to open entropy source %s: %s",
                   fname,
                   strerror (errno));
        return 1;
    }
    g_debug ("reading from entropy source: %s", fname);
    read_ret = read (rand_fd, &rand_seed, sizeof (rand_seed));
    if (read_ret == -1) {
        g_warning ("failed to read from entropy source %s, %s",
                   fname,
                   strerror (errno));
        return 1;
    }
    g_debug ("seeding rand with %ld", rand_seed);
    srand48_r (rand_seed, rand_data);

    return 0;
}

static gpointer
init_thread_func (gpointer user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    gint wakeup_fds[2] = { 0 }, ret;

    g_info ("init_thread_func start");
    g_mutex_lock (&data->init_mutex);
    /* Setup program signals */
    signal (SIGINT, signal_handler);
    signal (SIGTERM, signal_handler);

    if (seed_rand_data (TABD_RAND_FILE, &data->rand_data) != 0)
        g_error ("failed to seed random number generator");

    data->manager = session_manager_new();
    if (data->manager == NULL)
        g_error ("failed to allocate connection_manager");
    if (pipe2 (wakeup_fds, O_CLOEXEC) != 0)
        g_error ("failed to make wakeup socket: %s", strerror (errno));
    data->wakeup_send_fd = wakeup_fds [1];

    data->tab = tab_new (NULL);
    ret = tab_start (data->tab);
    if (ret != 0)
        g_error ("failed to start tab_t: %s", strerror (errno));
    data->session_watcher =
        session_watcher_new (data->manager, wakeup_fds [0], data->tab);
    data->response_watcher =
        response_watcher_new (data->tab);

    ret = session_watcher_start (data->session_watcher);
    if (ret != 0)
        g_error ("failed to start connection_watcher");
    ret = response_watcher_start (data->response_watcher);
    if (ret != 0)
        g_error ("failed to start response_watcher");

    g_mutex_unlock (&data->init_mutex);
    g_info ("init_thread_func done");
}

gint
parse_opts (gint            argc,
            gchar          *argv[],
            tabd_options_t *options)
{
    gchar *logger_name = "stdout";
    GOptionContext *ctx;
    GError *err = NULL;
    gboolean system_bus = FALSE;

    GOptionEntry entries[] = {
        { "logger", 'l', 0, G_OPTION_ARG_STRING, &logger_name,
          "The name of desired logger, stdout is default.", "[stdout|syslog]"},
        { "system", 's', 0, G_OPTION_ARG_NONE, &system_bus,
          "Connect to the system dbus." },
        { NULL },
    };
    ctx = g_option_context_new (" - TPM2 software stack Access Broker Daemon (tabd)");
    g_option_context_add_main_entries (ctx, entries, NULL);
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
        g_print ("Failed to initialize: %s\n", err->message);
        g_clear_error (&err);
        g_option_context_free (ctx);
        return 1;
    }
    /* select the bus type, default to G_BUS_TYPE_SESSION */
    options->bus = system_bus ? G_BUS_TYPE_SYSTEM : G_BUS_TYPE_SESSION;
    if (set_logger (logger_name) == -1) {
        g_print ("Unknown logger: %s, try --help\n", logger_name);
        return 1;
    }
    g_option_context_free (ctx);

    return 0;
}

int
main (int argc, char *argv[])
{
  g_info ("tabd startup");
  guint owner_id;
  gmain_data_t gmain_data = { 0 };
  GThread *init_thread;
  tabd_options_t options = { 0 };

  if (parse_opts (argc, argv, &options) != 0)
      return 1;

  g_mutex_init (&gmain_data.init_mutex);
  g_loop = gmain_data.loop = g_main_loop_new (NULL, FALSE);
  /* Initialize program data on a separate thread. The main thread needs to
   * acquire the dbus name and get into the GMainLoop ASAP to be responsive to
   * bus clients.
   */
  init_thread = g_thread_new (TABD_INIT_THREAD_NAME,
                              init_thread_func,
                              &gmain_data);
  owner_id = g_bus_own_name (options.bus,
                             TAB_DBUS_NAME,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             &gmain_data,
                             NULL);

  g_info ("entering g_main_loop");
  g_main_loop_run (gmain_data.loop);
  g_info ("g_main_loop_run done, cleaning up");
  g_thread_join (init_thread);
  /* cleanup glib stuff first so we stop getting events */
  g_bus_unown_name (owner_id);
  if (gmain_data.skeleton != NULL)
      g_object_unref (gmain_data.skeleton);
  /* stop the connection watcher thread */
  session_watcher_cancel (gmain_data.session_watcher);
  session_watcher_join (gmain_data.session_watcher);
  session_watcher_free (gmain_data.session_watcher);
  session_manager_free (gmain_data.manager);
  response_watcher_cancel (gmain_data.response_watcher);
  response_watcher_join (gmain_data.response_watcher);
  response_watcher_free (gmain_data.response_watcher);
  tab_cancel (gmain_data.tab);
  tab_join (gmain_data.tab);
  tab_free (gmain_data.tab);

  return 0;
}
