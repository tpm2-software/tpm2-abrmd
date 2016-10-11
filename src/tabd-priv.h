#ifndef TABD_PRIV_H
#define TABD_PRIV_H

#include "tcti-options.h"

#define TABD_INIT_THREAD_NAME "tss2-tabd_init-thread"
#define TABD_RAND_FILE "/dev/random"

typedef struct tabd_options {
    GBusType        bus;
    TctiOptions    *tcti_options;
} tabd_options_t;

#endif /* TABD_PRIV_H */
