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
tcti_transmit (Tcti      *self,
               size_t     size,
               uint8_t   *command)
{
    return tss2_tcti_transmit (TCTI_GET_INTERFACE (self)->tcti_context,
                               size,
                               command);
}
TSS2_RC
tcti_receive (Tcti      *self,
              size_t    *size,
              uint8_t   *response,
              int32_t    timeout)
{
    return tss2_tcti_receive (TCTI_GET_INTERFACE (self)->tcti_context,
                              size,
                              response,
                              timeout);
}
TSS2_RC
tcti_cancel (Tcti   *self)
{
    return tss2_tcti_cancel (TCTI_GET_INTERFACE (self)->tcti_context);
}
TSS2_RC
tcti_set_locality (Tcti     *self,
                   uint8_t   locality)
{
    return tss2_tcti_set_locality (TCTI_GET_INTERFACE (self)->tcti_context,
                                   locality);
}
