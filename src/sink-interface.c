/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */

#include "util.h"
#include "sink-interface.h"

G_DEFINE_INTERFACE (Sink, sink, G_TYPE_INVALID);

static void
sink_default_init (SinkInterface *iface)
{
    UNUSED_PARAM(iface);
    /* noop, required by G_DEFINE_INTERFACE */
}

/**
 * boilerplate code to call enqueue function in class implementing the
 * interface
 */
void
sink_enqueue (Sink       *self,
              GObject    *obj)
{
    SinkInterface *iface;

    g_debug ("sink_enqueue");
    g_return_if_fail (IS_SINK (self));
    iface = SINK_GET_INTERFACE (self);
    g_return_if_fail (iface->enqueue != NULL);

    iface->enqueue (self, obj);
}
