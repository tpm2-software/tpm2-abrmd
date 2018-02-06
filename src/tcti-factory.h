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
/*
 * The TctiOption object is a slightly missnamed factory object responsible
 * for instantiating TCTI objects. Additionally this object encapsulates all
 * knowledge of TCTI options and so we provide a _get_group function that
 * returns a GOptionGroup that the caller can use to provide users with info
 * about the supported TCTIs and the structure of their parameters.
 * Once this GOptionGroup has been used to collect arguments from the
 * commandline then the caller may call _get_tcti to get the factory to
 * produce / return a Tcti object configured according to the options
 * provided.
 */
#ifndef TCTI_FACTORY_H
#define TCTI_FACTORY_H

#include <glib-object.h>
#include "tcti.h"
#include "tcti-type-enum.h"

G_BEGIN_DECLS

#ifndef G_OPTION_FLAG_NONE
#define G_OPTION_FLAG_NONE 0
#endif

typedef struct _TctiFactoryClass {
    GObjectClass     parent;
} TctiFactoryClass;

typedef struct _TctiFactory
{
    GObject          parent_instance;
    TctiTypeEnum     tcti_type;
    gchar           *device_name;
    gchar           *socket_address;
    guint            socket_port;
    gchar           *file_name;
    gchar           *conf_str;
} TctiFactory;

#define TYPE_TCTI_FACTORY             (tcti_factory_get_type       ())
#define TCTI_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_FACTORY, TctiFactory))
#define TCTI_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_FACTORY, TctiFactoryClass))
#define IS_TCTI_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_FACTORY))
#define IS_TCTI_FACTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_FACTORY))
#define TCTI_FACTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_FACTORY, TctiFactoryClass))

GType         tcti_factory_get_type  (void);
TctiFactory*  tcti_factory_new       (void);
GOptionGroup* tcti_factory_get_group (TctiFactory *options);
Tcti*         tcti_factory_get_tcti  (TctiFactory *options);

G_END_DECLS
#endif /* TCTI_FACTORY_H */
