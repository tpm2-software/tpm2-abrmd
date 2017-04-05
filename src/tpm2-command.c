#include <inttypes.h>
#include <stdint.h>

#include "tpm2-command.h"
#include "util.h"
/* macros to make getting at fields in the command more simple */

#define HANDLE_OFFSET (sizeof (TPMI_ST_COMMAND_TAG) + \
                       sizeof (UINT32) + \
                       sizeof (TPM_CC))
#define HANDLE_INDEX(i) (sizeof (TPM_HANDLE) * i)
#define HANDLE_GET(buffer, count) (*(TPM_HANDLE*)(HANDLE_START (buffer) + \
                                                  HANDLE_INDEX (count)))
#define HANDLE_START(buffer) (buffer + HANDLE_OFFSET)

G_DEFINE_TYPE (Tpm2Command, tpm2_command, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_ATTRIBUTES,
    PROP_SESSION,
    PROP_BUFFER,
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
    case PROP_ATTRIBUTES:
        self->attributes = (TPMA_CC)g_value_get_uint (value);
        break;
    case PROP_BUFFER:
        if (self->buffer != NULL) {
            g_warning ("  buffer already set");
            break;
        }
        self->buffer = (guint8*)g_value_get_pointer (value);
        break;
    case PROP_SESSION:
        if (self->connection != NULL) {
            g_warning ("  connection already set");
            break;
        }
        self->connection = g_value_get_object (value);
        g_object_ref (self->connection);
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

    g_debug ("tpm2_command_get_property: 0x%" PRIxPTR, (uintptr_t)self);
    switch (property_id) {
    case PROP_ATTRIBUTES:
        g_value_set_uint (value, self->attributes.val);
        break;
    case PROP_BUFFER:
        g_value_set_pointer (value, self->buffer);
        break;
    case PROP_SESSION:
        g_value_set_object (value, self->connection);
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
    if (cmd->connection)
        g_object_unref (cmd->connection);
    G_OBJECT_CLASS (tpm2_command_parent_class)->finalize (obj);
}
static void
tpm2_command_init (Tpm2Command *command)
{ /* noop */ }
/**
 * Boilerplate GObject initialization. Get a pointer to the parent class,
 * setup a finalize function.
 */
static void
tpm2_command_class_init (Tpm2CommandClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tpm2_command_parent_class == NULL)
        tpm2_command_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = tpm2_command_finalize;
    object_class->get_property = tpm2_command_get_property;
    object_class->set_property = tpm2_command_set_property;

    obj_properties [PROP_ATTRIBUTES] =
        g_param_spec_uint ("attributes",
                           "TPMA_CC",
                           "Attributes for command.",
                           0,
                           UINT32_MAX,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_BUFFER] =
        g_param_spec_pointer ("buffer",
                              "TPM2 command buffer",
                              "memory buffer holding a TPM2 command",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SESSION] =
        g_param_spec_object ("connection",
                             "Session object",
                             "The Connection object that sent the command",
                             TYPE_CONNECTION,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate constructor, but some GObject properties would be nice.
 */
Tpm2Command*
tpm2_command_new (Connection     *connection,
                  guint8          *buffer,
                  TPMA_CC          attributes)
{
    return TPM2_COMMAND (g_object_new (TYPE_TPM2_COMMAND,
                                       "attributes", attributes,
                                       "buffer",  buffer,
                                       "connection", connection,
                                       NULL));
}
/**
 * Same as tpm2_command_new but instead of taking the command buffer
 * as a parameter we read it from the 'fd' parameter (file descriptor).
 */
Tpm2Command*
tpm2_command_new_from_fd (Connection *connection,
                          gint         fd,
                          CommandAttrs *command_attrs)
{
    guint8 *command_buf = NULL;
    UINT32 command_size = 0;
    TPMA_CC attributes;
    TPM_CC  command_code;

    g_debug ("tpm2_command_new_from_fd: %d", fd);
    command_buf = read_tpm_command_from_fd (fd, &command_size);
    if (command_buf == NULL)
        return NULL;
    command_code = be32toh (*(TPM_CC*)(command_buf +
                                       sizeof (TPMI_ST_COMMAND_TAG) +
                                       sizeof (UINT32)));
    attributes = command_attrs_from_cc (command_attrs, command_code);
    if (attributes.val == 0) {
        g_warning ("Failed to find TPMA_CC for TPM_CC: 0x%" PRIx32,
                   command_code);
        free (command_buf);
        return NULL;
    }
    return tpm2_command_new (connection, command_buf, attributes);
}
/* Simple "getter" to expose the attributes associated with the command. */
TPMA_CC
tpm2_command_get_attributes (Tpm2Command *command)
{
    return command->attributes;
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
/*
 * Return the Connection object associated with this Tpm2Command. This
 * is the Connection object representing the client that sent this command.
 * The reference count on this object is incremented before the Connection
 * object is returned to the caller. The caller is responsible for
 * decrementing the reference count when it is done using the connection object.
 */
Connection*
tpm2_command_get_connection (Tpm2Command *command)
{
    g_object_ref (command->connection);
    return command->connection;
}
/* Return the number of handles in the command. */
guint8
tpm2_command_get_handle_count (Tpm2Command *command)
{
    g_debug ("tpm2_command_get_handle_count");
    uint32_t tmp;

    if (command == NULL) {
        g_warning ("tpm2_command_get_handle_count received NULL parameter");
        return 0;
    }
    tmp = tpm2_command_get_attributes (command).val;
    return (guint8)((tmp & TPMA_CC_CHANDLES) >> 25);
}
/*
 * Simple function to access handles in the provided Tpm2Command. The
 * 'handle_number' parameter is a 0-based index into the handle area
 * which is effectively an array. If the handle_number requests a handle
 * beyond the end of this array 0 is returned.
 */
TPM_HANDLE
tpm2_command_get_handle (Tpm2Command *command,
                         guint8       handle_number)
{
    guint8 real_count;

    if (command == NULL)
        return 0;
    real_count = tpm2_command_get_handle_count (command);
    if (real_count > handle_number) {
        return be32toh (HANDLE_GET (command->buffer, handle_number));
    } else {
        return 0;
    }
}
/*
 * Simple function to set a handle at the 0-based index into the Tpm2Command
 * handle area to the provided value. If the handle_number is past the bounds
 * FALSE is returned.
 */
gboolean
tpm2_command_set_handle (Tpm2Command *command,
                         TPM_HANDLE   handle,
                         guint8       handle_number)
{
    guint8 real_count;

    if (command == NULL)
        return FALSE;
    real_count = tpm2_command_get_handle_count (command);
    if (real_count > handle_number) {
        HANDLE_GET (command->buffer, handle_number) = htobe32 (handle);
        return TRUE;
    } else {
        return FALSE;
    }
}
/*
 * Return the handles from the Tpm2Command back to the caller by way of the
 * handles parameter. The caller must allocate sufficient space to hold
 * however many handles are in this command. Take a look @
 * tpm2_command_get_handle_count.
 */
gboolean
tpm2_command_get_handles (Tpm2Command *command,
                          TPM_HANDLE   handles[],
                          guint8       count)
{
    guint8 real_count, i;

    if (command == NULL || handles == NULL) {
        g_warning ("tpm2_command_get_handles passed NULL parameter");
        return FALSE;
    }
    real_count = tpm2_command_get_handle_count (command);
    if (real_count > count) {
        g_warning ("tpm2_command_get_handles passed insufficient handle array");
        return FALSE;
    }

    for (i = 0; i < real_count; ++i)
        handles[i] = be32toh (HANDLE_GET (command->buffer, i));

    return TRUE;
}
/*
 * Set handles in the Tpm2Command buffer. The handles are passed in the
 * 'handles' parameter, the size of this array through the 'count'
 * parameter. Each invocation of this function sets all handles in the
 * Tpm2Command and so the size of the handles array (aka 'count') must be
 * the same as the number of handles in the command.
 */
gboolean
tpm2_command_set_handles (Tpm2Command *command,
                          TPM_HANDLE   handles[],
                          guint8       count)
{
    guint8 real_count, i;

    if (command == NULL || handles == NULL) {
        g_warning ("tpm2_command_get_handles passed NULL parameter");
        return FALSE;
    }
    real_count = tpm2_command_get_handle_count (command);
    if (real_count != count) {
        g_warning ("tpm2_command_set_handles passed handle array with wrong "
                   "number of handles");
        return FALSE;
    }

    for (i = 0; i < real_count; ++i)
        HANDLE_GET (command->buffer, i) = htobe32 (handles [i]);

    return TRUE;
}
/*
 * This function is a work around. The handle in a command buffer for the
 * FlushContext command is not in the handle area and no handles are reported
 * in the attributes structure. This means that in order to cope with
 * virtualized handles in this command we must reach into the parameter area
 * which breaks the abstractions we've built for accessing handles. Thus we
 * provide a special function for getting at this single handle.
 * Use this function with caution.
 */
TPM_HANDLE
tpm2_command_get_flush_handle (Tpm2Command *command)
{
    if (command == NULL) {
        g_warning ("tpm2_command_get_flush_handle passed null parameter");
        return 0;
    }
    if (tpm2_command_get_code (command) != TPM_CC_FlushContext) {
        g_warning ("tpm2_command_get_flush_handle called with wrong command");
        return 0;
    }

    /*
     * Despite not techncially being in the "handle area" of the command we
     * are still able to access the handle as though it were. This is because
     * there are no other handles or authorizations allowd in the command and
     * the handle being flushed is the first parameter.
     */
    return be32toh (HANDLE_GET (tpm2_command_get_buffer (command), 0));
}
