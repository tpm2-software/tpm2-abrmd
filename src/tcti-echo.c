#include <errno.h>
#include <string.h>

#include "tcti-echo.h"
#include "tss2-tcti-echo.h"

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
tcti_echo_finalize (GObject *obj)
{
    Tcti *tcti = TCTI (obj);

    if (tcti->tcti_context != NULL) {
        tss2_tcti_finalize (tcti->tcti_context);
        g_free (tcti->tcti_context);
    }
    if (tcti_echo_parent_class)
        G_OBJECT_CLASS (tcti_echo_parent_class)->finalize (obj);
}
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_echo_class_init (gpointer klass)
{
    GObjectClass  *object_class = G_OBJECT_CLASS  (klass);
    TctiClass *base_class   = TCTI_CLASS (klass);

    if (tcti_echo_parent_class == NULL)
        tcti_echo_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_echo_finalize;
    object_class->get_property = tcti_echo_get_property;
    object_class->set_property = tcti_echo_set_property;

    base_class->initialize     = (TctiInitFunc)tcti_echo_initialize;

    obj_properties[PROP_SIZE] =
        g_param_spec_uint ("size",
                           "buffer size",
                           "Size for allocated internal buffer",
                           TSS2_TCTI_ECHO_MIN_BUF,
                           TSS2_TCTI_ECHO_MAX_BUF,
                           TSS2_TCTI_ECHO_MAX_BUF,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

/* Upon first call to *_get_type we register the type with the GType system.
 * We keep a static GType around to speed up future calls.
 */
GType
tcti_echo_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (TYPE_TCTI,
                                              "TctiEcho",
                                              sizeof (TctiEchoClass),
                                              (GClassInitFunc) tcti_echo_class_init,
                                              sizeof (TctiEcho),
                                              NULL,
                                              0);
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
tcti_echo_initialize (TctiEcho *tcti_echo)
{
    Tcti          *tcti  = TCTI (tcti_echo);
    TSS2_RC        rc    = TSS2_RC_SUCCESS;
    size_t         ctx_size;

    if (tcti->tcti_context)
        goto out;
    rc = tss2_tcti_echo_init (NULL, &ctx_size, tcti_echo->size);
    if (rc != TSS2_RC_SUCCESS)
        goto out;
    g_debug ("allocating tcti_context: 0x%x", ctx_size);
    tcti->tcti_context = g_malloc0 (ctx_size);
    rc = tss2_tcti_echo_init (tcti->tcti_context, &ctx_size, tcti_echo->size);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to initialize echo TCTI context: 0x%x", rc);
        g_free (tcti->tcti_context);
        tcti->tcti_context = NULL;
    }
out:
    return rc;
}
