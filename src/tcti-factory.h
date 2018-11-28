/* SPDX-License-Identifier: BSD-2 */
#ifndef TCTI_FACTORY_H
#define TCTI_FACTORY_H

#include <glib-object.h>
#include <tss2/tss2_tcti.h>

#include "tcti.h"

G_BEGIN_DECLS

typedef struct _TctiFactoryClass {
   GObjectClass           parent;
} TctiFactoryClass;

typedef struct _TctiFactory
{
    GObject               parent;
    gchar                *name;
    gchar                *conf;
} TctiFactory;

#define TYPE_TCTI_FACTORY             (tcti_factory_get_type       ())
#define TCTI_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_FACTORY, TctiFactory))
#define TCTI_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_FACTORY, TctiFactoryClass))
#define IS_TCTI_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_FACTORY))
#define IS_TCTI_FACTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_FACTORY))
#define TCTI_FACTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_FACTORY, TctiFactoryClass))

GType                tcti_factory_get_type       (void);
TctiFactory*         tcti_factory_new            (gchar const      *name,
                                                  gchar const      *conf);
Tcti*                tcti_factory_create         (TctiFactory      *self);
G_END_DECLS
#endif /* TCTI_FACTORY_H */
