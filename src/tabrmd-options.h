/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Intel Corporation
 */
#ifndef TABRMD_OPTIONS_H
#define TABRMD_OPTIONS_H

#include <gio/gio.h>

#include "tabrmd-defaults.h"

#define TABRMD_OPTIONS_INIT_DEFAULT { \
    .bus = (GBusType)TABRMD_DBUS_TYPE_DEFAULT, \
    .flush_all = FALSE, \
    .max_connections = TABRMD_CONNECTIONS_MAX_DEFAULT, \
    .max_transients = TABRMD_TRANSIENT_MAX_DEFAULT, \
    .max_sessions = TABRMD_SESSIONS_MAX_DEFAULT, \
    .dbus_name = TABRMD_DBUS_NAME_DEFAULT, \
    .prng_seed_file = TABRMD_ENTROPY_SRC_DEFAULT, \
    .allow_root = FALSE, \
    .tcti_conf = TABRMD_TCTI_CONF_DEFAULT, \
}

typedef struct tabrmd_options {
    GBusType        bus;
    gboolean        flush_all;
    guint           max_connections;
    guint           max_transients;
    guint           max_sessions;
    gchar          *dbus_name;
    const gchar    *prng_seed_file;
    gboolean        allow_root;
    gchar          *tcti_conf;
} tabrmd_options_t;

gboolean
parse_opts (gint argc,
            gchar *argv[],
            tabrmd_options_t *options);

#endif /* TABRMD_OPTIONS_H */
