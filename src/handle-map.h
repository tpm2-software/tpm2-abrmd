#ifndef HANDLE_MAP_H
#define HANDLE_MAP_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sapi/tpm20.h>

G_BEGIN_DECLS

typedef struct _HandleMapClass {
    GObjectClass      parent;
} HandleMapClass;

typedef struct _HandleMap {
    GObject             parent_instance;
    pthread_mutex_t     mutex;
    GHashTable         *handle_to_object_hash_table;
} HandleMap;

#define TYPE_HANDLE_MAP              (handle_map_get_type   ())
#define HANDLE_MAP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_HANDLE_MAP, HandleMap))
#define HANDLE_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_HANDLE_MAP, HandleMapClass))
#define IS_HANDLE_MAP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_HANDLE_MAP))
#define IS_HANDLE_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_HANDLE_MAP))
#define HANDLE_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_HANDLE_MAP, HandleMapClass))

GType            handle_map_get_type    (void);
HandleMap*       handle_map_new         (void);
void             handle_map_insert      (HandleMap     *map,
                                         TPM_HANDLE     handle,
                                         GObject       *object);
gint             handle_map_remove      (HandleMap     *map,
                                         TPM_HANDLE     handle);
GObject*         handle_map_lookup      (HandleMap     *map,
                                         TPM_HANDLE     handle);
guint            handle_map_size        (HandleMap     *map);

G_END_DECLS
#endif /* HANDLE_MAP_H */
