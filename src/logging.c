/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <syslog.h>
#include "logging.h"

#define LOG_LEVEL_ALL \
     (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | \
      G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)
/**
 * This function that implements the GLogFunc prototype. It is intended
 * for use as a log handler function for glib logging.
 */
static void
syslog_log_handler (const char     *log_domain,
                    GLogLevelFlags  log_level,
                    const char     *message,
                    gpointer        log_config_list)
{
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
/**
 * Convenience function to set logger for GLog.
 */
gint
set_logger (gchar *name)
{
   if (g_strcmp0 (name, "syslog") == 0) {
        g_info ("logging to syslog");
        g_log_set_handler (NULL,
                           LOG_LEVEL_ALL | G_LOG_FLAG_FATAL | \
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
