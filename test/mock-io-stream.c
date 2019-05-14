/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>

#include "mock-io-stream.h"
#include "util.h"

G_DEFINE_TYPE (MockIOStream, mock_io_stream, G_TYPE_IO_STREAM);

enum {
    PROP_0,
    PROP_ISTREAM,
    PROP_OSTREAM,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL };

static void
mock_io_stream_set_property (GObject      *object,
                             guint         property_id,
                             GValue const *value,
                             GParamSpec   *pspec)
{
    MockIOStream *self = MOCK_IO_STREAM (object);

    g_debug ("%s: MockIOStream 0x%" PRIxPTR, __func__, (uintptr_t)self);
    switch (property_id) {
    case PROP_ISTREAM:
        g_debug ("%s: GInputStream: 0x%" PRIxPTR,
                 __func__, (uintptr_t)self->istream);
        self->istream = g_value_dup_object (value);
        break;
    case PROP_OSTREAM:
        self->ostream = g_value_dup_object (value);
        g_debug ("%s: GOutputStream: 0x%" PRIxPTR,
                 __func__, (uintptr_t)self->ostream);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
mock_io_stream_get_property (GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
    MockIOStream *self = MOCK_IO_STREAM (object);

    g_debug ("%s: MockIOStream 0x%" PRIxPTR, __func__, (uintptr_t)self);
    switch (property_id) {
    case PROP_ISTREAM:
        g_value_set_object (value, self->istream);
        break;
    case PROP_OSTREAM:
        g_value_set_object (value, self->ostream);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static GInputStream*
mock_io_stream_get_istream (GIOStream *iostream)
{
    return MOCK_IO_STREAM (iostream)->istream;
}

static GOutputStream*
mock_io_stream_get_ostream (GIOStream *iostream)
{
    return MOCK_IO_STREAM (iostream)->ostream;
}

static void
mock_io_stream_init (MockIOStream *self)
{
    UNUSED_PARAM (self);
}

/*
 * This dispose function is kinda weird:
 * The parent class closes the underlying stream objects for us in its own
 * dispose function. Typically we would unref all of our objects here and then
 * chain up to the parent but if we do that the stream objects will have been
 * destroyed and the parent class will throw a bunch of CRITICAL error
 * messages on account of it.
 *
 * This leaves us with the option of either doing the unref-ing in the
 * finalize function (which all the gobject docs says is the wrong place to
 * do this) or we can do this after we chain up to the parent. I've opted for
 * the later.
 */
static void
mock_io_stream_dispose (GObject *obj)
{
    MockIOStream *self = MOCK_IO_STREAM (obj);

    G_OBJECT_CLASS (mock_io_stream_parent_class)->dispose (obj);
    g_clear_object (&self->istream);
    g_clear_object (&self->ostream);
}

static void
mock_io_stream_class_init (MockIOStreamClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    GIOStreamClass *stream_class = G_IO_STREAM_CLASS (klass);

    if (mock_io_stream_parent_class == NULL) {
        mock_io_stream_parent_class = g_type_class_peek_parent (klass);
    }

    obj_class->dispose = mock_io_stream_dispose;
    obj_class->get_property = mock_io_stream_get_property;
    obj_class->set_property = mock_io_stream_set_property;
    stream_class->get_input_stream = mock_io_stream_get_istream;
    stream_class->get_output_stream = mock_io_stream_get_ostream;

    obj_properties [PROP_ISTREAM] =
        g_param_spec_object ("input-stream",
                             "input stream",
                             "input stream",
                             G_TYPE_INPUT_STREAM,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_OSTREAM] =
        g_param_spec_object ("output-stream",
                             "output stream",
                             "output stream",
                             G_TYPE_OUTPUT_STREAM,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (obj_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

GIOStream*
mock_io_stream_new (GInputStream *istream,
                    GOutputStream *ostream)
{
    return G_IO_STREAM (g_object_new (TYPE_MOCK_IO_STREAM,
                                      "input-stream", istream,
                                      "output-stream", ostream,
                                      NULL));
}
