#include "tcti-interface.h"

G_DEFINE_INTERFACE (Tcti, tcti, G_TYPE_OBJECT);

static void
tcti_default_init (TctiInterface *iface)
{
/* noop, required by G_DEFINE_INTERFACE */
}

TSS2_TCTI_CONTEXT*
tcti_get_context (Tcti  *self)
{
    TctiInterface *iface;
    g_debug ("tcti_get_context");

    g_return_val_if_fail (IS_TCTI (self), NULL);

    iface = TCTI_GET_INTERFACE (self);
    g_return_val_if_fail (iface->get_context != NULL, NULL);
    return iface->get_context (self);
}
