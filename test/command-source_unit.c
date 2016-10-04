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
#include "command-source.h"
#include "tcti-echo.h"

typedef struct source_test_data {
    SessionManager *manager;
    CommandSource *source;
    SessionData *session;
    Tab  *tab;
    Tcti *tcti;
    gint wakeup_send_fd;
    gboolean match;
} source_test_data_t;

/* command_source_allocate_test begin
 * Test to allcoate and destroy a CommandSource.
 */
static void
command_source_allocate_test (void **state)
{
    source_test_data_t *data = (source_test_data_t *)*state;
    CommandSource *source = NULL;

    source = command_source_new (data->manager, 0, data->tab);
    assert_non_null (source);
    g_object_unref (source);
}

static void
command_source_allocate_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = session_manager_new ();
    data->tcti = (Tcti*)tcti_echo_new (TCTI_ECHO_MIN_BUF);
    data->tab = tab_new (data->tcti);

    *state = data;
}

static void
command_source_allocate_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    g_object_unref (data->manager);
    g_object_unref (data->tab);
    free (data);
}
/* command_source_allocate end */

/* command_source_start_test
 * This is a basic usage flow test. Can it be started, canceled and joined.
 * We're testing the underlying pthread usage ... mostly.
 */
void
command_source_start_test (void **state)
{
    source_test_data_t *data;
    gint ret;

    data = (source_test_data_t*)*state;
    command_source_start(data->source);
    sleep (1);
    assert_true (data->source->running);
    ret = command_source_cancel (data->source);
    assert_int_equal (ret, 0);
    ret = command_source_join (data->source);
    assert_int_equal (ret, 0);
}

static void
command_source_start_setup (void **state)
{
    source_test_data_t *data;
    gint ret, fds[2] = { 0 };

    ret = pipe2 (fds, O_CLOEXEC);
    if (ret == -1)
        g_error ("pipe2 failed w/ errno: %d %s", errno, strerror (errno));
    data = calloc (1, sizeof (source_test_data_t));
    data->wakeup_send_fd = fds[1];
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    data->tcti = (Tcti*)tcti_echo_new (TCTI_ECHO_MIN_BUF);
    data->tab = tab_new (data->tcti);
    data->source = command_source_new (data->manager, fds[0], data->tab);
    if (data->source == NULL)
        g_error ("failed to allocate new command_source");

    *state = data;
}

static void
command_source_start_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    close (data->wakeup_send_fd);
    g_object_unref (data->source);
    g_object_unref (data->manager);
    g_object_unref (data->tab);
    free (data);
}
/* command_source_start_test end */

/* command_source_wakeup_test
 * Here we test the command_source wakeup mechanism. We do all of the
 * necessary setup to get the session source thread running and then write
 * a bit of data to it. This causes it to run the wakeup callback that we
 * provided. We get an indication that this callback was run through the
 * user_data provided, namely the wokeup gboolean from the source_test_data
 * structure.
*/
static void
command_source_wakeup_setup (void **state)
{
    source_test_data_t *data;
    int fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        g_error ("failed to get pipe2s");
    data->tab = tab_new ((Tcti*)tcti_echo_new (TCTI_ECHO_MIN_BUF));
    data->source = command_source_new (data->manager,
                                        fds[0],
                                        data->tab);
    data->wakeup_send_fd = fds[1];
    if (data->source == NULL)
        g_error ("failed to allocate new command_source");

    *state = data;
}
static void
command_source_wakeup_test (void **state)
{
    source_test_data_t *data;
    gint ret;

    data = (source_test_data_t*)*state;
    /* This string is arbitrary. */
    ret = command_source_start (data->source);
    assert_int_equal (ret, 0);
    ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    sleep (1);
    assert_true (ret != -1);
   /* sleep here to give the source thread time to react to the data we've
     * sent over the wakeup_send_fd.
     */
    ret = command_source_cancel (data->source);
    assert_int_equal (ret, 0);
    ret = command_source_join (data->source);
    assert_int_equal (ret, 0);
}
/* command_source_wakeup_test end */
/* command_source_session_insert_test begin
 * In this test we create a session source and all that that entails. We then
 * create a new session and insert it into the session manager. We then signal
 * to the source that there's a new session in the manager by sending data to
 * it over the send end of the wakeup pipe "wakeup_send_fd". We then check the
 * session_fdset in the source structure to be sure the receive end of the
 * session pipe is set. This is how we know that the source is now watching
 * for data from the new session.
 */
static void
command_source_session_insert_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    CommandSource *source = data->source;
    SessionManager *manager = data->manager;
    SessionData *session;
    gint ret, receive_fd, send_fd;

    /* */
    session = session_data_new (&receive_fd, &send_fd, 5);
    assert_false (FD_ISSET (session->receive_fd, &source->session_fdset));
    ret = command_source_start(source);
    assert_int_equal (ret, 0);
    session_manager_insert (data->manager, session);
    ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    assert_true (ret > 0);
    sleep (1);
    assert_true (FD_ISSET (session->receive_fd, &source->session_fdset));
    session_manager_remove (data->manager, session);
    command_source_cancel (source);
    command_source_join (source);
}
/* command_source_sesion_insert_test end */
/* command_source_session_data_test start */
/* This test picks up where the insert test left off.
 * This time we insert a session but then write some data to it.
 * The session source should pick this up, create a DataMessage,
 * insert it into the tab.
 * The tab should turn around and dump this into a TCTI (which it doesn't do
 * yet) and then put the response into the tab output queue.
 * We then grab this response and compare it to the data that we sent.
 * They should be identical.
 */
static void
command_source_session_data_setup (void **state)
{
    source_test_data_t *data;
    int fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        g_error ("failed to get pipe2s");
    data->tab = tab_new ((Tcti*)tcti_echo_new (TCTI_ECHO_MIN_BUF));
    ret = tab_start (data->tab);
    if (ret != 0)
        g_error ("failed to start tab");
    data->source = command_source_new (data->manager,
                                        fds[0],
                                        data->tab);
    data->wakeup_send_fd = fds[1];
    if (data->source == NULL)
        g_error ("failed to allocate new command_source");

    ret = command_source_start (data->source);
    if (ret != 0)
        g_error ("failed to start CommandSource");

    *state = data;
}
static void
command_source_session_data_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    close (data->wakeup_send_fd);
    command_source_cancel (data->source);
    command_source_join (data->source);
    tab_cancel (data->tab);
    tab_join (data->tab);

    g_object_unref (data->source);
    g_object_unref (data->tab);
    g_object_unref (data->manager);
    free (data);
}
static void
command_source_session_data_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    CommandSource *source = data->source;
    SessionManager    *manager = data->manager;
    Tab               *tab     = data->tab;
    DataMessage *msg;
    SessionData *session;
    gint ret, receive_fd, send_fd;

    session = session_data_new (&receive_fd, &send_fd, 5);
    assert_false (FD_ISSET (session->receive_fd, &source->session_fdset));
    session_manager_insert (data->manager, session);
    ret = write (data->wakeup_send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    assert_true (ret > 0);
    sleep (1);
    assert_true (FD_ISSET (session->receive_fd, &source->session_fdset));

    g_debug ("writing to send_fd");
    ret = write (send_fd, WAKEUP_DATA, WAKEUP_SIZE);
    assert_true (ret > 0);
    g_debug ("calling tab_get_timeout_response\n");
    msg = DATA_MESSAGE (tab_get_timeout_response (tab, 1e6));

    g_debug ("I got this message back:");
    data_message_print (msg);
    assert_memory_equal (WAKEUP_DATA, msg->data, WAKEUP_SIZE);
    session_manager_remove (data->manager, session);
    g_object_unref (msg);
}
/* command_source_session_data_test end */
int
main (int argc,
      char* argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (command_source_allocate_test,
                                  command_source_allocate_setup,
                                  command_source_allocate_teardown),
        unit_test_setup_teardown (command_source_start_test,
                                  command_source_start_setup,
                                  command_source_start_teardown),
        unit_test_setup_teardown (command_source_wakeup_test,
                                  command_source_wakeup_setup,
                                  command_source_start_teardown),
        unit_test_setup_teardown (command_source_session_insert_test,
                                  command_source_wakeup_setup,
                                  command_source_start_teardown),
        unit_test_setup_teardown (command_source_session_data_test,
                                  command_source_session_data_setup,
                                  command_source_session_data_teardown),
    };
    return run_tests (tests);
}
