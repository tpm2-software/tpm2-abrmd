/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
#ifndef TCTI_H
#define TCTI_H

#include <glib-object.h>
#include <tss2/tss2_tcti.h>

G_BEGIN_DECLS

typedef struct _Tcti      Tcti;
typedef struct _TctiClass TctiClass;

typedef TSS2_RC (*TctiInitFunc) (Tcti *self);

struct _Tcti {
    GObject             parent;
    TSS2_TCTI_CONTEXT  *tcti_context;
};

struct _TctiClass {
    GObjectClass        parent;
    TctiInitFunc        initialize;
};

#define TYPE_TCTI             (tcti_get_type ())
#define TCTI(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TCTI, Tcti))
#define IS_TCTI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TCTI))
#define TCTI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI, TctiClass))
#define IS_TCTI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI))
#define TCTI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI, TctiClass))

GType               tcti_get_type        (void);
TSS2_RC             tcti_initialize      (Tcti            *self);
TSS2_RC             tcti_transmit        (Tcti            *self,
                                          size_t           size,
                                          uint8_t         *command);
TSS2_RC             tcti_receive         (Tcti            *self,
                                          size_t          *size,
                                          uint8_t         *response,
                                          int32_t          timeout);
TSS2_RC             tcti_cancel          (Tcti            *self);
TSS2_RC             tcti_set_locality    (Tcti            *self,
                                          uint8_t          locality);
TSS2_TCTI_CONTEXT*  tcti_peek_context    (Tcti            *self);

G_END_DECLS
#endif /* TCTI_H */
