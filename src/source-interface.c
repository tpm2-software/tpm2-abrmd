/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */

#include "util.h"
#include "source-interface.h"

G_DEFINE_INTERFACE (Source, source, G_TYPE_INVALID);

static void
source_default_init (SourceInterface *iface)
{
    UNUSED_PARAM(iface);
    /* noop, required by G_DEFINE_INTERFACE */
}

/**
 * boilerplate code to call enqueue function in class implementing the
 * interface
 */
void
source_add_sink (Source       *self,
                 Sink         *sink)
{
    SourceInterface *iface;

    g_debug ("source_add_sink");
    g_return_if_fail (IS_SOURCE (self));
    iface = SOURCE_GET_INTERFACE (self);
    g_return_if_fail (iface->add_sink != NULL);

    iface->add_sink (self, sink);
}
