/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef MOCK_IO_STREAM_H
#define MOCK_IO_STREAM_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <tss2/tss2_tpm2_types.h>

G_BEGIN_DECLS

typedef struct _MockIOStreamClass {
    GIOStreamClass      parent;
} MockIOStreamClass;

typedef struct _MockIOStream {
    GIOStream           parent_instance;
    GInputStream       *istream;
    GOutputStream      *ostream;
} MockIOStream;

#define TYPE_MOCK_IO_STREAM             (mock_io_stream_get_type         ())
#define MOCK_IO_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_MOCK_IO_STREAM, MockIOStream))
#define MOCK_IO_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_MOCK_IO_STREAM, MockIOStreamClass))
#define IS_MOCK_IO_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_MOCK_IO_STREAM))
#define IS_MOCK_IO_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_MOCK_IO_STREAM))
#define MOCK_IO_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_MOCK_IO_STREAM, MockIOStreamClass))

GType                mock_io_stream_get_type       (void);
GIOStream*           mock_io_stream_new            (GInputStream *istream,
                                                    GOutputStream *ostream);

G_END_DECLS

#endif /* MOCK_IO_STREAM_H */
