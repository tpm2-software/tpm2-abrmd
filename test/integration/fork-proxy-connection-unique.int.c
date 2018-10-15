/*
 * Copyright 2018, Intel Corporation
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

GDBusProxy*
create_proxy_simple (void)
{
    GDBusProxy *proxy;

    proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NULL,
                                           "org.freedesktop.DBus",
                                           "/org/freedesktop/DBus",
                                           "org.freedesktop.DBus.ListNames",
                                           NULL,
                                           &error);
    g_assert_no_error (error);

    return proxy;
}

GDBusProxy*
create_proxy_via_addr_conn (void)
{
    GDBusConnection *connection;
    GDBusProxy *proxy;
    GError *error = NULL;
    gboolean ret;
    char *address;

    address = g_dbus_address_get_for_bus_sync (G_BUS_TYPE_SESSION, NULL, &error);
    g_assert_no_error (error);

    g_debug ("%s: g_dbus_connection_new_for_address_sync: %s", __func__, address);
    connection = g_dbus_connection_new_for_address_sync (
        address,
        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
        NULL,
        NULL,
        &error);
    g_assert_no_error (error);

    g_debug ("%s: g_initable_init", __func__);
    ret = g_initable_init (G_INITABLE (connection), NULL, &error);
    g_assert_no_error (error);
    if (ret == FALSE)
        g_error ("g_initable_init returned FALSE");

    g_debug ("%s: g_dbus_proxy_new_sync", __func__);
    proxy = g_dbus_proxy_new_sync (connection,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                   "org.freedesktop.DBus",
                                   "/org/freedesktop/DBus",
                                   "org.freedesktop.DBus.ListNames",
                                   NULL,
                                   &error);
    g_assert_no_error (error);

    g_clear_object (&connection);
    g_free (address);

    return proxy;
}
/*
 */
int
main (void)
{
    GDBusProxy *proxy;
    pid_t pid;
    int status;

    proxy = create_proxy_via_addr_conn ();
    pid = fork ();
    /* crate proxy object child after fork */
    if (pid == 0) {
        g_debug ("%s: pid %d -> child", __func__, pid);
        proxy = create_proxy_via_addr_conn ();
    } else {
        /* parent waitpid on child, check exit & signal status */
        g_debug ("%s: pid %d -> parent", __func__, pid);
        if (waitpid (pid, &status, 0) == -1) {
            perror ("waitpid");
            g_error ("%s:%d: waitpid failed: ", __func__, pid);
        } else if (WEXITSTATUS (status)) {
            g_error("%s:%d: child exit status %d", __func__, pid, status);
        } else if (WIFSIGNALED (status)) {
            g_error("%s:%d: child was killed by signal %d", __func__, pid, status);
        }
    }

    g_debug ("%s:%d Test succeeded.", __func__, pid);
    return 0;
}
