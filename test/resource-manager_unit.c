#include <glib.h>
#include <inttypes.h>
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
    Tpm2Response    *response;
    gint             send_fd, recv_fd;
    TPM_HANDLE       vhandles [2];
    TPMA_CC         command_attrs;
} test_data_t;

/**
 * Mock function for testing the resource_manager_process_tpm2_command
 * function which depends on the access_broker_send_command and must
 * handle the call and the associated error conditions.
 */
Tpm2Response*
__wrap_access_broker_send_command (AccessBroker *access_broker,
                                   Tpm2Command  *command,
                                   TSS2_RC      *rc)
{
    Tpm2Response *response;

    *rc      = (TSS2_RC)mock ();
    response = TPM2_RESPONSE (mock ());

    return response;
}
/**
 * Mock function for testing the resoruce_manager_process_tpm2_command
 * function. When the AccessBroker returns a Tpm2Response object the
 * ResourceManager passes this object to whatever sink has been provided
 * to it. For the purposes of testing we don't provide a valid Sink.
 * Instead we mock this function as a way to take the response object
 * away from the ResourceManager and verify that it was produced
 * correctly.
 *
 * We do something weird here though: we pass the test data structure into
 * this function by way of the will_return / wrap mechanism. This isn't how
 * these are intended to be used but it's the only way to get the test
 * data structure into the function so that we can verify the sink was
 * passed the Tpm2Response object that we expect.
 */
void
__wrap_sink_enqueue (Sink      *self,
                     GObject   *obj)
{
    test_data_t *data = (test_data_t*)mock ();
    data->response = TPM2_RESPONSE (obj);
}
TSS2_RC
__wrap_access_broker_context_saveflush (AccessBroker *broker,
                                        TPM_HANDLE    handle,
                                        TPMS_CONTEXT *contedt)
{
   return (TSS2_RC)mock ();
}
/*
 * Wrap call to access_broker_context_load. Pops two parameters off the
 * stack with the 'mock' command. The first is the RC which is returned
 * directly to the caller. The other is the new physical handle that was
 * allocated by the TPM for the context that was just loaded.
 */
TSS2_RC
__wrap_access_broker_context_load (AccessBroker *access_broker,
                                   TPMS_CONTEXT *context,
                                   TPM_HANDLE   *handle)
{
    TSS2_RC    rc      = (TSS2_RC)mock ();
    TPM_HANDLE phandle = (TPM_HANDLE)mock ();

    assert_non_null (handle);
    *handle = phandle;

    return rc;
}
static void
resource_manager_setup (void **state)
{
    test_data_t *data;
    HandleMap   *handle_map;
    TSS2_RC rc;

    data = calloc (1, sizeof (test_data_t));
    data->tcti_echo = tcti_echo_new (1024);
    rc = tcti_echo_initialize (data->tcti_echo);
    if (rc != TSS2_RC_SUCCESS)
        g_debug ("tcti_echo_initialize FAILED");
    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    data->access_broker = access_broker_new (TCTI (data->tcti_echo));
    data->resource_manager = resource_manager_new (data->access_broker);
    data->session = session_data_new (&data->recv_fd, &data->send_fd, 10, handle_map);
    g_object_unref (handle_map);

    *state = data;
}
static void
resource_manager_setup_two_transient_handles (void **state)
{
    test_data_t *data;
    guint8 *buffer;

    resource_manager_setup (state);
    data = *state;

    data->vhandles [0] = HR_TRANSIENT + 0x1;
    data->vhandles [1] = HR_TRANSIENT + 0x2;
    data->command_attrs.val = (2 << 25) + TPM_CC_StartAuthSession; /* 2 handles + TPM2_StartAuthSession */

    /* create Tpm2Command that we'll be transforming */
    buffer = calloc (1, TPM_COMMAND_HEADER_SIZE + 2 * sizeof (TPM_HANDLE));
    buffer [0]  = 0x80;
    buffer [1]  = 0x02;
    buffer [2]  = 0x00;
    buffer [3]  = 0x00;
    buffer [4]  = 0x00;
    buffer [5]  = TPM_COMMAND_HEADER_SIZE + 2 * sizeof (TPM_HANDLE);
    buffer [6]  = 0x00;
    buffer [7]  = 0x00;
    buffer [8]  = TPM_CC_StartAuthSession >> 8;
    buffer [9]  = TPM_CC_StartAuthSession & 0xff;
    buffer [10] = TPM_HT_TRANSIENT;
    buffer [11] = 0x00;
    buffer [12] = 0x00;
    buffer [13] = data->vhandles [0] & 0xff; /* first virtual handle */
    buffer [14] = TPM_HT_TRANSIENT;
    buffer [15] = 0x00;
    buffer [16] = 0x00;
    buffer [17] = data->vhandles [1] & 0xff; /* second virtual handle */
    data->command = tpm2_command_new (data->session,
                                      buffer,
                                      data->command_attrs);
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

    buffer = calloc (1, TPM_COMMAND_HEADER_SIZE);
    data->command = tpm2_command_new (data->session, buffer, (TPMA_CC){ 0, });
    sink_enqueue (SINK (data->resource_manager), G_OBJECT (data->command));
    command_out = TPM2_COMMAND (message_queue_dequeue (data->resource_manager->in_queue));

    assert_int_equal (data->command, command_out);
    assert_int_equal (1, 1);
}
/**
 * A test: exercise the resource_manager_process_tpm2_command function.
 * This function is normally invoked by the ResoruceManager internal
 * thread. We invoke it directly here to control variables and timing
 * issues with the thread.
 */
static void
resource_manager_process_tpm2_command_success_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Tpm2Response *response;
    guint8 *buffer;

    buffer = calloc (1, TPM_COMMAND_HEADER_SIZE);
    /**
     * we don't use the test data structure to hold the command object since
     * it will be freed by the call to resource_manager_process_tpm2_command
     * and the teardown function will attempt to free it again if set.
     */
    data->command = tpm2_command_new (data->session, buffer, (TPMA_CC){ 0, });
    response = tpm2_response_new_rc (data->session, TSS2_RC_SUCCESS);
    /*
     * This response object will be freed by the process_tpm2_command
     * function. We take an extra reference and free when we're done.
     */
    g_object_ref (response);

    will_return (__wrap_access_broker_send_command, TSS2_RC_SUCCESS);
    will_return (__wrap_access_broker_send_command, response);
    /**
     * The sink_enqueue wrap function will assign the Tpm2Response it's passed
     * to the test data structure.
     */
    will_return (__wrap_sink_enqueue, data);
    resource_manager_process_tpm2_command (data->resource_manager,
                                           data->command);
    assert_int_equal (data->response, response);
    g_object_unref (response);
}
static void
resource_manager_flushsave_context_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    HandleMapEntry *entry;
    TPM_HANDLE vhandle = HR_TRANSIENT + 0x1, phandle = HR_TRANSIENT + 0x2;
    TSS2_RC rc;

    entry = handle_map_entry_new (phandle, vhandle);
    will_return (__wrap_access_broker_context_saveflush, TSS2_RC_SUCCESS);
    rc = resource_manager_flushsave_context (data->resource_manager, entry);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (handle_map_entry_get_phandle (entry), 0);
}
/*
 * This test case pushes an error RC on to the mock stack for the
 * access_broker_context_flushsave function. The flushsave_context function
 * in the RM will then successfully find the phandle in the provided entry
 * but the call to the AccessBroker will fail.
 * The function under test should propagate this RC to the caller (us) and
 * should *NOT* update the phandle in the provided entry.
 */
static void
resource_manager_flushsave_context_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    HandleMapEntry *entry;
    TPM_HANDLE vhandle = HR_TRANSIENT + 0x1, phandle = HR_TRANSIENT + 0x2;
    TSS2_RC rc;

    entry = handle_map_entry_new (phandle, vhandle);
    will_return (__wrap_access_broker_context_saveflush, TPM_RC_INITIALIZE);
    rc = resource_manager_flushsave_context (data->resource_manager, entry);
    assert_int_equal (rc, TPM_RC_INITIALIZE);
    assert_int_equal (handle_map_entry_get_phandle (entry), phandle);
}
/*
 */
static void
resource_manager_virt_to_phys_test (void **state)
{
    test_data_t    *data = (test_data_t*)*state;
    HandleMapEntry *entry;
    TPM_HANDLE      phandle = HR_TRANSIENT + 0x1, vhandle = 0;
    TPM_HANDLE      handle_ret = 0;
    TSS2_RC         rc = TSS2_RC_SUCCESS;

    will_return (__wrap_access_broker_context_load, TSS2_RC_SUCCESS);
    will_return (__wrap_access_broker_context_load, phandle);
    /* and the rest of it */

    /* create & populate HandleMap for transient handle */
    vhandle = tpm2_command_get_handle (data->command, 0);
    entry = handle_map_entry_new (phandle, vhandle);
    /* function under test, */
    rc = resource_manager_virt_to_phys (data->resource_manager,
                                        data->command,
                                        entry,
                                        0);
    g_object_unref (entry);
    handle_ret = tpm2_command_get_handle (data->command, 0);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (phandle, handle_ret);

}
/*
 */
static void
resource_manager_load_contexts_test (void **state)
{
    test_data_t    *data = (test_data_t*)*state;
    HandleMapEntry *entry;
    HandleMapEntry *entries [3] = { 0 };
    HandleMap      *map;
    TPM_HANDLE      phandles [2] = {
        HR_TRANSIENT + 0xeb,
        HR_TRANSIENT + 0xbe,
    };
    TPM_HANDLE      vhandles [3] = { 0 };
    TPM_HANDLE      handle_ret;
    TSS2_RC         rc = TSS2_RC_SUCCESS;
    guint8          handle_count, i;
    guint           entry_count = 2;

    will_return (__wrap_access_broker_context_load, TSS2_RC_SUCCESS);
    will_return (__wrap_access_broker_context_load, phandles [0]);
    will_return (__wrap_access_broker_context_load, TSS2_RC_SUCCESS);
    will_return (__wrap_access_broker_context_load, phandles [1]);

    handle_count = tpm2_command_get_handle_count (data->command);
    tpm2_command_get_handles (data->command, vhandles, 3);
    map = session_data_get_trans_map (data->session);
    for (i = 0; i < handle_count; ++i) {
        entry = handle_map_entry_new (phandles [i], vhandles [i]);
        handle_map_insert (map, vhandles [i], entry);
        g_object_unref (entry);
    }
    g_debug ("before resource_manager_load_contexts");
    rc = resource_manager_load_contexts (data->resource_manager,
                                         data->command,
                                         entries,
                                         &entry_count);
    g_debug ("after resource_manager_load_contexts");
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    for (i = 0; i < handle_count; ++i) {
        handle_ret = tpm2_command_get_handle (data->command, i);
        assert_int_equal (phandles [i], handle_ret);
    }
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
        unit_test_setup_teardown (resource_manager_process_tpm2_command_success_test,
                                  resource_manager_setup,
                                  resource_manager_teardown),
        unit_test_setup_teardown (resource_manager_flushsave_context_test,
                                  resource_manager_setup,
                                  resource_manager_teardown),
        unit_test_setup_teardown (resource_manager_flushsave_context_fail_test,
                                  resource_manager_setup,
                                  resource_manager_teardown),
        unit_test_setup_teardown (resource_manager_virt_to_phys_test,
                                  resource_manager_setup_two_transient_handles,
                                  resource_manager_teardown),
        unit_test_setup_teardown (resource_manager_load_contexts_test,
                                  resource_manager_setup_two_transient_handles,
                                  resource_manager_teardown),
    };
    return run_tests (tests);
}
