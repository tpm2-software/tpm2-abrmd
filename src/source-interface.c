#include "source-interface.h"

static void
source_default_init (SourceInterface *iface)
{
    /* noop, requied by G_DEFINE_INTERFACE */
}

GType
source_get_type (void)
{
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                              "source",
                                              sizeof (SourceInterface),
                                              (GClassInitFunc)source_default_init,
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
source_add_sink (Source       *self,
                 Sink         *sink)
{
    SourceInterface *iface;

    g_debug ("source_add_sink");
    g_return_val_if_fail (IS_SOURCE (self), NULL);
    iface = SOURCE_GET_INTERFACE (self);
    g_return_val_if_fail (iface->add_sink != NULL, NULL);

    iface->add_sink (self, sink);
}
