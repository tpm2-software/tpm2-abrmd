#ifndef TABD_TCTI_ECHO_H
#define TABD_TCTI_ECHO_H

#include <sapi/tpm20.h>
#include <glib-object.h>

#include "tcti-echo.h"

G_BEGIN_DECLS

typedef struct _TctiEchoClass {
    GObjectClass parent;
} TctiEchoClass;

typedef struct _TctiEcho
{
    GObject            parent_instance;
    guint              size;
    TSS2_TCTI_CONTEXT *tcti_context;
} TctiEcho;

#define TYPE_TCTI_ECHO             (tcti_echo_get_type         ())
#define TCTI_ECHO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_ECHO, TctiEcho))
#define TCTI_ECHO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_ECHO, TctiEchoClass))
#define IS_TCTI_ECHO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_ECHO))
#define IS_TCTI_ECHO_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_ECHO))
#define TCTI_ECHO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_ECHO, TctiEchoClass))

GType                tcti_echo_get_type       (void);
TctiEcho*            tcti_echo_new            (guint        size);
TSS2_TCTI_CONTEXT*   tcti_echo_get_context    (TctiEcho    *tcti);

G_END_DECLS

#endif /* TABD_TCTI_ECHO_H */
