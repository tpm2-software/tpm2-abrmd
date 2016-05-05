#include <syslog.h>
#include "tss2-tabd-logging.h"

#define LOG_LEVEL_ALL \
     (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | \
      G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)

static void
syslog_log_handler (const char     *log_domain,
                    GLogLevelFlags  log_level,
                    const char     *message,
                    gpointer        log_config_list)
{
    switch (log_level) {
    case G_LOG_FLAG_FATAL:
        syslog (LOG_ALERT, message);
        break;
    case G_LOG_LEVEL_ERROR:
        syslog (LOG_ERR, message);
        break;
    case G_LOG_LEVEL_CRITICAL:
        syslog (LOG_CRIT, message);
        break;
    case G_LOG_LEVEL_WARNING:
        syslog (LOG_WARNING, message);
        break;
    case G_LOG_LEVEL_MESSAGE:
        syslog (LOG_NOTICE, message);
        break;
    case G_LOG_LEVEL_INFO:
        syslog (LOG_INFO, message);
        break;
    case G_LOG_LEVEL_DEBUG:
        syslog (LOG_DEBUG, message);
        break;
    default:
        syslog (LOG_INFO, message);
    }
}

gint
set_logger (gchar *name)
{
    gint ret = 0;

    if (name == NULL)
        goto out;
    if (g_strcmp0 (name, "stdout") == 0) {
        g_info ("logging to glib standard logger");
        goto out;
    }
    if (g_strcmp0 (name, "syslog") == 0) {
        g_info ("logging to syslog");
        g_log_set_handler (NULL,
                           LOG_LEVEL_ALL | G_LOG_FLAG_FATAL | \
                           G_LOG_FLAG_RECURSION,
                           syslog_log_handler,
                           NULL);
        goto out;
    }
    ret = -1;
out:
    return ret;
}
