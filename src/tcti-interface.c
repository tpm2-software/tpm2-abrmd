#include "tcti-interface.h"

G_DEFINE_INTERFACE (Tcti, tcti, G_TYPE_OBJECT);

static void
tcti_default_init (TctiInterface *iface)
{
/* noop, required by G_DEFINE_INTERFACE */
}

TSS2_RC
tcti_initialize (Tcti *self)
{
    TctiInterface *iface;

    g_debug ("tcti_initialize");
    g_return_val_if_fail (IS_TCTI (self), NULL);
    iface = TCTI_GET_INTERFACE (self);
    g_return_val_if_fail (iface->initialize != NULL, NULL);

    return iface->initialize (self);
}

TSS2_RC
tcti_get_context (Tcti               *self,
                  TSS2_TCTI_CONTEXT **ctx)
{
    TctiInterface *iface;
    g_debug ("tcti_get_context");

    g_return_val_if_fail (IS_TCTI (self), NULL);

    iface = TCTI_GET_INTERFACE (self);
    g_return_val_if_fail (iface->get_context != NULL, NULL);

    return iface->get_context (self, ctx);
}
