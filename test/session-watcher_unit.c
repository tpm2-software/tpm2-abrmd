#include <glib.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tab.h"
#include "session-manager.h"
#include "session-watcher.h"

typedef struct watcher_test_data {
    session_manager_t *manager;
    session_watcher_t *watcher;
    session_data_t *session;
    tab_t *tab;
    TSS2_TCTI_CONTEXT *tcti;
    gint wakeup_send_fd;
    gboolean match;
} watcher_test_data_t;

/* session_watcher_allocate_test begin
 * Test to allcoate and destroy a session_watcher_t.
 */
static void
session_watcher_allocate_test (void **state)
{
    watcher_test_data_t *data = (watcher_test_data_t *)*state;
    session_watcher_t *watcher = NULL;

    watcher = session_watcher_new (data->manager, 0, data->tab);
    assert_non_null (watcher);
    session_watcher_free (watcher);
}

static void
session_watcher_allocate_setup (void **state)
{
    watcher_test_data_t *data;

    data = calloc (1, sizeof (watcher_test_data_t));
    data->manager = session_manager_new ();
    data->tcti = NULL;
    data->tab = tab_new (data->tcti);

    *state = data;
}

static void
session_watcher_allocate_teardown (void **state)
{
    watcher_test_data_t *data = (watcher_test_data_t*)*state;

    session_manager_free (data->manager);
    tab_free (data->tab);
    free (data);
}
/* session_watcher_allocate end */

/* session_watcher_start_test
 * This is a basic usage flow test. Can it be started, canceled and joined.
 * We're testing the underlying pthread usage ... mostly.
 */
void
session_watcher_start_test (void **state)
{
    watcher_test_data_t *data;
    gint ret;

    data = (watcher_test_data_t*)*state;
    session_watcher_start(data->watcher);
    sleep (1);
    assert_true (data->watcher->running);
    ret = session_watcher_cancel (data->watcher);
    assert_int_equal (ret, 0);
    ret = session_watcher_join (data->watcher);
    assert_int_equal (ret, 0);
}

static void
session_watcher_start_setup (void **state)
{
    watcher_test_data_t *data;
    gint ret, fds[2] = { 0 };

    ret = pipe2 (fds, O_CLOEXEC);
    if (ret == -1)
        g_error ("pipe2 failed w/ errno: %d %s", errno, strerror (errno));
    data = calloc (1, sizeof (watcher_test_data_t));
    data->wakeup_send_fd = fds[1];
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    data->tcti = NULL;
    data->tab = tab_new (data->tcti);
    data->watcher = session_watcher_new (data->manager, fds[0], data->tab);
    if (data->watcher == NULL)
        g_error ("failed to allocate new session_watcher");

    *state = data;
}

static void
session_watcher_start_teardown (void **state)
{
    watcher_test_data_t *data = (watcher_test_data_t*)*state;

    close (data->wakeup_send_fd);
    session_watcher_free (data->watcher);
    session_manager_free (data->manager);
    tab_free (data->tab);
    free (data);
}
/* session_watcher_start_test end */

/* session_watcher_wakeup_test
 * Here we test the session_watcher wakeup mechanism. We do all of the
 * necessary setup to get the session watcher thread running and then write
 * a bit of data to it. This causes it to run the wakeup callback that we
 * provided. We get an indication that this callback was run through the
 * user_data provided, namely the wokeup gboolean from the watcher_test_data
 * structure.
*/
static void
session_watcher_wakeup_setup (void **state)
{
    watcher_test_data_t *data;
    int fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (watcher_test_data_t));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        g_error ("failed to get pipe2s");
    data->tab = tab_new (NULL);
    data->watcher = session_watcher_new (data->manager,
                                        fds[0],
                                        data->tab);
    data->wakeup_send_fd = fds[1];
    if (data->watcher == NULL)
        g_error ("failed to allocate new session_watcher");

    *state = data;
}
static void
session_watcher_wakeup_test (void **state)
{
    watcher_test_data_t *data;
    gint ret;

    data = (watcher_test_data_t*)*state;
    /* This string is arbitrary. */
    ret = session_watcher_start (data->watcher);
    assert_int_equal (ret, 0);
    ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    sleep (1);
    assert_true (ret != -1);
   /* sleep here to give the watcher thread time to react to the data we've
     * sent over the wakeup_send_fd.
     */
    ret = session_watcher_cancel (data->watcher);
    assert_int_equal (ret, 0);
    ret = session_watcher_join (data->watcher);
    assert_int_equal (ret, 0);
}
/* session_watcher_wakeup_test end */
/* session_watcher_session_insert_test begin
 * In this test we create a session watcher and all that that entails. We then
 * create a new session and insert it into the session manager. We then signal
 * to the watcher that there's a new session in the manager by sending data to
 * it over the send end of the wakeup pipe "wakeup_send_fd". We then check the
 * session_fdset in the watcher structure to be sure the receive end of the
 * session pipe is set. This is how we know that the watcher is now watching
 * for data from the new session.
 */
static void
session_watcher_session_insert_test (void **state)
{
    struct watcher_test_data *data = (struct watcher_test_data*)*state;
    session_watcher_t *watcher = data->watcher;
    session_manager_t *manager = data->manager;
    session_data_t *session;
    gint ret, receive_fd, send_fd;

    /* */
    session = session_data_new (&receive_fd, &send_fd, 5);
    assert_false (FD_ISSET (session->receive_fd, &watcher->session_fdset));
    ret = session_watcher_start(watcher);
    assert_int_equal (ret, 0);
    session_manager_insert (data->manager, session);
    ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    assert_true (ret > 0);
    sleep (1);
    assert_true (FD_ISSET (session->receive_fd, &watcher->session_fdset));
    session_manager_remove (data->manager, session);
    session_watcher_cancel (watcher);
    session_watcher_join (watcher);
}
/* session_watcher_sesion_insert_test end */
int
main (int argc,
      char* argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (session_watcher_allocate_test,
                                  session_watcher_allocate_setup,
                                  session_watcher_allocate_teardown),
        unit_test_setup_teardown (session_watcher_start_test,
                                  session_watcher_start_setup,
                                  session_watcher_start_teardown),
        unit_test_setup_teardown (session_watcher_wakeup_test,
                                  session_watcher_wakeup_setup,
                                  session_watcher_start_teardown),
        unit_test_setup_teardown (session_watcher_session_insert_test,
                                  session_watcher_wakeup_setup,
                                  session_watcher_start_teardown),
    };
    return run_tests (tests);
}
