/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
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
