#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>

#include <tss2-tabd.h>
#include "session-manager.h"
#include "session-watcher.h"
#include "session.h"

#ifdef G_OS_UNIX
#include <gio/gunixfdlist.h>
#endif

/* Structure to hold data that we pass to the gmain loop as 'user_data'.
 * This data will be available to events from gmain including events from
 * the DBus.
 */
typedef struct gmain_data {
    GMainLoop *loop;
    GDBusNodeInfo *introspection_data;
    session_manager_t *manager;
    gint wakeup_send_fd;
} gmain_data_t;

/* This global pointer to the GMainLoop is necessary so that we can react to
 * unix signals. Only the signal handler should touch this.
 */
static GMainLoop *g_loop;

/* This must be global so that our signal handlers can interact with it. */
static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='us.twobit.tss2.TabdInterface'>"
  "    <method name='CreateConnection'>"
  "      <arg type='ah' name='fds' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  g_info ("handle_method_call");
  gmain_data_t *data = (gmain_data_t*)user_data;
  if (data == NULL)
      g_error ("handle_method_call: user_data is NULL");
  if (g_strcmp0 (method_name, "CreateConnection") == 0)
    {
      /* create connection method*/
#ifdef G_OS_UNIX
      if (g_dbus_connection_get_capabilities (connection) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING)
        {
          GDBusMessage *reply = NULL;
          GUnixFDList *fd_list = NULL;
          GError *error = NULL;

          gint client_fds[2] = { 0, 0 }, ret = 0;
          session_t *session = session_new (&client_fds[0], &client_fds[1]);
          if (session == NULL)
              g_error ("Failed to allocate new session.");
          g_debug ("Returning two fds: %d, %d", client_fds[0], client_fds[1]);
          fd_list = g_unix_fd_list_new_from_array (client_fds, 2);
          g_assert_no_error (error);

          reply = g_dbus_message_new_method_reply (g_dbus_method_invocation_get_message (invocation));
          g_dbus_message_set_unix_fd_list (reply, fd_list);

          error = NULL;
          g_dbus_connection_send_message (connection,
                                          reply,
                                          G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                          NULL, /* out_serial */
                                          &error);
          g_assert_no_error (error);

          ret = session_manager_insert (data->manager, session);
          if (ret != 0)
              g_error ("Failed to add new session to session_manager.");
          ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
          if (ret < 1)
              g_error ("failed to wakeup watcher");

          g_object_unref (invocation);
          g_object_unref (fd_list);
          g_object_unref (reply);
        }
      else
        {
          g_dbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.Failed",
                                                      "Your message bus daemon does not support file descriptor passing (need D-Bus >= 1.3.0)");
        }
#else
      g_dbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.NotOnUnix",
                                                  "Your OS does not support file descriptor passing");
#endif
    }
  else
    {
      g_dbus_method_invocation_return_error (invocation,
                                             G_DBUS_ERROR,
                                             G_DBUS_ERROR_UNKNOWN_METHOD,
                                             "Unknown method.");
    }
}

static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
    g_info ("on_bus_acquired: %s", name);
    guint registration_id;
    gmain_data_t *gmain_data;

    if (user_data == NULL)
        g_error ("bus_acquired but user_data is NULL");
    gmain_data = (gmain_data_t*)user_data;

    registration_id =
        g_dbus_connection_register_object (
            connection,
            TAB_DBUS_PATH,
            gmain_data->introspection_data->interfaces[0],
            &interface_vtable,
            user_data,  /* user_data */
            NULL,  /* user_data_free_func */
            NULL); /* GError** */
    g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  g_info ("on_name_acquired: %s", name);
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

  gmain_data.introspection_data =
      g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (gmain_data.introspection_data != NULL);

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

  g_main_loop_run (gmain_data.loop);
  g_info ("g_main_loop_run done, cleaning up");
  /* cleanup glib stuff first so we stop getting events */
  g_bus_unown_name (owner_id);
  g_dbus_node_info_unref (gmain_data.introspection_data);
  /* stop the connection watcher thread */
  session_watcher_cancel (watcher);
  session_watcher_join (watcher);
  session_watcher_free (watcher);
  session_manager_free (gmain_data.manager);

  return 0;
}
