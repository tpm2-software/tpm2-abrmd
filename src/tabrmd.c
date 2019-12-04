/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <tss2/tss2_tpm2_types.h>

#include "tabrmd-options.h"
#include "tabrmd-init.h"
#include "tabrmd.h"
#include "util.h"

/*
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
    gint ret;

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
    ret = GPOINTER_TO_INT (g_thread_join (init_thread));
    if (ret == 0 && gmain_data.ipc_disconnected) {
        ret = EX_IOERR;
    }
    gmain_data_cleanup (&gmain_data);
    return ret;
}
