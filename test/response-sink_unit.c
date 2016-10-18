#include <glib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "session-manager.h"
#include "tab.h"
#include "response-sink.h"
#include "tcti-echo.h"

/**
 * Test to allcoate and destroy a ResponseSink.
 */
static void
response_sink_allocate_test (void **state)
{
    Tcti *tcti;
    Tab  *tab;
    ResponseSink *sink;

    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    sink = response_sink_new ();
    tab = tab_new (tcti, SINK (sink));

    g_object_unref (sink);
    g_object_unref (tab);
}

/* response_sink_start_stop_test begin
 */
typedef struct start_stop_data {
    ResponseSink  *sink;
    Tab              *tab;
    Tcti             *tcti;
} start_stop_data_t;

void *
not_response_sink_thread (void *data)
{
    while (TRUE) {
        sleep (1);
        pthread_testcancel ();
    }
}

gint
__wrap_response_sink_start (ResponseSink *sink)
{
    if (sink->thread != 0)
        g_error ("response_sink already started");
    return pthread_create (&sink->thread, NULL, not_response_sink_thread, sink);
}

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

    data->tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    data->sink = response_sink_new ();
    data->tab  = tab_new (data->tcti, SINK (data->sink));

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
