/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <stdlib.h>
#include <syslog.h>
#include "util.h"
#include "logging.h"

/**
 * This function that implements the GLogFunc prototype. It is intended
 * for use as a log handler function for glib logging.
 */
void
syslog_log_handler (const char     *log_domain,
                    GLogLevelFlags  log_level,
                    const char     *message,
                    gpointer        log_config_list)
{
    UNUSED_PARAM(log_domain);
    UNUSED_PARAM(log_config_list);

    switch (log_level) {
    case G_LOG_FLAG_FATAL:
        syslog (LOG_ALERT, "%s", message);
        break;
    case G_LOG_LEVEL_ERROR:
        syslog (LOG_ERR, "%s", message);
        break;
    case G_LOG_LEVEL_CRITICAL:
        syslog (LOG_CRIT, "%s", message);
        break;
    case G_LOG_LEVEL_WARNING:
        syslog (LOG_WARNING, "%s", message);
        break;
    case G_LOG_LEVEL_MESSAGE:
        syslog (LOG_NOTICE, "%s", message);
        break;
    case G_LOG_LEVEL_INFO:
        syslog (LOG_INFO, "%s", message);
        break;
    case G_LOG_LEVEL_DEBUG:
        syslog (LOG_DEBUG, "%s", message);
        break;
    default:
        syslog (LOG_INFO, "%s", message);
    }
}
/*
 * The G_MESSAGES_DEBUG environment variable is a space separated list of
 * glib logging domains that we want to see debug and info messages from.
 * The right way to do this is to declare a logging domain for the application
 * but for now we simply look for the special value of "all" and enable info
 * and debug messages if it's set.
 */
int
get_enabled_log_levels (void)
{
    gchar *g_log_domains = NULL;

    g_log_domains = getenv ("G_MESSAGES_DEBUG");
    if (g_log_domains == NULL) {
        return LOG_LEVEL_DEFAULT;
    }
    if (g_strcmp0 (g_log_domains, "all") == 0) {
        return LOG_LEVEL_ALL;
    } else {
        return LOG_LEVEL_DEFAULT;
    }
}
/**
 * Convenience function to set logger for GLog.
 */
gint
set_logger (gchar *name)
{
    int enabled_log_levels = 0;

    if (g_strcmp0 (name, "syslog") == 0) {
        enabled_log_levels = get_enabled_log_levels ();
        g_log_set_handler (NULL,
                           enabled_log_levels | G_LOG_FLAG_FATAL | \
                           G_LOG_FLAG_RECURSION,
                           syslog_log_handler,
                           NULL);
        return 0;
    } else if (g_strcmp0 (name, "stdout") == 0) {
        /* stdout is the default for g_log, nothing to do but return 0 */
        g_info ("logging to stdout");
        return 0;
    }
    return -1;
}
