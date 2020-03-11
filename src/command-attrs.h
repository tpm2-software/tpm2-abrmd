/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef COMMAND_ATTRS_H
#define COMMAND_ATTRS_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <stdlib.h>

#include <tss2/tss2_tpm2_types.h>

G_BEGIN_DECLS

typedef struct _CommandAttrsClass {
    GObjectClass    parent;
} CommandAttrsClass;

typedef struct _CommandAttrs {
    GObject                parent_instance;
    TPMA_CC               *command_attrs;
    UINT32                 count;
} CommandAttrs;

#include "tpm2.h"

#define TYPE_COMMAND_ATTRS              (command_attrs_get_type   ())
#define COMMAND_ATTRS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_COMMAND_ATTRS, CommandAttrs))
#define COMMAND_ATTRS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_COMMAND_ATTRS, CommandAttrsClass))
#define IS_COMMAND_ATTRS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_COMMAND_ATTRS))
#define IS_COMMAND_ATTRS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_COMMAND_ATTRS))
#define COMMAND_ATTRS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_COMMAND_ATTRS, CommandAttrsClass))

GType            command_attrs_get_type    (void);
CommandAttrs*    command_attrs_new         (void);
gint             command_attrs_init_tpm    (CommandAttrs     *attrs,
                                            Tpm2 *tpm2);
TPMA_CC          command_attrs_from_cc     (CommandAttrs     *attrs,
                                            TPM2_CC            command_code);

G_END_DECLS
#endif /* COMMAND_ATTRS_H */
