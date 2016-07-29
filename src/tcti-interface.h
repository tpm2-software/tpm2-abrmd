#ifndef TCTI_INTERFACE_H
#define TCTI_INTERFACE_H

#include <sapi/tpm20.h>
#include <glib-object.h>

#define TYPE_TCTI                (tcti_get_type ())
#define TCTI(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TCTI, Tcti))
#define IS_TCTI(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TCTI))
#define TCTI_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TYPE_TCTI, TctiInterface))

typedef struct _Tcti          Tcti;
typedef struct _TctiInterface TctiInterface;

/* type for function pointer defined by interface */
typedef TSS2_TCTI_CONTEXT* (*TctiGetContext) (Tcti *self);

struct _TctiInterface {
    GTypeInterface    parent;
    TctiGetContext    get_context;
};

GType              tcti_get_type    (void);
TSS2_TCTI_CONTEXT* tcti_get_context (Tcti *self);

#endif /* TCTI_INTERFACE_H */
