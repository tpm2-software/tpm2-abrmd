/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TEST_OPTIONS_H
#define TEST_OPTIONS_H

#include <gio/gio.h>
#include "tss2-tcti-tabrmd.h"

/* Default number of attempts to init selected TCTI */
#define TCTI_RETRIES_DEFAULT 5

/* environment variables holding TCTI config */
#define ENV_TCTI      "TABRMD_TEST_TCTI"
#define ENV_TCTI_CONF "TABRMD_TEST_TCTI_CONF"
#define ENV_TCTI_RETRIES    "TABRMD_TEST_TCTI_RETRIES"

#define TEST_OPTS_DEFAULT_INIT { \
    .tcti_filename = NULL, \
    .tcti_conf = NULL, \
    .tcti_retries = TCTI_RETRIES_DEFAULT, \
}

typedef struct {
    const char *tcti_filename;
    const char *tcti_conf;
    uintmax_t   tcti_retries;
} test_opts_t;

/* functions to get test options from the user and to print helpful stuff */

const char  *bus_str_from_type          (GBusType              bus_type);
GBusType     bus_type_from_str          (const char*           bus_type_str);
int          get_test_opts_from_env     (test_opts_t          *opts);
int          sanity_check_test_opts     (test_opts_t          *opts);
void         dump_test_opts             (test_opts_t          *opts);

#endif /* TEST_OPTIONS_H */
