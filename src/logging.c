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
    }
    return -1;
}
