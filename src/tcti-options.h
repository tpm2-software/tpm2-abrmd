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
#ifndef TCTI_OPTIONS_H
#define TCTI_OPTIONS_H

#include <glib-object.h>
#include "tcti.h"
#include "tcti-type-enum.h"

G_BEGIN_DECLS

#ifndef G_OPTION_FLAG_NONE
#define G_OPTION_FLAG_NONE 0
#endif

typedef struct _TctiOptionsClass {
    GObjectClass     parent;
} TctiOptionsClass;

typedef struct _TctiOptions
{
    GObject          parent_instance;
    TctiTypeEnum     tcti_type;
    gchar           *device_name;
    gchar           *socket_address;
    guint            socket_port;
    guint            echo_size;
} TctiOptions;

#define TYPE_TCTI_OPTIONS             (tcti_options_get_type       ())
#define TCTI_OPTIONS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_OPTIONS, TctiOptions))
#define TCTI_OPTIONS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_OPTIONS, TctiOptionsClass))
#define IS_TCTI_OPTIONS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_OPTIONS))
#define IS_TCTI_OPTIONS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_OPTIONS))
#define TCTI_OPTIONS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_OPTIONS, TctiOptionsClass))

GType         tcti_options_get_type  (void);
TctiOptions*  tcti_options_new       (void);
GOptionGroup* tcti_options_get_group (TctiOptions *options);
Tcti*         tcti_options_get_tcti  (TctiOptions *options);

G_END_DECLS
#endif /* TCTI_OPTIONS_H */
