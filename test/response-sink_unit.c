/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"
#include "response-sink.h"

/**
 * Test to allocate and destroy a ResponseSink.
 */
static void
response_sink_allocate_test (void **state)
{
    ResponseSink *sink;
    UNUSED_PARAM(state);

    sink = response_sink_new ();

    g_object_unref (sink);
}

int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (response_sink_allocate_test),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
