#ifndef TCTI_OPTIONS_H
#define TCTI_OPTIONS_H

#include <glib-object.h>
#include "tcti-type-enum.h"
#include "tcti-interface.h"

G_BEGIN_DECLS

typedef struct _TctiOptionsClass {
    GObjectClass     parent;
} TctiOptionsClass;

typedef struct _TctiOptions
{
    GObject          parent_instance;
    TctiTypeEnum     tcti_type;
    gchar           *device_name;
    gchar           *socket_address;
    gint             socket_port;
    guint            echo_size;
} TctiOptions;

#define TYPE_TCTI_OPTIONS             (tcti_options_get_type       ())
#define TCTI_OPTIONS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_OPTIONS, TctiOptions))
#define TCTI_OPTIONS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_OPTIONS, TctiOptionsClass))
#define IS_TCTI_OPTIONS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_OPTIONS))
#define IS_TCTI_OPTIONS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_OPTIONS))
#define TCTI_OPTIONS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_OPTIONS, TctiOptionsClass))

GType         tcti_options_get_type  (void);
TctiOptions*  tcti_options_new       (void);
GOptionGroup* tcti_options_get_group (TctiOptions *options);
Tcti*         tcti_options_get_tcti  (TctiOptions *options);

G_END_DECLS
#endif /* TCTI_OPTIONS_H */
