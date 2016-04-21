#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>

#include <tss2-tabd.h>
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
    gint wakeup_send_fd;
} gmain_data_t;

/* This global pointer to the GMainLoop is necessary so that we can react to
 * unix signals. Only the signal handler should touch this.
 */
static GMainLoop *g_loop;

// recursively iterate a container
static void
iterate_container_recursive (GVariant *container)
{
    GVariantIter iter;
    GVariant *child;
    gint fd = 0;

    g_variant_iter_init (&iter, container);
    while ((child = g_variant_iter_next_value (&iter))) {
        g_print ("type '%s'\n", g_variant_get_type_string (child));
        g_variant_get (child, "h", &fd);
        g_print ("value '%d'\n", fd);

        if (g_variant_is_container (child))
            iterate_container_recursive (child);

        g_variant_unref (child);
    }
}

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

int
main (int argc, char *argv[])
{
  g_info ("tabd startup");
  guint owner_id;
  gint wakeup_fds[2] = { 0 }, ret;
  gmain_data_t gmain_data = { 0 };

  /* This initialization should be done on a separate thread. Our goal here
   * is to get into the g_main_loop_run as fast as possible so we can become
   * responsive to events on the DBus.
   */
  /* Bot the gmain thread and the session_watcher thread share a connection
   * manager. The gmain thread uses the session_manager to create new
   * connections in response to DBus events. The session_watcher thread
   * monitors incoming data from clients via the session_manager.
   */
  gmain_data.manager = session_manager_new();
  if (gmain_data.manager == NULL)
      g_error ("failed to allocate connection_manager");
  if (pipe2 (wakeup_fds, O_CLOEXEC) != 0)
      g_error ("failed to make wakeup socket: %s", strerror (errno));
  gmain_data.wakeup_send_fd = wakeup_fds [1];

  session_watcher_t *watcher =
      session_watcher_new (gmain_data.manager, wakeup_fds [0]);

  g_loop = gmain_data.loop = g_main_loop_new (NULL, FALSE);
  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             TAB_DBUS_NAME,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             &gmain_data,
                             NULL);

  ret = session_watcher_start (watcher);
  if (ret != 0)
      g_error ("failed to start connection_watcher");

  /* Setup program signals */
  signal (SIGINT, signal_handler);
  signal (SIGTERM, signal_handler);

  g_info ("entering g_main_loop");
  g_main_loop_run (gmain_data.loop);
  g_info ("g_main_loop_run done, cleaning up");
  /* cleanup glib stuff first so we stop getting events */
  g_bus_unown_name (owner_id);
  if (gmain_data.skeleton != NULL)
      g_object_unref (gmain_data.skeleton);
  /* stop the connection watcher thread */
  session_watcher_cancel (watcher);
  session_watcher_join (watcher);
  session_watcher_free (watcher);
  session_manager_free (gmain_data.manager);

  return 0;
}
