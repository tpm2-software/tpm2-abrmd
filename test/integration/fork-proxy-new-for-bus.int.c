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

/*
 */
int
main (void)
{
    GDBusProxy *proxy [2];
    GError *error = NULL;
    pid_t pid;
    int status;

    proxy [0] = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL,
                                               "org.freedesktop.DBus",
                                               "/org/freedesktop/DBus",
                                               "org.freedesktop.DBus.ListNames",
                                               NULL,
                                               &error);
    g_assert_no_error (error);
    /* return 0; */
    pid = fork ();
    /* crate proxy object child after fork */
    if (pid == 0) {
        g_debug ("%s: pid %d -> child", __func__, pid);
        proxy [1] = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL,
                                               "org.freedesktop.DBus",
                                               "/org/freedesktop/DBus",
                                               "org.freedesktop.DBus.ListNames",
                                               NULL,
                                               &error);
        g_assert_no_error (error);
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
