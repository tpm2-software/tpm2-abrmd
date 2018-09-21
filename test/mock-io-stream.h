/*
 * Copyright (c) 2018, Intel Corporation
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
