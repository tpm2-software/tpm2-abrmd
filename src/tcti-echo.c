#include <errno.h>
#include <string.h>

#include "tcti-interface.h"
#include "tcti-echo.h"
#include "tcti-echo-priv.h"

/**
 * Begin TSS2_TCTI_CONTEXT code
 */
TSS2_RC
tcti_echo_transmit (TSS2_TCTI_CONTEXT *tcti_context,
                    size_t             size,
                    uint8_t           *command)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (echo_context == NULL)
        return TSS2_TCTI_RC_BAD_REFERENCE;
    if (echo_context->state != CAN_TRANSMIT)
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    if (size > echo_context->buf_size || size == 0)
        return TSS2_TCTI_RC_BAD_VALUE;

    memcpy (echo_context->buf, command, size);
    echo_context->data_size = size;
    echo_context->state = CAN_RECEIVE;
    return TSS2_RC_SUCCESS;
}

TSS2_RC
tcti_echo_receive (TSS2_TCTI_CONTEXT *tcti_context,
                   size_t            *size,
                   uint8_t           *response,
                   int32_t            timeout)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (echo_context == NULL || size == NULL)
        return TSS2_TCTI_RC_BAD_REFERENCE;
    if (echo_context->state != CAN_RECEIVE)
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    if (*size < echo_context->data_size)
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    memcpy (response, echo_context->buf, echo_context->data_size);
    *size = echo_context->data_size;
    echo_context->state = CAN_TRANSMIT;

    return TSS2_RC_SUCCESS;
}

void
tcti_echo_finalize (TSS2_TCTI_CONTEXT *tcti_context)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (tcti_context == NULL)
        g_error ("tcti_echo_finalize passed NULL TCTI context");
    if (echo_context->buf)
        free (echo_context->buf);
}

TSS2_RC
tcti_echo_cancel (TSS2_TCTI_CONTEXT *tcti_context)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tcti_echo_get_poll_handles (TSS2_TCTI_CONTEXT     *tcti_context,
                            TSS2_TCTI_POLL_HANDLE *handles,
                            size_t                *num_handles)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tcti_echo_set_locality (TSS2_TCTI_CONTEXT *tcti_context,
                        uint8_t            locality)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tcti_echo_init (TSS2_TCTI_CONTEXT *tcti_context,
                size_t            *ctx_size,
                size_t             buf_size)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    g_debug ("tcti_echo_init");
    if (tcti_context == NULL && ctx_size == NULL)
        return TSS2_TCTI_RC_BAD_VALUE;
    if (tcti_context == NULL && ctx_size != NULL) {
        *ctx_size = sizeof (TCTI_ECHO_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    if (buf_size > TCTI_ECHO_MAX_BUF) {
        g_warning ("tcti_echo_init cannot allocate data buffer larger than 0x%x bytes",
                   TCTI_ECHO_MAX_BUF);
        return TSS2_TCTI_RC_BAD_VALUE;
    } else if (buf_size == 0) {
        buf_size = TCTI_ECHO_MAX_BUF;
    }

    TSS2_TCTI_MAGIC (tcti_context)            = TCTI_ECHO_MAGIC;
    TSS2_TCTI_VERSION (tcti_context)          = 1;
    TSS2_TCTI_TRANSMIT (tcti_context)         = tcti_echo_transmit;
    TSS2_TCTI_RECEIVE (tcti_context)          = tcti_echo_receive;
    TSS2_TCTI_FINALIZE (tcti_context)         = tcti_echo_finalize;
    TSS2_TCTI_CANCEL (tcti_context)           = tcti_echo_cancel;
    TSS2_TCTI_GET_POLL_HANDLES (tcti_context) = tcti_echo_get_poll_handles;
    TSS2_TCTI_SET_LOCALITY (tcti_context)     = tcti_echo_set_locality;

    g_debug ("tcti_echo_init allocate data buffer: 0x%x", buf_size);
    echo_context->buf = (uint8_t*)calloc (1, buf_size);
    if (echo_context->buf == NULL)
        g_error ("tcti_echo_init buffer allocation failed: %s", strerror (errno));
    echo_context->buf_size = buf_size;
    echo_context->data_size = 0;
    echo_context->state = CAN_TRANSMIT;

    return TSS2_RC_SUCCESS;
}
/**
 * End TSS2_TCTI_CONTEXT code
 */
/**
 * Begin GObject code.
 */
static gpointer tcti_echo_parent_class = NULL;

enum {
    PROP_0,
    PROP_SIZE,
    N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
tcti_echo_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    TctiEcho *self = TCTI_ECHO (object);

    g_debug ("tcti_echo_set_property");
    switch (property_id) {
    case PROP_SIZE:
        self->size = g_value_get_uint (value);
        g_debug ("  PROP_SIZE: 0x%x", self->size);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tcti_echo_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
    TctiEcho *self = TCTI_ECHO (object);

    g_debug ("tcti_echo_get_property");
    switch (property_id) {
    case PROP_SIZE:
        g_debug ("  PROP_SIZE: 0x%x", self->size);
        g_value_set_uint (value, self->size);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/* override the parent finalize method so we can free the data associated with
 * the TctiEcho instance.
 * NOTE: breaking naming convention due to conflict with TCTI finalize function
 */
static void
tcti_echo_finalize_gobject (GObject *obj)
{
    TctiEcho *tcti_echo = TCTI_ECHO (obj);
    TctiInterface *tcti_iface = TCTI_GET_INTERFACE (obj);

    if (tcti_iface->tcti_context) {
        tss2_tcti_finalize (tcti_iface->tcti_context);
        g_free (tcti_iface->tcti_context);
    }
    if (tcti_echo_parent_class)
        G_OBJECT_CLASS (tcti_echo_parent_class)->finalize (obj);
}
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_echo_class_init (gpointer klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tcti_echo_parent_class == NULL)
        tcti_echo_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_echo_finalize_gobject;
    object_class->get_property = tcti_echo_get_property;
    object_class->set_property = tcti_echo_set_property;

    obj_properties[PROP_SIZE] =
        g_param_spec_uint ("size",
                           "buffer size",
                           "Size for allocated internal buffer",
                           TCTI_ECHO_MIN_BUF,
                           TCTI_ECHO_MAX_BUF,
                           TCTI_ECHO_DEFAULT_BUF,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

static void
tcti_echo_interface_init (gpointer g_iface)
{
    TctiInterface *tcti_interface = (TctiInterface*)g_iface;
    tcti_interface->initialize  = tcti_echo_initialize;
}
/* Upon first call to *_get_type we register the type with the GType system.
 * We keep a static GType around to speed up future calls.
 */
GType
tcti_echo_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_OBJECT,
                                              "TctiEcho",
                                              sizeof (TctiEchoClass),
                                              (GClassInitFunc) tcti_echo_class_init,
                                              sizeof (TctiEcho),
                                              NULL,
                                              0);
        const GInterfaceInfo tcti_info = {
            (GInterfaceInitFunc) tcti_echo_interface_init,
            NULL,
            NULL
        };
        g_type_add_interface_static (type, TYPE_TCTI, &tcti_info);
    }
    return type;
}
TctiEcho*
tcti_echo_new (guint size)
{
    g_debug ("tcti_echo_new with parameter size: 0x%x", size);
    return TCTI_ECHO (g_object_new (TYPE_TCTI_ECHO,
                                    "size", size,
                                    NULL));
}
TSS2_RC
tcti_echo_initialize (Tcti *tcti)
{
    TctiEcho      *self  = TCTI_ECHO (tcti);
    TctiInterface *iface = TCTI_GET_INTERFACE (tcti);
    TSS2_RC        rc    = TSS2_RC_SUCCESS;
    size_t         ctx_size;

    if (iface->tcti_context)
        goto out;
    rc = tcti_echo_init (NULL, &ctx_size, self->size);
    if (rc != TSS2_RC_SUCCESS)
        goto out;
    g_debug ("allocating tcti_context: 0x%x", ctx_size);
    iface->tcti_context = g_malloc0 (ctx_size);
    rc = tcti_echo_init (iface->tcti_context, &ctx_size, self->size);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to initialize echo TCTI context: 0x%x", rc);
        g_free (iface->tcti_context);
        iface->tcti_context;
    }
out:
    return rc;

}
/**
 * End GObject code
 */
