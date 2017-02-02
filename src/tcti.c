#include <inttypes.h>

#include "tcti.h"

G_DEFINE_ABSTRACT_TYPE (Tcti, tcti, G_TYPE_OBJECT);

static void
tcti_init (Tcti *tcti)
{
/* noop, required by G_DEFINE_ABSTRACT_TYPE */
    tcti->tcti_context = NULL;
}
static void
tcti_class_init (TctiClass *klass)
{
    klass->initialize   = tcti_initialize;
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
    g_debug ("tcti_initialize: 0x%" PRIxPTR, (uintptr_t)self);
    return TCTI_GET_CLASS (self)->initialize (self);
}

TSS2_TCTI_CONTEXT*
tcti_peek_context (Tcti *self)
{
    g_debug ("tcti_peek_context: 0x%" PRIxPTR, (uintptr_t)self);
    return self->tcti_context;
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
    return tss2_tcti_transmit (self->tcti_context,
                               size,
                               command);
}
TSS2_RC
tcti_receive (Tcti      *self,
              size_t    *size,
              uint8_t   *response,
              int32_t    timeout)
{
    return tss2_tcti_receive (self->tcti_context,
                              size,
                              response,
                              timeout);
}
TSS2_RC
tcti_cancel (Tcti  *self)
{
    return tss2_tcti_cancel (self->tcti_context);
}
TSS2_RC
tcti_set_locality (Tcti     *self,
                   uint8_t   locality)
{
    return tss2_tcti_set_locality (self->tcti_context, locality);
}
