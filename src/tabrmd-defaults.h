/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Intel Corporation
 */
#ifndef TABRMD_DEFAULTS_H
#define TABRMD_DEFAULTS_H

#define TABRMD_CONNECTIONS_MAX_DEFAULT 27
#define TABRMD_CONNECTION_MAX 100
#define TABRMD_DBUS_NAME_DEFAULT "com.intel.tss2.Tabrmd"
#define TABRMD_DBUS_TYPE_DEFAULT G_BUS_TYPE_SYSTEM
#define TABRMD_DBUS_PATH "/com/intel/tss2/Tabrmd/Tcti"
#define TABRMD_DBUS_METHOD_CREATE_CONNECTION "CreateConnection"
#define TABRMD_DBUS_METHOD_CANCEL "Cancel"
#define TABRMD_ERROR tabrmd_error_quark ()
#define TABRMD_ENTROPY_SRC_DEFAULT "/dev/urandom"
#define TABRMD_SESSIONS_MAX_DEFAULT 4
#define TABRMD_SESSIONS_MAX 64
#define TABRMD_TCTI_CONF_DEFAULT "device:/dev/tpm0"
#define TABRMD_TRANSIENT_MAX_DEFAULT 27
#define TABRMD_TRANSIENT_MAX 100

#endif
