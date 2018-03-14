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
#ifndef COMMAND_ATTRS_H
#define COMMAND_ATTRS_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <stdlib.h>

#include <tss2/tpm20.h>

G_BEGIN_DECLS

typedef struct _CommandAttrsClass {
    GObjectClass    parent;
} CommandAttrsClass;

typedef struct _CommandAttrs {
    GObject                parent_instance;
    TPMA_CC               *command_attrs;
    UINT32                 count;
} CommandAttrs;

#include "access-broker.h"

#define TYPE_COMMAND_ATTRS              (command_attrs_get_type   ())
#define COMMAND_ATTRS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_COMMAND_ATTRS, CommandAttrs))
#define COMMAND_ATTRS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_COMMAND_ATTRS, CommandAttrsClass))
#define IS_COMMAND_ATTRS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_COMMAND_ATTRS))
#define IS_COMMAND_ATTRS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_COMMAND_ATTRS))
#define COMMAND_ATTRS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_COMMAND_ATTRS, CommandAttrsClass))

GType            command_attrs_get_type    (void);
CommandAttrs*    command_attrs_new         (void);
gint             command_attrs_init_tpm    (CommandAttrs     *attrs,
                                            AccessBroker     *broker);
TPMA_CC          command_attrs_from_cc     (CommandAttrs     *attrs,
                                            TPM2_CC            command_code);

G_END_DECLS
#endif /* COMMAND_ATTRS_H */
