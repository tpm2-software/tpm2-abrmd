/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#ifndef LOGGING_H
#define LOGGING_H

#include <glib.h>

/* Macro to log "critical" events, then exit indicating failure. */
#define tabrmd_critical(fmt, ...) \
    do { \
        g_critical (fmt, ##__VA_ARGS__); \
        exit (EXIT_FAILURE); \
    } while (0)

#define LOG_LEVEL_DEFAULT (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | \
                           G_LOG_LEVEL_WARNING)
#define LOG_LEVEL_ALL     (LOG_LEVEL_DEFAULT | G_LOG_LEVEL_MESSAGE | \
                           G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)

void
syslog_log_handler (const char     *log_domain,
                    GLogLevelFlags  log_level,
                    const char     *message,
                    gpointer        log_config_list);
int get_enabled_log_levels (void);
gint set_logger (gchar *name);
#endif /* LOGGING_H */
