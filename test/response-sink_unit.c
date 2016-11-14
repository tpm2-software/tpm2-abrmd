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

    ret = response_sink_start (data->sink);
    assert_int_equal (ret, 0);
    ret = response_sink_cancel (data->sink);
    assert_int_equal (ret, 0);
    ret = response_sink_join (data->sink);
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
