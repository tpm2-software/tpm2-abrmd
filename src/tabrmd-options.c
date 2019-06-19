/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Intel Corporation
 */
#include <glib.h>
#include <tss2/tss2_tpm2_types.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "tabrmd-options.h"
#include "util.h"

/* work around older glib versions missing this symbol */
#ifndef G_OPTION_FLAG_NONE
#define G_OPTION_FLAG_NONE 0
#endif

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
    UNUSED_PARAM(option_name);
    UNUSED_PARAM(value);
    UNUSED_PARAM(data);
    UNUSED_PARAM(error);

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
gboolean
parse_opts (gint argc,
            gchar *argv[],
            tabrmd_options_t *options)
{
    gchar *logger_name = "stdout";
    GOptionContext *ctx;
    GError *err = NULL;
    gboolean session_bus = FALSE;

    GOptionEntry entries[] = {
        { "dbus-name", 'n', 0, G_OPTION_ARG_STRING, &options->dbus_name,
          "Name for daemon to \"own\" on the D-Bus",
          TABRMD_DBUS_NAME_DEFAULT },
        { "logger", 'l', 0, G_OPTION_ARG_STRING, &logger_name,
          "The name of desired logger, stdout is default.", "[stdout|syslog]"},
        { "session", 's', 0, G_OPTION_ARG_NONE, &session_bus,
          "Connect to the session bus (system bus is default).", NULL },
        { "flush-all", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
          &options->flush_all,
          "Flush all objects and sessions from TPM on startup.", NULL },
        { "max-connections", 'm', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &options->max_connections, "Maximum number of client connections.",
          NULL },
        { "max-sessions", 'e', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &options->max_sessions,
          "Maximum number of sessions per connection.", NULL },
        { "max-transients", 'r', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &options->max_transients,
          "Maximum number of loaded transient objects per client.", NULL },
        { "prng-seed-file", 'g', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
          &options->prng_seed_file, "File to read seed value for PRNG",
          options->prng_seed_file },
        { "version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
          show_version, "Show version string", NULL },
        { "allow-root", 'o', 0, G_OPTION_ARG_NONE,
          &options->allow_root,
          "Allow the daemon to run as root, which is not recommended", NULL },
        {
            .long_name       = "tcti",
            .short_name      = 't',
            .flags           = G_OPTION_FLAG_NONE,
            .arg             = G_OPTION_ARG_STRING,
            .arg_data        = &options->tcti_conf,
            .description     = "TCTI configuration string. See tpm2-abrmd (8) for search rules.",
            .arg_description = "tcti-conf",
        },
        { NULL, '\0', 0, 0, NULL, NULL, NULL },
    };

    g_warning ("tcti_conf before: \"%s\"", options->tcti_conf);

    ctx = g_option_context_new (" - TPM2 software stack Access Broker Daemon (tabrmd)");
    g_option_context_add_main_entries (ctx, entries, NULL);
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
        g_critical ("Failed to parse options: %s", err->message);
        return FALSE;
    }
    g_option_context_free (ctx);
    /* select the bus type, default to G_BUS_TYPE_SESSION */
    options->bus = session_bus ? G_BUS_TYPE_SESSION : G_BUS_TYPE_SYSTEM;
    if (set_logger (logger_name) == -1) {
        g_critical ("Unknown logger: %s, try --help\n", logger_name);
        return FALSE;
    }
    if (options->max_connections < 1 ||
        options->max_connections > TABRMD_CONNECTION_MAX)
    {
        g_critical ("maximum number of connections must be between 1 "
                    "and %d", TABRMD_CONNECTION_MAX);
        return FALSE;
    }
    if (options->max_sessions < 1 ||
        options->max_sessions > TABRMD_SESSIONS_MAX_DEFAULT)
    {
        g_critical ("max-sessions must be between 1 and %d",
                    TABRMD_SESSIONS_MAX_DEFAULT);
        return FALSE;
    }
    if (options->max_transients < 1 ||
        options->max_transients > TABRMD_TRANSIENT_MAX)
    {
        g_critical ("max-trans-obj parameter must be between 1 and %d",
                    TABRMD_TRANSIENT_MAX);
        return FALSE;
    }
    g_warning ("tcti_conf after: \"%s\"", options->tcti_conf);
    return TRUE;
}
