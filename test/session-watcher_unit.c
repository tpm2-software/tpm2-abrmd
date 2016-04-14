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

#include "session-manager.h"
#include "session-watcher.h"

/* session_watcher_allocate_test begin
 * Test to allcoate and destroy a session_watcher_t.
 */
static void
session_watcher_allocate_test (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;
    session_watcher_t *watcher = NULL;

    watcher = session_watcher_new (manager, 0);
    assert_non_null (watcher);
    session_watcher_free (watcher);
}

static void
session_watcher_allocate_setup (void **state)
{
    session_manager_t *manager = NULL;

    manager = session_manager_new ();
    *state = manager;
}

static void
session_watcher_allocate_teardown (void **state)
{
    session_manager_t *manager = (session_manager_t*)*state;

    session_manager_free (manager);
}
/* session_watcher_allocate end */

/* session_watcher_start_test
 * This is a basic usage flow test. Can it be started, canceled and joined.
 * We're testing the underlying pthread usage ... mostly.
 */
struct watcher_test_data {
    session_manager_t *manager;
    session_watcher_t *watcher;
    gint wakeup_send_fd;
    gboolean wokeup;
};

void
session_watcher_start_test (void **state)
{
    struct watcher_test_data *data;
    gint ret;

    data = (struct watcher_test_data*)*state;
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
    struct watcher_test_data *data;
    gint fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (struct watcher_test_data));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        g_error ("failed to get pipe2s");
    data->wakeup_send_fd = fds[1];
    data->watcher = session_watcher_new (data->manager, fds[0]);
    if (data->watcher == NULL)
        g_error ("failed to allocate new session_watcher");

    *state = data;
}

static void
session_watcher_start_teardown (void **state)
{
    struct watcher_test_data *data;

    data = (struct watcher_test_data*)*state;
    close (data->watcher->wakeup_receive_fd);
    session_watcher_free (data->watcher);
    session_manager_free (data->manager);
    close (data->wakeup_send_fd);
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
int
session_watcher_wakeup_callback (session_watcher_t *watcher,
                                 gpointer user_data)
{
    gint ret;

    ret = read (watcher->wakeup_receive_fd, watcher->buf, BUF_SIZE);
    assert_true (ret != -1);
    *(gboolean*)user_data = TRUE;

    return ret;
}

static void
session_watcher_wakeup_setup (void **state)
{
    struct watcher_test_data *data;
    int fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (struct watcher_test_data));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        g_error ("failed to get pipe2s");
    data->wokeup = FALSE;
    data->watcher = session_watcher_new_full (data->manager,
                                              fds[0],
                                              NULL,
                                              session_watcher_wakeup_callback,
                                              &data->wokeup);
    data->wakeup_send_fd = fds[1];
    if (data->watcher == NULL)
        g_error ("failed to allocate new session_watcher");

    *state = data;
}

static void
session_watcher_wakeup_test (void **state)
{
    struct watcher_test_data *data;
    gint ret;

    data = (struct watcher_test_data*)*state;
    /* This string is arbitrary. */
    ret = write (data->wakeup_send_fd, "hi", strlen ("hi"));
    assert_true (ret != -1);
    ret = session_watcher_start (data->watcher);
    assert_int_equal (ret, 0);
    /* sleep here to give the watcher thread time to react to the data we've
     * sent over the wakeup_send_fd.
     */
    sleep (1);
    assert_true (data->wokeup);
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
session_watcher_session_insert_setup (void **state)
{
    struct watcher_test_data *data;
    int fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (struct watcher_test_data));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        g_error ("failed to get pipe2s");
    data->wokeup = FALSE;
    data->watcher = session_watcher_new_full (data->manager,
                                              fds[0],
                                              NULL,
                                              NULL,
                                              NULL);
    data->wakeup_send_fd = fds[1];
    if (data->watcher == NULL)
        g_error ("failed to allocate new session_watcher");

    *state = data;
}

static void
session_watcher_session_insert_test (void **state)
{
    struct watcher_test_data *data = (struct watcher_test_data*)*state;
    session_watcher_t *watcher = data->watcher;
    session_manager_t *manager = data->manager;
    session_t *session;
    gint ret, receive_fd, send_fd;
    gchar buf[256] = { 0 };

    /* */
    session = session_new (&receive_fd, &send_fd);
    assert_false (FD_ISSET (session->receive_fd, &watcher->session_fdset));
    ret = session_watcher_start(watcher);
    assert_int_equal (ret, 0);
    session_manager_insert (data->manager, session);
    ret = write (data->wakeup_send_fd, "wakeup", strlen ("wakeup"));
    assert_true (ret > 0);
    sleep (1);
    assert_true (FD_ISSET (session->receive_fd, &watcher->session_fdset));
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
                                  session_watcher_session_insert_setup,
                                  session_watcher_start_teardown),
    };
    return run_tests (tests);
}
