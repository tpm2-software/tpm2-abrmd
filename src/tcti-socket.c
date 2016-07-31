#include "tcti-socket.h"

/* Boiler-plate gobject code. */
static gpointer tcti_socket_parent_class = NULL;

enum {
    PROP_0,
    PROP_ADDRESS,
    PROP_PORT,
    N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
tcti_socket_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    TctiSocket *self = TCTI_SOCKET (object);

    switch (property_id) {
    case PROP_ADDRESS:
        g_free (self->address);
        self->address = g_value_dup_string (value);
        g_debug ("TctiSocket set address: %s", self->address);
        break;
    case PROP_PORT:
        /* uint -> uint16 */
        self->port = g_value_get_uint (value);
        g_debug ("TctiSocket set port: %u", self->port);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tcti_socket_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    TctiSocket *self = TCTI_SOCKET (object);

    switch (property_id) {
    case PROP_ADDRESS:
        g_value_set_string (value, self->address);
        break;
    case PROP_PORT:
        g_value_set_uint (value, self->port);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/* override the parent finalize method so we can free the data associated with
 * the TctiSocket instance.
 */
static void
tcti_socket_finalize (GObject *obj)
{
    TctiSocket *tcti_socket = TCTI_SOCKET (obj);

    g_free (tcti_socket->address);
    if (tcti_socket_parent_class)
        G_OBJECT_CLASS (tcti_socket_parent_class)->finalize (obj);
}
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_socket_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tcti_socket_parent_class == NULL)
        tcti_socket_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_socket_finalize;
    object_class->get_property = tcti_socket_get_property;
    object_class->set_property = tcti_socket_set_property;

    obj_properties[PROP_ADDRESS] =
        g_param_spec_string ("address",
                             "Socket address",
                             "Address for connection to TPM",
                             "/dev/tpm0",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_PORT] = 
        g_param_spec_uint ("port",
                           "Socket port",
                           "Port for connection to TPM",
                           0,
                           65535, /* max for uint16 */
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

static void
tcti_socket_interface_init (gpointer g_iface)
{
    TctiInterface *tcti_interface = (TctiInterface*)g_iface;
    tcti_interface->get_context   = tcti_socket_get_context;
    tcti_interface->initialize    = tcti_socket_initialize;
}
/* Upon first call to *_get_type we register the type with the GType system.
 * We keep a static GType around to speed up future calls.
 */
GType
tcti_socket_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "TctiSocket",
                                              sizeof (TctiSocketClass),
                                              (GClassInitFunc) tcti_socket_class_init,
                                              sizeof (TctiSocket),
                                              NULL,
                                              0);
        const GInterfaceInfo tcti_info = {
            (GInterfaceInitFunc) tcti_socket_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_TCTI, &tcti_info);
    }
    return type;
}
TctiSocket*
tcti_socket_new (const gchar *address,
                 guint16      port)
{
    return TCTI_SOCKET (g_object_new (TYPE_TCTI_SOCKET,
                                      "address", address,
                                      "port", port,
                                      NULL));
}
TSS2_RC
tcti_socket_initialize (Tcti *tcti)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    size_t ctx_size;
    TctiSocket *self = TCTI_SOCKET (tcti);

    TCTI_SOCKET_CONF config = {
        .hostname          = self->address,
        .port              = self->port,
        .logCallback       = NULL,
        .logBufferCallback = NULL,
        .logData           = NULL,
    };

    if (self->tcti_context == NULL) {
        rc = InitSocketTcti (NULL, &ctx_size, NULL, 0);
        if (rc != TSS2_RC_SUCCESS)
            goto out;
        self->tcti_context = g_malloc0 (ctx_size);
        rc = InitSocketTcti (self->tcti_context, &ctx_size, &config, 0);
        if (rc != TSS2_RC_SUCCESS)
            g_free (self->tcti_context);
    }
out:
    return rc;
}
/* Create and expose the internal TCTI context needed by the TSS / SAPI */
TSS2_RC
tcti_socket_get_context (Tcti               *tcti,
                         TSS2_TCTI_CONTEXT **ctx)
{
    TSS2_RC rc;
    TctiSocket *self = TCTI_SOCKET (tcti);

    if (self->tcti_context == NULL)
        rc = tcti_socket_initialize (tcti);
    *ctx = self->tcti_context;
    return rc;
}
