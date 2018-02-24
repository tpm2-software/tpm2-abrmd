/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
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
    return Tss2_Tcti_Transmit (self->tcti_context,
                               size,
                               command);
}
TSS2_RC
tcti_receive (Tcti      *self,
              size_t    *size,
              uint8_t   *response,
              int32_t    timeout)
{
    return Tss2_Tcti_Receive (self->tcti_context,
                              size,
                              response,
                              timeout);
}
TSS2_RC
tcti_cancel (Tcti  *self)
{
    return Tss2_Tcti_Cancel (self->tcti_context);
}
TSS2_RC
tcti_set_locality (Tcti     *self,
                   uint8_t   locality)
{
    return Tss2_Tcti_SetLocality (self->tcti_context, locality);
}
