#ifndef TPM2_RESPONSE_H
#define TPM2_RESPONSE_H

#include <glib-object.h>
#include <tpm20.h>

#include "session-data.h"

G_BEGIN_DECLS

typedef struct _Tpm2ResponseClass {
    GObjectClass    parent;
} Tpm2ResponseClass;

typedef struct _Tpm2Response {
    GObject         parent_instance;
    SessionData    *session;
    guint8         *buffer;
    TPMA_CC         attributes;
} Tpm2Response;

#define TPM_RESPONSE_HEADER_SIZE (sizeof (TPM_ST) + sizeof (UINT32) + sizeof (TPM_RC))

#define TYPE_TPM2_RESPONSE            (tpm2_response_get_type      ())
#define TPM2_RESPONSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TPM2_RESPONSE, Tpm2Response))
#define TPM2_RESPONSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TPM2_RESPONSE, Tpm2ResponseClass))
#define IS_TPM2_RESPONSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TPM2_RESPONSE))
#define IS_TPM2_RESPONSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TPM2_RESPONSE))
#define TPM2_RESPONSE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TPM2_RESPONSE, Tpm2ResponseClass))

GType               tpm2_response_get_type      (void);
Tpm2Response*       tpm2_response_new           (SessionData     *session,
                                                 guint8          *buffer,
                                                 TPMA_CC          attributes);
Tpm2Response*       tpm2_response_new_rc        (SessionData     *session,
                                                 TPM_RC           rc);
TPMA_CC             tpm2_response_get_attributes (Tpm2Response   *response);
guint8*             tpm2_response_get_buffer    (Tpm2Response    *response);
TPM_RC              tpm2_response_get_code      (Tpm2Response    *response);
guint32             tpm2_response_get_size      (Tpm2Response    *response);
TPM_ST              tpm2_response_get_tag       (Tpm2Response    *response);
SessionData*        tpm2_response_get_session   (Tpm2Response    *response);

G_END_DECLS

#endif /* TPM2_RESPONSE_H */
