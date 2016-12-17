#ifndef ACCESS_BROKER_H
#define ACCESS_BROKER_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sapi/tpm20.h>

#include "tcti.h"
#include "tpm2-command.h"
#include "tpm2-response.h"

G_BEGIN_DECLS

typedef struct _AccessBrokerClass {
    GObjectClass      parent;
} AccessBrokerClass;

typedef struct _AccessBroker {
    GObject                 parent_instance;
    pthread_mutex_t         sapi_mutex;
    TSS2_SYS_CONTEXT       *sapi_context;
    Tcti                   *tcti;
    TPMS_CAPABILITY_DATA    properties_fixed;
    gboolean                initialized;
} AccessBroker;

#define TYPE_ACCESS_BROKER              (access_broker_get_type ())
#define ACCESS_BROKER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_ACCESS_BROKER, AccessBroker))
#define ACCESS_BROKER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_ACCESS_BROKER, AccessBrokerClass))
#define IS_ACCESS_BROKER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_ACCESS_BROKER))
#define IS_ACCESS_BROKER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_ACCESS_BROKER))
#define ACCESS_BROKER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_ACCESS_BROKER, AccessBrokerClass))

GType              access_broker_get_type       (void);
AccessBroker*      access_broker_new            (Tcti            *tcti);
TSS2_RC            access_broker_init           (AccessBroker    *broker);
gint               access_broker_lock           (AccessBroker    *broker);
gint               access_broker_unlock         (AccessBroker    *broker);
Tpm2Response*      access_broker_send_command   (AccessBroker    *broker,
                                                 Tpm2Command     *command,
                                                 TSS2_RC         *rc);
TSS2_RC            access_broker_get_max_command    (AccessBroker   *broker,
                                                     guint32        *value);

G_END_DECLS

#endif
