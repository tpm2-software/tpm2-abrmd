#include "thread.h"

G_DEFINE_ABSTRACT_TYPE (Thread, thread, G_TYPE_OBJECT);

static void
thread_init (Thread *iface)
{
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

gint
thread_cancel (Thread *self)
{
    ThreadClass *class = THREAD_GET_CLASS (self);
    gint thread_ret = 0;

    if (self->thread_id == 0) {
        g_warning ("thread not running");
        return -1;
    }
    thread_ret = pthread_cancel (self->thread_id);
    if (class->thread_unblock)
        class->thread_unblock (self);
    return thread_ret;
}

gint
thread_join (Thread *self)
{
    gint ret = pthread_join (self->thread_id, NULL);
    self->thread_id = 0;
    return ret;
}
