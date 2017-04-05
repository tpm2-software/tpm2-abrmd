#include "thread-interface.h"

G_DEFINE_INTERFACE (Thread, thread, G_TYPE_INVALID);

static void
thread_default_init (ThreadInterface *iface)
{
/* noop, required by G_DEFINE_INTERFACE */
}
/**
 * AFAIK this is boilerplate for 'interface' / abstract objects. All we do
 * is get a reference to the interface for the parameter object 'self',
 * check to be sure the function pointer we care about is not null, then
 * we call it and return the result.
 */
gint
thread_cancel (Thread *self)
{
    ThreadInterface *iface;

    g_debug ("tab_pthread_cancel");
    g_return_val_if_fail (IS_THREAD (self), -1);
    iface = THREAD_GET_INTERFACE (self);
    g_return_val_if_fail (iface->cancel != NULL, -1);

    return iface->cancel (self);
}
gint
thread_join (Thread *self)
{
    ThreadInterface *iface;

    g_debug ("thread_join");
    g_return_val_if_fail (IS_THREAD (self), -1);
    iface = THREAD_GET_INTERFACE (self);
    g_return_val_if_fail (iface->join != NULL, -1);

    return iface->join (self);
}
gint
thread_start (Thread *self)
{
    ThreadInterface *iface;

    g_debug ("thread_start");
    g_return_val_if_fail (IS_THREAD (self), -1);
    iface = THREAD_GET_INTERFACE (self);
    g_return_val_if_fail (iface->start != NULL, -1);

    return iface->start (self);
}
