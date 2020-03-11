/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
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

    g_debug (__func__);
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
                        Tpm2 *tpm2)
{
    TSS2_RC rc;

    rc = tpm2_get_command_attrs (tpm2,
                                 &attrs->count,
                                 &attrs->command_attrs);
    if (rc != TSS2_RC_SUCCESS) {
        return -1;
    }

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
