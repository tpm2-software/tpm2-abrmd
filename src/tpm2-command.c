#include "tpm2-command.h"

/**
 * Boiler-plate gobject code.
 * NOTE: I tried to use the G_DEFINE_TYPE macro to take care of this boiler
 * plate for us but ended up with weird segfaults in the type checking macros.
 * Going back to doing this by hand resolved the issue thankfully.
 */
static gpointer tpm2_command_parent_class = NULL;

enum {
    PROP_0,
    PROP_SESSION,
    PROP_BUFFER,
    PROP_SIZE,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * GObject property setter.
 */
static void
tpm2_command_set_property (GObject        *object,
                           guint           property_id,
                           GValue const   *value,
                           GParamSpec     *pspec)
{
    Tpm2Command *self = TPM2_COMMAND (object);

    switch (property_id) {
    case PROP_BUFFER:
        if (self->buffer != NULL) {
            g_warning ("  buffer already set");
            break;
        }
        self->buffer = (guint8*)g_value_get_pointer (value);
        break;
    case PROP_SESSION:
        if (self->session != NULL) {
            g_warning ("  session already set");
            break;
        }
        self->session = g_value_get_object (value);
        break;
    case PROP_SIZE:
        self->size = g_value_get_uint (value);
        g_debug ("  size: 0x%x", self->size);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * GObject property getter.
 */
static void
tpm2_command_get_property (GObject     *object,
                           guint        property_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
    Tpm2Command *self = TPM2_COMMAND (object);

    g_debug ("tpm2_command_get_property: 0x%x", self);
    switch (property_id) {
    case PROP_BUFFER:
        g_value_set_pointer (value, self->buffer);
        break;
    case PROP_SESSION:
        g_value_set_object (value, self->session);
        break;
    case PROP_SIZE:
        g_value_set_uint (value, self->size);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * override the parent finalize method so we can free the data associated with
 * the DataMessage instance.
 */
static void
tpm2_command_finalize (GObject *obj)
{
    Tpm2Command *cmd = TPM2_COMMAND (obj);

    g_debug ("tpm2_command_finalize");
    if (cmd->buffer)
        g_free (cmd->buffer);
    if (cmd->session)
        g_object_unref (cmd->session);
    G_OBJECT_CLASS (tpm2_command_parent_class)->finalize (obj);
}
/**
 * Boilerplate GObject initialization. Get a pointer to the parent class,
 * setup a finalize function.
 */
static void
tpm2_command_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tpm2_command_parent_class == NULL)
        tpm2_command_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = tpm2_command_finalize;
    object_class->get_property = tpm2_command_get_property;
    object_class->set_property = tpm2_command_set_property;

    obj_properties [PROP_BUFFER] =
        g_param_spec_pointer ("buffer",
                              "TPM2 command buffer",
                              "memory buffer holding a TPM2 command",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SESSION] =
        g_param_spec_object ("session",
                             "Session object",
                             "The Session object that sent the command",
                             TYPE_SESSION_DATA,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SIZE] =
        g_param_spec_uint ("size",
                           "Size of the command",
                           "The Size of the TPM2 command buffer",
                           0,
                           G_MAXUINT32,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Upon first call to *_get_type we register the type with the GType system.
 * We keep a static GType around to speed up future calls.
 */
GType
tpm2_command_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "Tpm2Command",
                                              sizeof (Tpm2CommandClass),
                                              (GClassInitFunc) tpm2_command_class_init,
                                              sizeof (Tpm2Command),
                                              NULL,
                                              0);
    }
    return type;
}
/**
 * Boilerplate constructor, but some GObject properties would be nice.
 */
Tpm2Command*
tpm2_command_new (SessionData     *session,
                  guint32          size,
                  guint8          *buffer)
{
    return TPM2_COMMAND (g_object_new (TYPE_TPM2_COMMAND,
                                       "buffer",  buffer,
                                       "session", session,
                                       "size",    size,
                                       NULL));
}
/**
 */
guint8*
tpm2_command_get_buffer (Tpm2Command *command)
{
    return command->buffer;
}
/**
 */
TPM_CC
tpm2_command_get_code (Tpm2Command *command)
{
    return be32toh (*(TPM_CC*)(command->buffer +
                               sizeof (TPMI_ST_COMMAND_TAG) +
                               sizeof (UINT32)));
}
/**
 */
guint32
tpm2_command_get_size (Tpm2Command *command)
{
    return be32toh (*(guint32*)(command->buffer +
                                sizeof (TPMI_ST_COMMAND_TAG)));
}
/**
 */
TPMI_ST_COMMAND_TAG
tpm2_command_get_tag (Tpm2Command *command)
{
    return be16toh (*(TPMI_ST_COMMAND_TAG*)(command->buffer));
}
