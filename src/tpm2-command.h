#ifndef TPM2_COMMAND_H
#define TPM2_COMMAND_H

#include <glib-object.h>
#include <tpm20.h>

#include "session-data.h"

G_BEGIN_DECLS

typedef struct _Tpm2CommandClass {
    GObjectClass    parent;
} Tpm2CommandClass;

typedef struct _Tpm2Command {
    GObject         parent_instance;
    TPMA_CC         attributes;
    SessionData    *session;
    guint8         *buffer;
} Tpm2Command;

#include "command-attrs.h"

#define TYPE_TPM2_COMMAND            (tpm2_command_get_type      ())
#define TPM2_COMMAND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TPM2_COMMAND, Tpm2Command))
#define TPM2_COMMAND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TPM2_COMMAND, Tpm2CommandClass))
#define IS_TPM2_COMMAND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TPM2_COMMAND))
#define IS_TPM2_COMMAND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TPM2_COMMAND))
#define TPM2_COMMAND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TPM2_COMMAND, Tpm2CommandClass))

GType                 tpm2_command_get_type        (void);
Tpm2Command*          tpm2_command_new             (SessionData      *session,
                                                    guint8           *buffer,
                                                    TPMA_CC           attrs);
Tpm2Command*          tpm2_command_new_from_fd     (SessionData      *session,
                                                    gint              fd,
                                                    CommandAttrs     *attrs);
TPMA_CC               tpm2_command_get_attributes  (Tpm2Command      *command);
guint8*               tpm2_command_get_buffer      (Tpm2Command      *command);
TPM_CC                tpm2_command_get_code        (Tpm2Command      *command);
guint32               tpm2_command_get_size        (Tpm2Command      *command);
TPMI_ST_COMMAND_TAG   tpm2_command_get_tag         (Tpm2Command      *command);
SessionData*          tpm2_command_get_session     (Tpm2Command      *command);

G_END_DECLS

#endif /* TPM2_COMMAND_H */
