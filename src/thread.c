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
    gint ret = pthread_join (self->thread_id, NULL);
    self->thread_id = 0;
    return ret;
}
