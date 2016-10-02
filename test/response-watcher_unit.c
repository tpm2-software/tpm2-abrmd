#include <glib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "session-manager.h"
#include "tab.h"
#include "response-watcher.h"
#include "tcti-echo.h"

/**
 * Test to allcoate and destroy a ResponseWatcher.
 */
static void
response_watcher_allocate_test (void **state)
{
    Tcti *tcti;
    Tab  *tab;
    ResponseWatcher *watcher;

    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    tab = tab_new (tcti);
    watcher = response_watcher_new (tab);

    g_object_unref (watcher);
}

/* response_watcher_start_stop_test begin
 */
typedef struct start_stop_data {
    ResponseWatcher  *watcher;
    Tab              *tab;
    Tcti             *tcti;
} start_stop_data_t;

void *
not_response_watcher_thread (void *data)
{
    while (TRUE) {
        sleep (1);
        pthread_testcancel ();
    }
}

gint
__wrap_response_watcher_start (ResponseWatcher *watcher)
{
    if (watcher->thread != 0)
        g_error ("response_watcher already started");
    return pthread_create (&watcher->thread, NULL, not_response_watcher_thread, watcher);
}

static void
response_watcher_start_stop_test (void **state)
{
    start_stop_data_t *data = (start_stop_data_t*)*state;
    gint ret = 0;

    ret = response_watcher_start (data->watcher);
    assert_int_equal (ret, 0);
    ret = response_watcher_cancel (data->watcher);
    assert_int_equal (ret, 0);
    ret = response_watcher_join (data->watcher);
    assert_int_equal (ret, 0);
}
static void
response_watcher_start_stop_setup (void **state)
{
    start_stop_data_t *data = calloc (1, sizeof (start_stop_data_t));

    data->tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    data->tab  = tab_new (TCTI (data->tcti));
    data->watcher = response_watcher_new (data->tab);

    *state = data;
}
static void
response_watcher_start_stop_teardown (void **state)
{
    start_stop_data_t *data = (start_stop_data_t*)*state;

    g_object_unref (data->watcher);
    g_free (data);
}
/* response_watcher_start_stop end */

int
main (int argc,
      char* argv[])
{
    const UnitTest tests[] = {
        unit_test (response_watcher_allocate_test),
        unit_test_setup_teardown (response_watcher_start_stop_test,
                                  response_watcher_start_stop_setup,
                                  response_watcher_start_stop_teardown),
    };
    return run_tests (tests);
}
