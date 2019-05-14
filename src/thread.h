/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
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
