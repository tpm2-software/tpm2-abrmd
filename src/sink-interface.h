/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
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
