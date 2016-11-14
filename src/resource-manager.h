#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sapi/tpm20.h>

#include "access-broker.h"
#include "message-queue.h"
#include "session-data.h"
#include "sink-interface.h"
#include "tab.h"

G_BEGIN_DECLS

typedef struct _ResourceManagerClass {
    GObjectClass      parent;
} ResourceManagerClass;

typedef struct _ResourceManager {
    GObject           parent_instance;
    AccessBroker     *access_broker;
    MessageQueue     *in_queue;
    pthread_t         thread;
    Sink             *sink;
} ResourceManager;

#define TYPE_RESOURCE_MANAGER              (resource_manager_get_type ())
#define RESOURCE_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_RESOURCE_MANAGER, ResourceManager))
#define RESOURCE_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_RESOURCE_MANAGER, ResourceManagerClass))
#define IS_RESOURCE_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_RESOURCE_MANAGER))
#define IS_RESOURCE_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_RESOURCE_MANAGER))
#define RESOURCE_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_RESOURCE_MANAGER, ResourceManagerClass))

GType                 resource_manager_get_type       (void);
ResourceManager*      resource_manager_new            (AccessBroker *broker);

G_END_DECLS
#endif /* RESOURCE_MANAGER_H */
