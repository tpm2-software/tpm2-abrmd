/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "response-sink.h"

/**
 * Test to allcoate and destroy a ResponseSink.
 */
static void
response_sink_allocate_test (void **state)
{
    ResponseSink *sink;

    sink = response_sink_new ();

    g_object_unref (sink);
}

/* response_sink_start_stop_test begin
 */
typedef struct start_stop_data {
    ResponseSink  *sink;
} start_stop_data_t;

static void
response_sink_start_stop_test (void **state)
{
    start_stop_data_t *data = (start_stop_data_t*)*state;
    gint ret = 0;

    ret = thread_start (THREAD (data->sink));
    assert_int_equal (ret, 0);
    ret = thread_cancel (THREAD (data->sink));
    assert_int_equal (ret, 0);
    ret = thread_join (THREAD (data->sink));
    assert_int_equal (ret, 0);
}
static void
response_sink_start_stop_setup (void **state)
{
    start_stop_data_t *data = calloc (1, sizeof (start_stop_data_t));

    data->sink = response_sink_new ();

    *state = data;
}
static void
response_sink_start_stop_teardown (void **state)
{
    start_stop_data_t *data = (start_stop_data_t*)*state;

    g_object_unref (data->sink);
    g_free (data);
}
/* response_sink_start_stop end */

int
main (int argc,
      char* argv[])
{
    const UnitTest tests[] = {
        unit_test (response_sink_allocate_test),
        unit_test_setup_teardown (response_sink_start_stop_test,
                                  response_sink_start_stop_setup,
                                  response_sink_start_stop_teardown),
    };
    return run_tests (tests);
}
