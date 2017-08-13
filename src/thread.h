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
#ifndef THREAD_INTERFACE_H
#define THREAD_INTERFACE_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _Thread Thread;
typedef struct _ThreadClass ThreadClass;

typedef void* (*ThreadFunc)        (void *user_data);
typedef void  (*ThreadUnblockFunc) (Thread *self);

struct _ThreadClass {
    GObjectClass      parent;
    ThreadFunc        thread_run;
    ThreadUnblockFunc thread_unblock;
};

struct _Thread {
    GObject     parent;
    pthread_t   thread_id;
};

#define TYPE_THREAD             (thread_get_type ())
#define THREAD(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_THREAD, Thread))
#define IS_THREAD(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_THREAD))
#define THREAD_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_THREAD, ThreadClass))
#define IS_THREAD_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_THREAD))
#define THREAD_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_THREAD, ThreadClass))

GType           thread_get_type     (void);
void            thread_cancel       (Thread            *self);
gint            thread_join         (Thread            *self);
gint            thread_start        (Thread            *self);

G_END_DECLS
#endif /* THREAD_INTERFACE_H */
