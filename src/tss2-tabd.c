#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tss2-tabd.h>
#include "tss2-tabd-priv.h"
#include "session-manager.h"
#include "session-watcher.h"
#include "session-data.h"
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
    session_watcher_t *watcher;
    gint wakeup_send_fd;
    struct drand48_data rand_data;
    GMutex init_mutex;
} gmain_data_t;

/* This global pointer to the GMainLoop is necessary so that we can react to
 * unix signals. Only the signal handler should touch this.
 */
static GMainLoop *g_loop;

/* This funcion creates a GVariant with type 'tuple'. In this tuple we place
 * another GVAriant with type 'array'. In this array we store GVariants with
 * type 'handle'. There will be nfd number of handle GVariants in this array.
 * Each handle corresponds to an FD in a GUnixFDList that is to be sent across
 * the DBus via g_dbus_method_invocation_return_value_with_unix_fd_list.
 */
static GVariant*
get_tuple_of_array_of_handles (gint nfd)
{
    GVariant *fd_tuple = NULL, *fd_array = NULL, **fds;
    gint i = 0;

    fds = calloc (nfd, sizeof (GVariant*));
    for (i = 0; i < nfd; ++i) {
        *(fds + i) = g_variant_new_handle (i);
    }
    fd_array = g_variant_new_array (NULL, fds, nfd);
    fd_tuple = g_variant_new_tuple (&fd_array, 1);
    return fd_tuple;
}

static gboolean
on_handle_create_connection (Tab                   *skeleton,
                             GDBusMethodInvocation *invocation,
                             gpointer               user_data)
{
    gmain_data_t *data = (gmain_data_t*)user_data;
    session_data_t *session = NULL;
    gint client_fds[2] = { 0, 0 }, ret = 0;
    GVariant *fd_tuple = NULL;
    GUnixFDList *fd_list = NULL;

    session = session_data_new (&client_fds[0], &client_fds[1]);
    if (session == NULL)
        g_error ("Failed to allocate new session.");
    g_debug ("Returning two fds: %d, %d", client_fds[0], client_fds[1]);
    fd_list = g_unix_fd_list_new_from_array (client_fds, 2);
    fd_tuple =
        get_tuple_of_array_of_handles (g_unix_fd_list_get_length (fd_list));
    g_dbus_method_invocation_return_value_with_unix_fd_list (
        invocation,
        fd_tuple,
        fd_list);
    /* make sure the init thread is done before we add the session to the
     * session manager
     */
    while (g_mutex_trylock (&data->init_mutex) != FALSE);
    g_mutex_unlock (&data->init_mutex);
    /* add session to manager and signal manager to wakeup! */
    ret = session_manager_insert (data->manager, session);
    if (ret != 0)
        g_error ("Failed to add new session to session_manager.");
    ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    if (ret < 1)
        g_error ("failed to wakeup watcher");

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

    data->watcher =
        session_watcher_new (data->manager, wakeup_fds [0]);

    ret = session_watcher_start (data->watcher);
    if (ret != 0)
        g_error ("failed to start connection_watcher");

    g_mutex_unlock (&data->init_mutex);
    g_info ("init_thread_func done");
}

int
main (int argc, char *argv[])
{
  g_info ("tabd startup");
  guint owner_id;
  gmain_data_t gmain_data = { 0 };
  GThread *init_thread;

  g_mutex_init (&gmain_data.init_mutex);
  g_loop = gmain_data.loop = g_main_loop_new (NULL, FALSE);
  /* Initialize program data on a separate thread. The main thread needs to
   * acquire the dbus name and get into the GMainLoop ASAP to be responsive to
   * bus clients.
   */
  init_thread = g_thread_new (TABD_INIT_THREAD_NAME,
                              init_thread_func,
                              &gmain_data);
  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
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
  session_watcher_cancel (gmain_data.watcher);
  session_watcher_join (gmain_data.watcher);
  session_watcher_free (gmain_data.watcher);
  session_manager_free (gmain_data.manager);

  return 0;
}
