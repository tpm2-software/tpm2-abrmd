#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "command-attrs.h"

static gpointer command_attrs_parent_class = NULL;

/* Boilerplate GObject mgmt code. */
static void
command_attrs_finalize (GObject *obj)
{
    CommandAttrs *attrs = COMMAND_ATTRS (obj);

    g_debug ("command_attrs_finalize: 0x%" PRIxPTR, attrs);
    if (attrs->command_attrs)
        free (attrs->command_attrs);
    G_OBJECT_CLASS (command_attrs_parent_class)->finalize (obj);
}

static void
command_attrs_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("command_attrs_class_init");
    if (command_attrs_parent_class == NULL)
        command_attrs_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = command_attrs_finalize;
}

GType
command_attrs_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "CommandAttrs",
                                              sizeof (CommandAttrsClass),
                                              (GClassInitFunc) command_attrs_class_init,
                                              sizeof (CommandAttrs),
                                              NULL,
                                              0);
    }
    return type;
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
command_attrs_init (CommandAttrs *attrs,
                    AccessBroker *broker)
{
    TSS2_RC               rc;
    TPMS_CAPABILITY_DATA  capability_data;
    TSS2_SYS_CONTEXT     *sapi_context;
    TPMI_YES_NO           more;
    int                   i;

    sapi_context = access_broker_lock_sapi (broker);
    if (sapi_context == NULL) {
        g_warning ("access_broker_lock_sapi returned NULL TSS2_SYS_CONTEXT.");
        return -1;
    }
    rc = access_broker_get_max_command (broker, &attrs->count);
    if (rc != TSS2_RC_SUCCESS || attrs->count == 0) {
        g_warning ("failed to get TPM_PT_TOTAL_COMMANDS: 0x%" PRIx32
                   ", count: 0x%" PRIx32, rc, attrs->count);
        return -1;
    }
    g_debug ("GetCapabilty for 0x%" PRIx32 " commands", attrs->count);
    rc = Tss2_Sys_GetCapability (sapi_context,
                                 NULL,
                                 TPM_CAP_COMMANDS,
                                 TPM_CC_FIRST,
                                 attrs->count,
                                 &more,
                                 &capability_data,
                                 NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to get TPM command attributes: 0x%" PRIx32, rc);
        rc = access_broker_unlock (broker);
        return -1;
    }
    rc = access_broker_unlock (broker);

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

/* TPM_CC is in the lower 15 bits of the TPMA_CC */
#define TPM_CC_FROM_TPMA_CC(attrs) (attrs.val & 0x7fff)
TPMA_CC
command_attrs_from_cc (CommandAttrs *attrs,
                       TPM_CC        command_code)
{
    int i;

    for (i = 0; i < attrs->count; ++i)
        if (TPM_CC_FROM_TPMA_CC (attrs->command_attrs[i]) == command_code)
            return attrs->command_attrs[i];

    return (TPMA_CC) { 0, };
}
