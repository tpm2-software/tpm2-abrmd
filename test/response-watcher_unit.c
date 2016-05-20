#include <glib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "session-manager.h"
#include "tab.h"
#include "response-watcher.h"

/* response_watcher_allocate_test begin
 * Test to allcoate and destroy a response_watcher_t.
 */
static void
response_watcher_allocate_test (void **state)
{
    tab_t *tab = (tab_t*)*state;
    response_watcher_t *watcher = NULL;

    watcher = response_watcher_new (tab);
    assert_non_null (watcher);
    assert_int_equal (tab, watcher->tab);
    response_watcher_free (watcher);
}

static void
response_watcher_allocate_setup (void **state)
{
    tab_t *tab = NULL;

    /* this is a hack, we need a mock TCTI */
    tab = tab_new (NULL);
    *state = tab;
}

static void
response_watcher_allocate_teardown (void **state)
{
    tab_t *tab = (tab_t*)*state;

    tab_free (tab);
}
/* response_watcher_allocate end */

/* response_watcher_start_stop_test begin
 */
typedef struct start_stop_data {
    response_watcher_t  *watcher;
    tab_t               *tab;
} start_stop_data_t;

void *
not_response_watcher_thread (void *data)
{
    while (TRUE) {
        sleep (1);
    }
}

gint
__wrap_response_watcher_start (response_watcher_t *watcher)
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
    data->tab = tab_new (NULL);
    data->watcher = response_watcher_new (data->tab);
    *state = data;
}
static void
response_watcher_start_stop_teardown (void **state)
{
    start_stop_data_t *data = (start_stop_data_t*)*state;
    response_watcher_free (data->watcher);
    tab_free (data->tab);
    free (data);
}

/* response_watcher_start_stop end */

int
main (int argc,
      char* argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (response_watcher_allocate_test,
                                  response_watcher_allocate_setup,
                                  response_watcher_allocate_teardown),
        unit_test_setup_teardown (response_watcher_start_stop_test,
                                  response_watcher_start_stop_setup,
                                  response_watcher_start_stop_teardown),
    };
    return run_tests (tests);
}
