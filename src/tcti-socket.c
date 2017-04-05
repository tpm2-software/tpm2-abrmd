#include "tcti-socket.h"

G_DEFINE_TYPE (TctiSocket, tcti_socket, TYPE_TCTI);

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
    TctiSocket    *tcti_socket = TCTI_SOCKET (obj);
    Tcti          *tcti        = TCTI (obj);

    if (tcti->tcti_context) {
        tss2_tcti_finalize (tcti->tcti_context);
        g_free (tcti->tcti_context);
    }
    g_free (tcti_socket->address);
    if (tcti_socket_parent_class)
        G_OBJECT_CLASS (tcti_socket_parent_class)->finalize (obj);
}
static void
tcti_socket_init (TctiSocket *socket)
{ /* noop */ }
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_socket_class_init (TctiSocketClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    TctiClass    *tcti_class   = TCTI_CLASS (klass);

    g_debug ("tcti_socket_class_init");
    if (tcti_socket_parent_class == NULL)
        tcti_socket_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_socket_finalize;
    object_class->get_property = tcti_socket_get_property;
    object_class->set_property = tcti_socket_set_property;

    tcti_class->initialize = (TctiInitFunc)tcti_socket_initialize;

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
/**
 * Allocate a new TctiSocket object and initialize the 'tcti_context'
 * variable to NULL.
 */
TctiSocket*
tcti_socket_new (const gchar *address,
                 guint        port)
{
    TctiSocket *tcti;

    tcti = TCTI_SOCKET (g_object_new (TYPE_TCTI_SOCKET,
                                      "address", address,
                                      "port", port,
                                      NULL));
    return tcti;
}
/**
 * Initialize an instance of a TSS2_TCTI_CONTEXT for the socket Tcti.
 */
TSS2_RC
tcti_socket_initialize (TctiSocket *self)
{
    Tcti          *tcti  = TCTI (self);
    TSS2_RC        rc    = TSS2_RC_SUCCESS;
    size_t ctx_size;

    TCTI_SOCKET_CONF config = {
        .hostname          = self->address,
        .port              = (guint16)self->port,
        .logCallback       = NULL,
        .logBufferCallback = NULL,
        .logData           = NULL,
    };

    if (tcti->tcti_context != NULL)
        goto out;
    rc = InitSocketTcti (NULL, &ctx_size, NULL, 0);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to get size for socket TCTI context structure: "
                   "0x%x", rc);
        goto out;
    }
    tcti->tcti_context = g_malloc0 (ctx_size);
    if (tcti->tcti_context == NULL) {
        g_warning ("Failed to allocate memory");
        goto out;
    }
    rc = InitSocketTcti (tcti->tcti_context, &ctx_size, &config, 0);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to initialize socket TCTI context: 0x%x", rc);
        g_free (tcti->tcti_context);
        tcti->tcti_context = NULL;
    }
out:
    return rc;
}
