#include "thread-interface.h"


static void
thread_default_init (ThreadInterface *iface)
{
/* noop, required by G_DEFINE_INTERFACE */
}

GType
thread_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                              "thread",
                                              sizeof (ThreadInterface),
                                              (GClassInitFunc)thread_default_init,
                                              0,
                                              (GInstanceInitFunc)NULL,
                                              (GTypeFlags) 0);
    }
    return type;
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
    g_return_val_if_fail (IS_THREAD (self), NULL);
    iface = THREAD_GET_INTERFACE (self);
    g_return_val_if_fail (iface->cancel != NULL, NULL);

    return iface->cancel (self);
}
gint
thread_join (Thread *self)
{
    ThreadInterface *iface;

    g_debug ("thread_join");
    g_return_val_if_fail (IS_THREAD (self), NULL);
    iface = THREAD_GET_INTERFACE (self);
    g_return_val_if_fail (iface->join != NULL, NULL);

    return iface->join (self);
}
gint
thread_start (Thread *self)
{
    ThreadInterface *iface;

    g_debug ("thread_start");
    g_return_val_if_fail (IS_THREAD (self), NULL);
    iface = THREAD_GET_INTERFACE (self);
    g_return_val_if_fail (iface->start != NULL, NULL);

    return iface->start (self);
}
