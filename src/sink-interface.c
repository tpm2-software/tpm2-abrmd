#include "sink-interface.h"

static void
sink_default_init (SinkInterface *iface)
{
    /* noop, requied by G_DEFINE_INTERFACE */
}

GType
sink_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                              "sink",
                                              sizeof (SinkInterface),
                                              (GClassInitFunc)sink_default_init,
                                              0,
                                              (GInstanceInitFunc)NULL,
                                              (GTypeFlags) 0);
    }
    return type;
}
/**
 * boilerplate code to call enqueue function in class implementing the
 * interface
 */
void
sink_enqueue (Sink       *self,
              GObject    *obj)
{
    SinkInterface *iface;

    g_debug ("sink_enqueue");
    g_return_if_fail (IS_SINK (self));
    iface = SINK_GET_INTERFACE (self);
    g_return_if_fail (iface->enqueue != NULL);

    iface->enqueue (self, obj);
}
