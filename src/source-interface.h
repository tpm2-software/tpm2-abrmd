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
#ifndef SOURCE_INTERFACE_H
#define SOURCE_INTERFACE_H

#include <glib-object.h>

#include "sink-interface.h"

G_BEGIN_DECLS

#define TYPE_SOURCE                (source_get_type ())
#define SOURCE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj),     TYPE_SOURCE, Source))
#define IS_SOURCE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj),     TYPE_SOURCE))
#define SOURCE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TYPE_SOURCE, SourceInterface))

typedef struct _Source              Source;
typedef struct _SourceInterface     SourceInterface;

/* types for function pointers defined by interface */
typedef void        (*SourceAddSink)        (Source        *self,
                                             Sink          *sink);

struct _SourceInterface {
    GTypeInterface         parent;
    SourceAddSink          add_sink;
};

GType      source_get_type         (void);
void       source_add_sink         (Source         *self,
                                    Sink           *sink);

G_END_DECLS
#endif /* SOURCE_INTERFACE_H */
