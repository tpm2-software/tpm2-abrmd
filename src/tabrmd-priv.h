#ifndef TABRMD_PRIV_H
#define TABRMD_PRIV_H

#include "tcti-options.h"

#define TABD_INIT_THREAD_NAME "tss2-tabrmd_init-thread"
#define TABD_RAND_FILE "/dev/random"

typedef struct tabrmd_options {
    GBusType        bus;
    TctiOptions    *tcti_options;
} tabrmd_options_t;

#endif /* TABRMD_PRIV_H */
