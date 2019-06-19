/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef CONTEXT_UTIL_H
#define CONTEXT_UTIL_H

#include <tss2/tss2_tcti.h>
#include <tss2/tss2_sys.h>

#include "test-options.h"

/**
 * functions to setup TCTIs and SAPI contexts  using data from the common
 * options
 */
TSS2_TCTI_CONTEXT*    tcti_init_from_opts (test_opts_t        *options);
TSS2_SYS_CONTEXT*     sapi_init_from_opts (test_opts_t        *options);
void                  sapi_teardown_full  (TSS2_SYS_CONTEXT   *sapi_context);

#endif /* CONTEXT_UTIL_H */
