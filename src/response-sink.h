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
#ifndef RESPONSE_SINK_H
#define RESPONSE_SINK_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>

#include "message-queue.h"
#include "thread.h"

G_BEGIN_DECLS

typedef struct _ResponseSinkClass {
    ThreadClass       parent;
} ResponseSinkClass;

/** DON'T TOUCH!
 * Direct access to data in this structure his highly discouraged. Use the
 * functions in this header to interact with this "object". If you're tempted
 * to access the structure directly you probably need to update the API.
 */
typedef struct _ResponseSink {
    Thread             parent_instance;
    MessageQueue      *in_queue;
} ResponseSink;

#define TYPE_RESPONSE_SINK              (response_sink_get_type ())
#define RESPONSE_SINK(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_RESPONSE_SINK, ResponseSink))
#define RESPONSE_SINK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_RESPONSE_SINK, ResponseSinkClass))
#define IS_RESPONSE_SINK(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_RESPONSE_SINK))
#define IS_RESPONSE_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_RESPONSE_SINK))
#define RESPONSE_SINK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_RESPONSE_SINK, ResponseSinkClass))

GType               response_sink_get_type    (void);
ResponseSink*       response_sink_new         (void);

G_END_DECLS
#endif /* RESPONSE_SINK_H */
