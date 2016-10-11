#ifndef TABD_TCTI_DEVICE_H
#define TABD_TCTI_DEVICE_H

#include <sapi/tpm20.h>
#include <tcti/tcti_device.h>
#include <glib-object.h>

#include "tcti-interface.h"

G_BEGIN_DECLS

typedef struct _TctiDeviceClass {
    GObjectClass       parent;
} TctiDeviceClass;

typedef struct _TctiDevice
{
    GObject            parent_instance;
    gchar             *filename;
} TctiDevice;

#define TYPE_TCTI_DEVICE             (tcti_device_get_type       ())
#define TCTI_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_DEVICE, TctiDevice))
#define TCTI_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_DEVICE, TctiDeviceClass))
#define IS_TCTI_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_DEVICE))
#define IS_TCTI_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_DEVICE))
#define TCTI_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_DEVICE, TctiDeviceClass))

GType                tcti_device_get_type       (void);
TctiDevice*          tcti_device_new            (gchar const      *filename);
TSS2_RC              tcti_device_initialize     (Tcti             *tcti);

G_END_DECLS
#endif /* TABD_TCTI_DEVICE_H */
