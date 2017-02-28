#ifndef TABRMD_PRIV_H
#define TABRMD_PRIV_H

#include "tcti-options.h"

#define MAX_TRANSIENT_OBJECTS 100
#define MAX_TRANSIENT_OBJECTS_DEFAULT 27
#define TABD_INIT_THREAD_NAME "tss2-tabrmd_init-thread"
#define TABD_RAND_FILE "/dev/random"

typedef struct tabrmd_options {
    GBusType        bus;
    TctiOptions    *tcti_options;
    gboolean        fail_on_loaded_trans;
    gint            max_connections;
    gint            max_transient_objects;
} tabrmd_options_t;

#endif /* TABRMD_PRIV_H */
