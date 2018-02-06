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
#ifndef TSS2_TABD_H
#define TSS2_TABD_H

#include <gio/gio.h>
#include <sapi/tpm20.h>

#include "tcti-factory.h"
#include "tcti-tabrmd.h"

#define TABRMD_DBUS_INTERFACE_DEFAULT        TCTI_TABRMD_DBUS_INTERFACE_DEFAULT
#define TABRMD_DBUS_NAME_DEFAULT             TCTI_TABRMD_DBUS_NAME_DEFAULT
#define TABRMD_DBUS_TYPE_DEFAULT             TCTI_TABRMD_DBUS_TYPE_DEFAULT
#define TABRMD_DBUS_PATH                     "/com/intel/tss2/Tabrmd/Tcti"
#define TABRMD_DBUS_METHOD_CREATE_CONNECTION "CreateConnection"
#define TABRMD_DBUS_METHOD_CANCEL            "Cancel"
#define TABRMD_ERROR tabrmd_error_quark ()

#define MAX_SESSIONS 64
#define MAX_SESSIONS_DEFAULT 4
#define MAX_TRANSIENT_OBJECTS 100
#define MAX_TRANSIENT_OBJECTS_DEFAULT 27
#define TABD_INIT_THREAD_NAME "tss2-tabrmd_init-thread"

/* implementation specific RCs */
#define TSS2_RESMGR_RC_INTERNAL_ERROR (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | (1 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT))
#define TSS2_RESMGR_RC_SAPI_INIT      (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | (2 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT))
#define TSS2_RESMGR_RC_OUT_OF_MEMORY  (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | (3 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT))
/* RCs in the RESMGR layer */
#define TSS2_RESMGR_RC_BAD_VALUE       (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | TSS2_BASE_RC_BAD_VALUE)
#define TSS2_RESMGR_RC_NOT_PERMITTED   (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | TSS2_BASE_RC_NOT_PERMITTED)
#define TSS2_RESMGR_RC_NOT_IMPLEMENTED (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | TSS2_BASE_RC_NOT_IMPLEMENTED)
#define TSS2_RESMGR_RC_GENERAL_FAILURE (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | TSS2_BASE_RC_GENERAL_FAILURE)
#define TSS2_RESMGR_RC_OBJECT_MEMORY   (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | TPM_RC_OBJECT_MEMORY)
#define TSS2_RESMGR_RC_SESSION_MEMORY  (TSS2_RC)(TSS2_RESMGR_ERROR_LEVEL | TPM_RC_SESSION_MEMORY)

typedef struct tabrmd_options {
    GBusType        bus;
    TctiFactory    *tcti_factory;
    gboolean        fail_on_loaded_trans;
    gboolean        flush_all;
    guint           max_connections;
    guint           max_transient_objects;
    guint           max_sessions;
    gchar          *dbus_name;
    const gchar    *prng_seed_file;
} tabrmd_options_t;

GQuark  tabrmd_error_quark (void);

TSS2_RC tss2_tcti_tabrmd_dump_trans_state (TSS2_TCTI_CONTEXT *tcti_context);

#endif /* TSS2_TABD_H */
