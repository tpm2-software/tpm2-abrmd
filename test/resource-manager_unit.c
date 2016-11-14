#include <glib.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "resource-manager.h"
#include "tcti-echo.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "thread-interface.h"
#include "tpm2-command.h"
#include "tpm2-header.h"

typedef struct test_data {
    AccessBroker    *access_broker;
    ResourceManager *resource_manager;
    SessionData     *session;
    TctiEcho        *tcti_echo;
    Tpm2Command     *command;
    Tpm2Response    *responce;
} test_data_t;

static void
resource_manager_setup (void **state)
{
    test_data_t *data;
    TSS2_RC rc;

    data = calloc (1, sizeof (test_data_t));
    data->tcti_echo = tcti_echo_new (1024);
    rc = tcti_echo_initialize (data->tcti_echo);
    if (rc != TSS2_RC_SUCCESS)
        g_debug ("tcti_echo_initialize FAILED");
    data->access_broker = access_broker_new (TCTI (data->tcti_echo));
    data->resource_manager = resource_manager_new (data->access_broker);

    *state = data;
}
static void
resource_manager_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_debug ("resource_manager_teardown");
    g_object_unref (data->resource_manager);
    if (data->access_broker) {
        g_debug ("resource_manager unref access_broker");
        g_object_unref (data->access_broker);
    }
    if (data->tcti_echo) {
        g_debug ("resource_manager unref TctiEcho");
        g_object_unref (data->tcti_echo);
    }
    if (data->session) {
        g_debug ("resource_manager unref SessionData");
        g_object_unref (data->session);
    }
    if (data->command) {
        g_debug ("resource_manager unref Tpm2Command");
        g_object_unref (data->command);
    }
    free (data);
}
/**
 * A test: ensure that that ResourceManager created in the 'setup' function
 * is identified by the type system as such.
 */
static void
resource_manager_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (IS_RESOURCE_MANAGER (data->resource_manager));
}
/**
 * A test: Ensure that the thread in the resource manager behaves as a
 * thread should.
 */
static void
resource_manager_thread_lifecycle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (thread_start  (THREAD (data->resource_manager)), 0);
    assert_int_equal (thread_cancel (THREAD (data->resource_manager)), 0);
    assert_int_equal (thread_join   (THREAD (data->resource_manager)), 0);
}
/**
 * A test: Ensure that the Sink interface to the ResourceManager works. We
 * create a Tpm2Command, send it through the Sink 'enqueue' interface, then
 * pull it out the other end by reaching in to the ResourceManagers internal
 * MessageQueue.
 */
static void
resource_manager_sink_enqueue_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Tpm2Command *command_out;
    guint8 *buffer;
    gint   fds[2] = { 0, };

    data->session = session_data_new (&fds[0], &fds[1], 0);
    buffer = calloc (1, TPM_COMMAND_HEADER_SIZE);
    data->command = tpm2_command_new (data->session, buffer);
    sink_enqueue (SINK (data->resource_manager), G_OBJECT (data->command));
    command_out = TPM2_COMMAND (message_queue_dequeue (data->resource_manager->in_queue));

    assert_int_equal (data->command, command_out);
    assert_int_equal (1, 1);
}
int
main (int   argc,
      char *argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (resource_manager_type_test,
                                  resource_manager_setup,
                                  resource_manager_teardown),
        unit_test_setup_teardown (resource_manager_thread_lifecycle_test,
                                  resource_manager_setup,
                                  resource_manager_teardown),
        unit_test_setup_teardown (resource_manager_sink_enqueue_test,
                                  resource_manager_setup,
                                  resource_manager_teardown),
    };
    return run_tests (tests);
}
