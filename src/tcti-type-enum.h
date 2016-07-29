#ifndef TABD_TCTI_TYPE_ENUM_H
#define TABD_TCTI_TYPE_ENUM_H

#include <glib-object.h>

typedef enum TCTI_TYPE {
    TCTI_TYPE_ECHO,
} TctiTypeEnum;

#define TYPE_TCTI_TYPE_ENUM      (tcti_type_enum_get_type ())
GType   tcti_type_enum_get_type  (void);

#endif
