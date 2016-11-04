#include <tpm20.h>

#include "tpm2-response.h"

#define TPM_RESPONSE_TAG(buffer) (*(TPM_ST*)buffer)
#define TPM_RESPONSE_SIZE(buffer) (*(guint32*)(buffer + \
                                               sizeof (TPM_ST)))
#define TPM_RESPONSE_CODE(buffer) (*(TPM_RC*)(buffer + \
                                              sizeof (TPM_ST) + \
                                              sizeof (UINT32)))
/**
 * Boiler-plate gobject code.
 * NOTE: I tried to use the G_DEFINE_TYPE macro to take care of this boiler
 * plate for us but ended up with weird segfaults in the type checking macros.
 * Going back to doing this by hand resolved the issue thankfully.
 */
static gpointer tpm2_response_parent_class = NULL;

enum {
    PROP_0,
    PROP_SESSION,
    PROP_BUFFER,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/**
 * GObject property setter.
 */
static void
tpm2_response_set_property (GObject        *object,
                            guint           property_id,
                            GValue const   *value,
                            GParamSpec     *pspec)
{
    Tpm2Response *self = TPM2_RESPONSE (object);

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
        g_object_ref (self->session);
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
tpm2_response_get_property (GObject     *object,
                            guint        property_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
    Tpm2Response *self = TPM2_RESPONSE (object);

    g_debug ("tpm2_response_get_property: 0x%x", self);
    switch (property_id) {
    case PROP_BUFFER:
        g_value_set_pointer (value, self->buffer);
        break;
    case PROP_SESSION:
        g_value_set_object (value, self->session);
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
tpm2_response_finalize (GObject *obj)
{
    Tpm2Response *self = TPM2_RESPONSE (obj);

    g_debug ("tpm2_response_finalize");
    if (self->buffer)
        g_free (self->buffer);
    if (self->session)
        g_object_unref (self->session);
    G_OBJECT_CLASS (tpm2_response_parent_class)->finalize (obj);
}
/**
 * Boilerplate GObject initialization. Get a pointer to the parent class,
 * setup a finalize function.
 */
static void
tpm2_response_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tpm2_response_parent_class == NULL)
        tpm2_response_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = tpm2_response_finalize;
    object_class->get_property = tpm2_response_get_property;
    object_class->set_property = tpm2_response_set_property;

    obj_properties [PROP_BUFFER] =
        g_param_spec_pointer ("buffer",
                              "TPM2 response buffer",
                              "memory buffer holding a TPM2 response",
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SESSION] =
        g_param_spec_object ("session",
                             "Session object",
                             "The Session object that sent the response",
                             TYPE_SESSION_DATA,
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
tpm2_response_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "Tpm2Response",
                                              sizeof (Tpm2ResponseClass),
                                              (GClassInitFunc) tpm2_response_class_init,
                                              sizeof (Tpm2Response),
                                              NULL,
                                              0);
    }
    return type;
}
/**
 * Boilerplate constructor, but some GObject properties would be nice.
 */
Tpm2Response*
tpm2_response_new (SessionData     *session,
                   guint8          *buffer)
{
    return TPM2_RESPONSE (g_object_new (TYPE_TPM2_RESPONSE,
                                       "buffer",  buffer,
                                       "session", session,
                                       NULL));
}
/**
 * This is a convenience wrapper that is used to create an error response
 * where all we need is a header with the RC set to something specific.
 */
Tpm2Response*
tpm2_response_new_rc (SessionData *session,
                      TPM_RC       rc)
{
    guint8 *buffer;

    buffer = calloc (1, TPM_RESPONSE_HEADER_SIZE);
    TPM_RESPONSE_TAG (buffer)  = htobe16 (TPM_ST_NO_SESSIONS);
    TPM_RESPONSE_SIZE (buffer) = htobe32 (TPM_RESPONSE_HEADER_SIZE);
    TPM_RESPONSE_CODE (buffer) = htobe32 (rc);
    return tpm2_response_new (session, buffer);
}
/**
 */
guint8*
tpm2_response_get_buffer (Tpm2Response *response)
{
    return response->buffer;
}
/**
 */
TPM_RC
tpm2_response_get_code (Tpm2Response *response)
{
    return be32toh (TPM_RESPONSE_CODE (response->buffer));
}
/**
 */
guint32
tpm2_response_get_size (Tpm2Response *response)
{
    return be32toh (TPM_RESPONSE_SIZE (response->buffer));
}
/**
 */
TPM_ST
tpm2_response_get_tag (Tpm2Response *response)
{
    return be16toh (TPM_RESPONSE_TAG (response->buffer));
}
/**
 */
SessionData*
tpm2_response_get_session (Tpm2Response *response)
{
    return response->session;
}
