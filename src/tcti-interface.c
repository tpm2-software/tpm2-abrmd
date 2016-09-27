#include "tcti-interface.h"

G_DEFINE_INTERFACE (Tcti, tcti, G_TYPE_OBJECT);

static void
tcti_default_init (TctiInterface *iface)
{
/* noop, required by G_DEFINE_INTERFACE */
}
/**
 * AFAIK this is boilerplate for 'interface' / abstract objects. All we do
 * is get a reference to the interface for the parameter object 'self',
 * check to be sure it's set to something non-NULL, then we call it and
 * return the result.
 */
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
/**
 * This function should be used carefully. It breaks the encapsulation of
 * the Tcti object and may cause weird side effects if the TSS2_TCTI_CONTEXT
 * structure and the Tcti object end up being used in different places or
 * threads.
 */
TSS2_TCTI_CONTEXT*
tcti_peek_context (Tcti *self)
{
    return TCTI_GET_INTERFACE (self)->tcti_context;
}
/**
 * The rest of these functions are just wrappers around the macros provided
 * by the TSS for calling the TCTI functions. There's no implementation for
 * 'getPollHandles' yet since I've got no need for it yet.
 */
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
