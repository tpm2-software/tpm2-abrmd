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
#include "sink-interface.h"
#include "source-interface.h"
#include "command-source.h"
#include "tpm2-command.h"

typedef struct source_test_data {
    SessionManager *manager;
    CommandSource *source;
    SessionData *session;
    gboolean match;
} source_test_data_t;


SessionData*
__wrap_session_manager_lookup_fd (SessionManager *manager,
                                  gint            fd_in)
{
    g_debug ("__wrap_session_manager_lookup_fd");
    return SESSION_DATA (mock ());
}

Tpm2Command*
__wrap_tpm2_command_new_from_fd (SessionData *session,
                                 gint         fd)
{
    g_debug ("__wrap_tpm2_command_from_fd");
    return TPM2_COMMAND (mock ());
}
void
__wrap_sink_enqueue (Sink     *sink,
                     GObject  *obj)
{
    Tpm2Command **command;

    g_debug ("__wrap_sink_enqueue");
    command = (Tpm2Command**)mock ();

    *command = TPM2_COMMAND (obj);
}
/* command_source_allocate_test begin
 * Test to allcoate and destroy a CommandSource.
 */
static void
command_source_allocate_test (void **state)
{
    source_test_data_t *data = (source_test_data_t *)*state;
    CommandSource *source = NULL;

    data->source = command_source_new (data->manager);
    assert_non_null (data->source);
}

static void
command_source_allocate_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = session_manager_new ();

    *state = data;
}

static void
command_source_allocate_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    g_object_unref (data->source);
    g_object_unref (data->manager);
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

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = session_manager_new ();
    if (data->manager == NULL)
        g_error ("failed to allocate new session_manager");
    data->source = command_source_new (data->manager);
    if (data->source == NULL)
        g_error ("failed to allocate new command_source");

    *state = data;
}

static void
command_source_start_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    g_object_unref (data->source);
    g_object_unref (data->manager);
    free (data);
}
/* command_source_start_test end */

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
command_source_wakeup_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = session_manager_new ();
    data->source  = command_source_new (data->manager);
    *state = data;
}

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
    sleep (1);
    assert_true (FD_ISSET (session->receive_fd, &source->session_fdset));
    session_manager_remove (data->manager, session);
    command_source_cancel (source);
    command_source_join (source);
}
/* command_source_sesion_insert_test end */
/* command_source_session_data_test start */
static void
command_source_session_data_setup (void **state)
{
    source_test_data_t *data;
    int fds[2] = { 0 }, ret;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = session_manager_new ();
    data->source = command_source_new (data->manager);

    *state = data;
}
static void
command_source_session_data_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    command_source_cancel (data->source);
    command_source_join (data->source);

    g_object_unref (data->source);
    g_object_unref (data->manager);
    free (data);
}
/**
 * A test: Test the command_source_session_responder function. We do this
 * by creating a new SessionData object, associating it with a new
 * Tpm2Command object (that we populate with a command body), and then
 * calling the command_source_session_responder.
 * This function will in turn call the session_manager_lookup_fd,
 * tpm2_command_new_from_fd, before finally calling the sink_enqueue function.
 * We mock these 3 functions to control the flow through the function under
 * test.
 * The most tricky bit to this is the way the __wrap_sink_enqueue function
 * works. Since this thing has no return value we pass it a reference to a
 * Tpm2Command pointer. It sets this to the Tpm2Command that it receives.
 * We determine success /failure for this test by verifying that the
 * sink_enqueue function receives the same Tpm2Command that we passed to
 * the command under test (command_source_session_responder).
 */
static void
command_source_session_responder_success_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    SessionData *session;
    Tpm2Command *command, *command_out;
    gint fds[2] = { 0, };
    guint8 *buffer;
    guint8 data_in [] = { 0x80, 0x01, 0x0,  0x0,  0x0,  0x16,
                          0x0,  0x0,  0x01, 0x7a, 0x0,  0x0,
                          0x0,  0x06, 0x0,  0x0,  0x01, 0x0,
                          0x0,  0x0,  0x0,  0x7f, 0x0a };
    gboolean result = FALSE;

    session = session_data_new (&fds[0], &fds[1], 0);
    /**
     * We must dynamically allocate the buffer for the Tpm2Command since
     * it takes ownership of the data buffer and frees it as part of it's
     * instance finalize method.
     */
    buffer = calloc (1, sizeof (data_in));
    memcpy (buffer, data_in, sizeof (data_in));
    command = tpm2_command_new (session, buffer);
    /* prime wraps */
    will_return (__wrap_session_manager_lookup_fd, session);
    will_return (__wrap_tpm2_command_new_from_fd, command);
    will_return (__wrap_sink_enqueue, &command_out);
    result = command_source_session_responder (data->source, 0, NULL);

    assert_int_equal (command_out, command);
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
        unit_test_setup_teardown (command_source_session_insert_test,
                                  command_source_wakeup_setup,
                                  command_source_start_teardown),
        unit_test_setup_teardown (command_source_session_responder_success_test,
                                  command_source_session_data_setup,
                                  command_source_session_data_teardown),
    };
    return run_tests (tests);
}
