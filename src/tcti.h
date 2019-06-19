/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TCTI_H
#define TCTI_H

#include <glib-object.h>
#include <tss2/tss2_tcti.h>

G_BEGIN_DECLS

typedef struct _Tcti      Tcti;
typedef struct _TctiClass TctiClass;

struct _TctiClass {
    GObjectClass        parent;
};

struct _Tcti {
    GObject             parent;
    TSS2_TCTI_CONTEXT  *tcti_context;
};

#define TYPE_TCTI             (tcti_get_type ())
#define TCTI(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TCTI, Tcti))
#define IS_TCTI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TCTI))
#define TCTI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI, TctiClass))
#define IS_TCTI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI))
#define TCTI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI, TctiClass))

GType               tcti_get_type        (void);
Tcti*               tcti_new             (TSS2_TCTI_CONTEXT *ctx);
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
