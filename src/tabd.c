#include <gio/gio.h>
#include <glib.h>
#include <stdlib.h>

#ifdef G_OS_UNIX
#include <gio/gunixfdlist.h>
#endif

#define TAB_DBUS_NAME "us.twobit.tss2.Tab"
#define TAB_DBUS_PATH "/us/twobit/tss2/Tab"
static GDBusNodeInfo *introspection_data = NULL;
static GMainLoop *loop = NULL;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='us.twobit.tss2.TabInterface'>"
  "    <method name='CreateConnection'>"
  "      <arg type='h' name='fd' direction='out'/>"
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
  if (g_strcmp0 (method_name, "CreateConnection") == 0)
    {
      /* create connection method*/
#ifdef G_OS_UNIX
      if (g_dbus_connection_get_capabilities (connection) & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING)
        {
          GDBusMessage *reply = NULL;
          GUnixFDList *fd_list = NULL;
          GError *error = NULL;

          int fds[2] = { 0, 0 }, ret = 0;
          ret = CreateConnection (user_data, &fds);
          fd_list = g_unix_fd_list_new_from_array (fds, 2);
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

  registration_id = g_dbus_connection_register_object (connection,
                                                       TAB_DBUS_PATH,
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL,  /* user_data */
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
  main_loop_quit ((GMainLoop*)user_data);
}

static void
signal_handler (int signum)
{
  g_info ("handling signal");
  main_loop_quit (loop);
}

int
main (int argc, char *argv[])
{
  g_info ("tabd startup");
  guint owner_id;

  /* Setup program signals */
  signal (SIGINT, signal_handler);
  signal (SIGTERM, signal_handler);
  /* We are lazy here - we don't want to manually provide
   * the introspection data structures - so we just build
   * them from XML.
   */
  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  loop = g_main_loop_new (NULL, FALSE);
  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             TAB_DBUS_NAME,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             loop,
                             NULL);
  g_main_loop_run (loop);

  g_info ("g_main_loop_run done, cleaning up");
  g_bus_unown_name (owner_id);
  g_dbus_node_info_unref (introspection_data);

  return 0;
}
