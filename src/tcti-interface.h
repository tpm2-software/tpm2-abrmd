#ifndef TCTI_INTERFACE_H
#define TCTI_INTERFACE_H

#include <sapi/tpm20.h>
#include <glib-object.h>

#define TCTI_BUFFER_MAX 4096

#define TYPE_TCTI                (tcti_get_type ())
#define TCTI(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TCTI, Tcti))
#define IS_TCTI(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TCTI))
#define TCTI_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TYPE_TCTI, TctiInterface))

typedef struct _Tcti          Tcti;
typedef struct _TctiInterface TctiInterface;

/* type for function pointer defined by interface */
typedef TSS2_RC (*TctiInit)       (Tcti *self);

struct _TctiInterface {
    GTypeInterface      parent;
    TSS2_TCTI_CONTEXT  *tcti_context;
    TctiInit            initialize;
};

TSS2_RC            tcti_init             (Tcti                *self);
TSS2_RC            tcti_transmit         (Tcti                *self,
                                          size_t               size,
                                          uint8_t             *command);
TSS2_RC            tcti_receive          (Tcti                *self,
                                          size_t              *size,
                                          uint8_t             *response,
                                          int32_t              timeout);
TSS2_RC            tcti_cancel           (Tcti                *self);
TSS2_RC            tcti_set_locality     (Tcti                *self,
                                          uint8_t              locality);
TSS2_TCTI_CONTEXT* tcti_peek_context     (Tcti                *self);

#endif /* TCTI_INTERFACE_H */
