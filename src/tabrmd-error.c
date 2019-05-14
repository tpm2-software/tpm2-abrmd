/* SPDX-License-Identifier: BSD-2-Clause */
#include <gio/gio.h>
#include <glib.h>
#include "tabrmd.h"

static const GDBusErrorEntry tabrmd_error_entries[] = {
    {
        .error_code      = TSS2_RESMGR_RC_GENERAL_FAILURE,
        .dbus_error_name = "com.intel.tss2.Tabrmd.Error.General"
    },
    {
        .error_code      = TSS2_RESMGR_RC_INTERNAL_ERROR,
        .dbus_error_name = "com.intel.tss2.Tabrmd.Error.Internal"
    },
    {   /* this one looks sketchy */
        .error_code      = TSS2_RESMGR_RC_SAPI_INIT,
        .dbus_error_name = "com.intel.tss2.Tabrmd.Error.Init"
    },
    {
        .error_code      = TSS2_RESMGR_RC_NOT_IMPLEMENTED,
        .dbus_error_name = "com.intel.tss2.Tabrmd.Error.NotImplemented"
    },
    {
        .error_code      = TSS2_RESMGR_RC_NOT_PERMITTED,
        .dbus_error_name = "com.intel.tss2.Tabrmd.Error.NotPermitted",
    },
};

/*
 * Register mapping of error codes to dbus error names.
 */
GQuark
tabrmd_error_quark (void)
{
    static volatile gsize quark_volatile = 0;
    g_dbus_error_register_error_domain ("tabrmd-error",
                                        &quark_volatile,
                                        tabrmd_error_entries,
                                        G_N_ELEMENTS (tabrmd_error_entries));
    return (GQuark) quark_volatile;
}
