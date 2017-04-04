#ifndef TABD_TCTI_TYPE_ENUM_H
#define TABD_TCTI_TYPE_ENUM_H

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * Set the default TCTI. In order of preference: libtcti-device (if enabled),
 * otherwise fall back to libtcti-socket. If neither are available throw an
 * error. This should be a configure time error though.
 */
#ifdef HAVE_TCTI_DEVICE
#define TCTI_TYPE_DEFAULT TCTI_TYPE_DEVICE
#elif HAVE_TCTI_SOCKET
#define TCTI_TYPE_DEFAULT TCTI_TYPE_SOCKET
#else
#error at least one TCTI must be enabled
#endif

typedef enum TCTI_TYPE {
    TCTI_TYPE_NONE,
#ifdef HAVE_TCTI_DEVICE
    TCTI_TYPE_DEVICE,
#endif
#ifdef HAVE_TCTI_SOCKET
    TCTI_TYPE_SOCKET,
#endif
} TctiTypeEnum;

#define TYPE_TCTI_TYPE_ENUM      (tcti_type_enum_get_type ())
GType   tcti_type_enum_get_type  (void);

G_END_DECLS
#endif /* TABD_TCTI_TYPE_ENUM_H */
