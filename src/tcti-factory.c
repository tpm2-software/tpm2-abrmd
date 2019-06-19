/* SPDX-License-Identifier: BSD-2-Clause */
#include <dlfcn.h>
#include <inttypes.h>

#include <tss2/tss2_tctildr.h>

#include "util.h"
#include "tabrmd.h"
#include "tcti-factory.h"

/*
 * This class implements a simple factory object used to instantiate Tcti
 * objects. When created this class is passed the tcti name and the
 * configuration string. These values are then used to instantiate a Tcti
 * object for the appropriate TSS2_TCTI_CONTEXT.
 */
G_DEFINE_TYPE (TctiFactory, tcti_factory, G_TYPE_OBJECT);

enum {
    PROP_0,
    PROP_CONF,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL };

static void
tcti_factory_set_property (GObject *object,
                           guint property_id,
                           GValue const *value,
                           GParamSpec *pspec)
{
    TctiFactory *self = TCTI_FACTORY (object);

    switch (property_id) {
    case PROP_CONF:
        self->conf = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
tcti_factory_get_property (GObject *object,
                           guint property_id,
                           GValue *value,
                           GParamSpec *pspec)
{
    TctiFactory *self = TCTI_FACTORY (object);

    switch (property_id) {
    case PROP_CONF:
        g_value_set_string (value, self->conf);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tcti_factory_finalize (GObject *obj)
{
    TctiFactory *self = TCTI_FACTORY (obj);

    g_clear_pointer (&self->conf, g_free);
    G_OBJECT_CLASS (tcti_factory_parent_class)->finalize (obj);
}
static void
tcti_factory_init (TctiFactory *tcti)
{
    UNUSED_PARAM(tcti);
}
/*
 * When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_factory_class_init (TctiFactoryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    if (tcti_factory_parent_class == NULL) {
        tcti_factory_parent_class = g_type_class_peek_parent (klass);
    }
    object_class->finalize = tcti_factory_finalize;
    object_class->get_property = tcti_factory_get_property;
    object_class->set_property = tcti_factory_set_property;

    obj_properties [PROP_CONF] =
        g_param_spec_string ("conf",
                             "TCTI config string",
                             "TCTI initialization configuration string.",
                             "empty string",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * Instantiate a new TctiFactory object. The 'name' and 'conf' values
 * provided determine which TCTI library is used and it's configuration
 * when the '_create' function is called.
 */
TctiFactory*
tcti_factory_new (const gchar *conf)
{
    return TCTI_FACTORY (g_object_new (TYPE_TCTI_FACTORY,
                                       "conf", conf,
                                       NULL));
}

/*
 * Initialize an instance of a TSS2_TCTI_CONTEXT and create a Tcti object
 * for it.
 */
Tcti*
tcti_factory_create (TctiFactory *self)
{
    TSS2_RC rc = TSS2_RC_SUCCESS;
    TSS2_TCTI_CONTEXT *ctx = NULL;

    g_debug ("%s: TctiFactory with TCTI conf '%s'", __func__, self->conf);
    rc = Tss2_TctiLdr_Initialize (self->conf, &ctx);
    if (rc != TSS2_RC_SUCCESS || ctx == NULL) {
        g_info ("%s: failed to initialize TCTI, RC: 0x%" PRIx32,
                 __func__, rc);
        return NULL;
    }
    return tcti_new (ctx);
}
