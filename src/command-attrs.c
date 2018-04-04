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
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "command-attrs.h"

G_DEFINE_TYPE (CommandAttrs, command_attrs, G_TYPE_OBJECT);

/*
 * G_DEFINE_TYPE requires an instance init even though we don't use it.
 */
static void
command_attrs_init (CommandAttrs *attrs)
{
    UNUSED_PARAM(attrs);
    /* noop */
}

static void
command_attrs_finalize (GObject *obj)
{
    CommandAttrs *attrs = COMMAND_ATTRS (obj);

    g_debug ("command_attrs_finalize: 0x%" PRIxPTR, (uintptr_t)attrs);
    g_clear_pointer (&attrs->command_attrs, g_free);
    G_OBJECT_CLASS (command_attrs_parent_class)->finalize (obj);
}

static void
command_attrs_class_init (CommandAttrsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("command_attrs_class_init");
    if (command_attrs_parent_class == NULL)
        command_attrs_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = command_attrs_finalize;
}

/*
 * Create a new CommandAttrs object. Do no initialization in the
 * constructor since initialization requires a round-trip to the TPM
 * to query for TPMA_CCs.
 */
CommandAttrs*
command_attrs_new (void)
{
    return COMMAND_ATTRS (g_object_new (TYPE_COMMAND_ATTRS, NULL));
}
/*
 */
gint
command_attrs_init_tpm (CommandAttrs *attrs,
                        AccessBroker *broker)
{
    TSS2_RC               rc;
    TPMS_CAPABILITY_DATA  capability_data;
    TSS2_SYS_CONTEXT     *sapi_context;
    TPMI_YES_NO           more;
    unsigned int          i;

    rc = access_broker_get_max_command (broker, &attrs->count);
    if (rc != TSS2_RC_SUCCESS || attrs->count == 0) {
        g_warning ("failed to get TPM2_PT_TOTAL_COMMANDS: 0x%" PRIx32
                   ", count: 0x%" PRIx32, rc, attrs->count);
        return -1;
    }
    sapi_context = access_broker_lock_sapi (broker);
    if (sapi_context == NULL) {
        g_warning ("access_broker_lock_sapi returned NULL TSS2_SYS_CONTEXT.");
        access_broker_unlock (broker);
        return -1;
    }
    g_debug ("GetCapabilty for 0x%" PRIx32 " commands", attrs->count);
    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 TPM2_CAP_COMMANDS,
                                 TPM2_CC_FIRST,
                                 attrs->count,
                                 &more,
                                 &capability_data,
                                 NULL);
    access_broker_unlock (broker);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to get TPM command attributes: 0x%" PRIx32, rc);
        return -1;
    }

    attrs->count = capability_data.data.command.count;
    g_debug ("got attributes for 0x%" PRIx32 " commands", attrs->count);
    attrs->command_attrs = (TPMA_CC*) calloc (1, sizeof (TPMA_CC) * attrs->count);
    if (attrs->command_attrs == NULL) {
        g_warning ("Failed to allocate memory for TPMA_CC: %s\n",
                   strerror (errno));
        return -1;
    }
    for (i = 0; i < attrs->count; ++i)
        attrs->command_attrs[i] = capability_data.data.command.commandAttributes[i];

    return 0;
}

/* TPM2_CC is in the lower 15 bits of the TPMA_CC */
#define TPM2_CC_FROM_TPMA_CC(attrs) (attrs & 0x7fff)
TPMA_CC
command_attrs_from_cc (CommandAttrs *attrs,
                       TPM2_CC        command_code)
{
    unsigned int i;

    for (i = 0; i < attrs->count; ++i)
        if (TPM2_CC_FROM_TPMA_CC (attrs->command_attrs[i]) == command_code)
            return attrs->command_attrs[i];

    return (TPMA_CC) { 0 };}
