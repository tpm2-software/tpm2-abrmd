#ifndef THREAD_INTERFACE_H
#define THREAD_INTERFACE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define TYPE_THREAD                (thread_get_type ())
#define THREAD(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_THREAD, Thread))
#define IS_THREAD(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_THREAD))
#define THREAD_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TYPE_THREAD, ThreadInterface))

typedef struct _Thread          Thread;
typedef struct _ThreadInterface ThreadInterface;

/* type for function pointer defined by interface */
typedef gint     (*ThreadCancel)       (Thread        *self);
typedef gint     (*ThreadJoin)         (Thread        *self);
typedef gint     (*ThreadStart)        (Thread        *self);

struct _ThreadInterface {
    GTypeInterface      parent;
    ThreadCancel        cancel;
    ThreadJoin          join;
    ThreadStart         start;
};

/**
 * We prefix these functions with tab_ to prevent collisions with the thread
 * library functions.
 */
GType           thread_get_type     (void);
gint            thread_cancel       (Thread            *self);
gint            thread_join         (Thread            *self);
gint            thread_start        (Thread            *self);

G_END_DECLS
#endif /* THREAD_INTERFACE_H */
