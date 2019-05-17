/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */

#include "util.h"
#include "thread.h"

G_DEFINE_ABSTRACT_TYPE (Thread, thread, G_TYPE_OBJECT);

static void
thread_init (Thread *iface)
{
    UNUSED_PARAM(iface);
/* noop, required by G_DEFINE_INTERFACE */
}
static void
thread_class_init (ThreadClass *klass)
{
    klass->thread_run = NULL;
}

gint
thread_start (Thread *self)
{
    if (self->thread_id != 0) {
        g_warning ("thread running");
        return -1;
    }
    return pthread_create (&self->thread_id,
                           NULL,
                           THREAD_GET_CLASS (self)->thread_run,
                           self);
}

void
thread_cancel (Thread *self)
{
    ThreadClass *class = THREAD_GET_CLASS (self);

    if (self->thread_id == 0) {
        g_warning ("thread not running");
        return;
    }

    if (class->thread_unblock != NULL)
        class->thread_unblock (self);
}

gint
thread_join (Thread *self)
{
    if (self->thread_id == 0) {
        g_warning ("thread not running");
        return -1;
    }

    gint ret = pthread_join (self->thread_id, NULL);
    self->thread_id = 0;
    return ret;
}
