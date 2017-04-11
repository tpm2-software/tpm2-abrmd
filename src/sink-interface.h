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
#ifndef SINK_INTERFACE_H
#define SINK_INTERFACE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define TYPE_SINK                (sink_get_type ())
#define SINK(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SINK, Sink))
#define IS_SINK(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SINK))
#define SINK_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TYPE_SINK, SinkInterface))

typedef struct _Sink              Sink;
typedef struct _SinkInterface     SinkInterface;

/* types for function pointers defined by interface */
typedef void        (*SinkEnqueue)        (Sink             *self,
                                           GObject          *obj);

struct _SinkInterface {
    GTypeInterface         parent;
    SinkEnqueue            enqueue;
};

GType      sink_get_type     (void);
void       sink_enqueue      (Sink           *self,
                              GObject        *obj);

G_END_DECLS
#endif
