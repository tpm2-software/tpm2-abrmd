/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <assert.h>
#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include <tss2/tss2_mu.h>

#include "tpm2.h"
#include "resource-manager.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "tcti.h"
#include "tcti-mock.h"
#include "tpm2-command.h"
#include "tpm2-header.h"
#include "util.h"

typedef struct test_data {
    Tpm2    *tpm2;
    ResourceManager *resource_manager;
    Connection      *connection;
    Tpm2Command     *command;
    Tpm2Response    *response;
    gint             client_fd;
    TPM2_HANDLE       vhandles [2];
    TPMA_CC         command_attrs;
} test_data_t;

/**
 * Mock function for testing the resource_manager_process_tpm2_command
 * function which depends on the tpm2_send_command and must
 * handle the call and the associated error conditions.
 */
Tpm2Response*
__wrap_tpm2_send_command (Tpm2 *tpm2,
                                   Tpm2Command  *command,
                                   TSS2_RC      *rc)
{
    Tpm2Response *response;
    UNUSED_PARAM(tpm2);
    UNUSED_PARAM(command);

    *rc      = mock_type (TSS2_RC);
    response = TPM2_RESPONSE (mock_ptr_type (GObject*));

    return response;
}
/**
 * Mock function for testing the resource_manager_process_tpm2_command
 * function. When the Tpm2 returns a Tpm2Response object the
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
    UNUSED_PARAM(self);
    test_data_t *data = mock_ptr_type (test_data_t*);
    data->response = TPM2_RESPONSE (obj);
}
TSS2_RC
__wrap_tpm2_context_saveflush (Tpm2 *broker,
                                        TPM2_HANDLE    handle,
                                        TPMS_CONTEXT *context)
{
    UNUSED_PARAM(broker);
    UNUSED_PARAM(handle);
    UNUSED_PARAM(context);
   return mock_type (TSS2_RC);
}
/*
 * Wrap call to tpm2_context_load. Pops two parameters off the
 * stack with the 'mock' command. The first is the RC which is returned
 * directly to the caller. The other is the new physical handle that was
 * allocated by the TPM for the context that was just loaded.
 */
TSS2_RC
__wrap_tpm2_context_load (Tpm2 *tpm2,
                                   TPMS_CONTEXT *context,
                                   TPM2_HANDLE   *handle)
{
    TSS2_RC    rc      = mock_type (TSS2_RC);
    TPM2_HANDLE phandle = mock_type (TPM2_HANDLE);
    UNUSED_PARAM(tpm2);
    UNUSED_PARAM(context);

    assert_non_null (handle);
    *handle = phandle;

    return rc;
}
static int
resource_manager_setup (void **state)
{
    test_data_t *data;
    GIOStream   *iostream;
    HandleMap   *handle_map;
    SessionList *session_list;
    Tcti *tcti = NULL;
    TSS2_TCTI_CONTEXT *context;

    context = tcti_mock_init_full ();
    tcti = tcti_new (context);

    data = calloc (1, sizeof (test_data_t));
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    data->tpm2 = tpm2_new (tcti);
    g_clear_object (&tcti);
    session_list = session_list_new (SESSION_LIST_MAX_ENTRIES_DEFAULT,
                                     SESSION_LIST_MAX_ABANDONED_DEFAULT);
    data->resource_manager = resource_manager_new (data->tpm2,
                                                   session_list);
    g_clear_object (&session_list);
    iostream = create_connection_iostream (&data->client_fd);
    data->connection = connection_new (iostream, 10, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);

    *state = data;
    return 0;
}
static int
resource_manager_setup_two_transient_handles (void **state)
{
    test_data_t *data;
    guint8 *buffer;
    size_t  buffer_size;

    resource_manager_setup (state);
    data = *state;

    data->vhandles [0] = TPM2_HR_TRANSIENT + 0x1;
    data->vhandles [1] = TPM2_HR_TRANSIENT + 0x2;
    data->command_attrs = (2 << 25) + TPM2_CC_StartAuthSession; /* 2 handles + TPM2_StartAuthSession */

    /* create Tpm2Command that we'll be transforming */
    buffer_size = TPM_HEADER_SIZE + 2 * sizeof (TPM2_HANDLE);
    buffer = calloc (1, buffer_size);
    *(TPM2_ST*)buffer = htobe16 (TPM2_ST_NO_SESSIONS);
    buffer [2]  = 0x00;
    buffer [3]  = 0x00;
    buffer [4]  = 0x00;
    buffer [5]  = buffer_size;
    buffer [6]  = 0x00;
    buffer [7]  = 0x00;
    buffer [8]  = TPM2_CC_StartAuthSession >> 8;
    buffer [9]  = TPM2_CC_StartAuthSession & 0xff;
    buffer [10] = TPM2_HT_TRANSIENT;
    buffer [11] = 0x00;
    buffer [12] = 0x00;
    buffer [13] = data->vhandles [0] & 0xff; /* first virtual handle */
    buffer [14] = TPM2_HT_TRANSIENT;
    buffer [15] = 0x00;
    buffer [16] = 0x00;
    buffer [17] = data->vhandles [1] & 0xff; /* second virtual handle */
    data->command = tpm2_command_new (data->connection,
                                      buffer,
                                      buffer_size,
                                      data->command_attrs);
    return 0;
}
static int
resource_manager_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_debug ("resource_manager_teardown");
    g_object_unref (data->resource_manager);
    if (data->tpm2) {
        g_debug ("resource_manager unref tpm2");
        g_object_unref (data->tpm2);
    }
    if (data->connection) {
        g_debug ("resource_manager unref Connection");
        g_object_unref (data->connection);
    }
    if (data->command) {
        g_debug ("resource_manager unref Tpm2Command");
        g_object_unref (data->command);
    }
    free (data);
    return 0;
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
 * A test: Ensure that the Sink interface to the ResourceManager works. We
 * create a Tpm2Command, send it through the ResourceManager enqueue
 * function then pull it out the other end by reaching in to the
 * ResourceManagers internal MessageQueue.
 * We *DO NOT* use the sink interface here since we've mock'd that for
 * other purposes and it would make the test largely meaningless.
 */
static void
resource_manager_sink_enqueue_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Tpm2Command *command_out;
    guint8 *buffer;

    buffer = calloc (1, TPM_HEADER_SIZE);
    data->command = tpm2_command_new (data->connection, buffer, TPM_HEADER_SIZE, (TPMA_CC){ 0, });
    resource_manager_enqueue (SINK (data->resource_manager), G_OBJECT (data->command));
    command_out = TPM2_COMMAND (message_queue_dequeue (data->resource_manager->in_queue));

    assert_int_equal (data->command, command_out);
    assert_int_equal (1, 1);
}
/**
 * A test: exercise the resource_manager_process_tpm2_command function.
 * This function is normally invoked by the ResourceManager internal
 * thread. We invoke it directly here to control variables and timing
 * issues with the thread.
 */
static void
resource_manager_process_tpm2_command_success_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    Tpm2Response *response;
    guint8 *buffer;

    buffer = calloc (1, TPM_HEADER_SIZE);
    /**
     * we don't use the test data structure to hold the command object since
     * it will be freed by the call to resource_manager_process_tpm2_command
     * and the teardown function will attempt to free it again if set.
     */
    data->command = tpm2_command_new (data->connection, buffer, TPM_HEADER_SIZE, (TPMA_CC){ 0, });
    response = tpm2_response_new_rc (data->connection, TSS2_RC_SUCCESS);
    /*
     * This response object will be freed by the process_tpm2_command
     * function. We take an extra reference and free when we're done.
     */
    g_object_ref (response);

    will_return (__wrap_tpm2_send_command, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_send_command, response);
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
    TPM2_HANDLE vhandle = TPM2_HR_TRANSIENT + 0x1, phandle = TPM2_HR_TRANSIENT + 0x2;

    entry = handle_map_entry_new (phandle, vhandle);
    will_return (__wrap_tpm2_context_saveflush, TSS2_RC_SUCCESS);
    resource_manager_flushsave_context (entry, data->resource_manager);
    assert_int_equal (handle_map_entry_get_phandle (entry), 0);
}

static void
resource_manager_flushsave_context_same_entries_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    HandleMapEntry *entry1, *entry2, *entry3;
    HandleMap *map;
    TPM2_HANDLE vhandle = TPM2_HR_TRANSIENT + 0x1,
                phandle1 = TPM2_HR_TRANSIENT + 0x2,
                phandle2 = TPM2_HR_TRANSIENT + 0x3;

    /* Create two map entries for the same vhandle and diffrent phandle */
    entry1 = handle_map_entry_new (phandle1, vhandle);
    entry2 = handle_map_entry_new (phandle2, vhandle);
    map = handle_map_new(TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);

    /* Insert the two mappings */
    handle_map_insert(map, vhandle, entry1);
    handle_map_insert(map, vhandle, entry2);

    /* Call flushsave_context on the second entry */
    will_return (__wrap_tpm2_context_saveflush, TSS2_RC_SUCCESS);
    resource_manager_flushsave_context (entry2, data->resource_manager);

    /* and check if the first is still valid */
    entry3 = handle_map_vlookup(map, vhandle);
    assert_int_equal (handle_map_entry_get_phandle (entry3), phandle1);

    /* Cleanup */
    handle_map_remove (map, vhandle);
    g_object_unref (entry1);
    g_object_unref (entry2);
    g_object_unref (map);
}

/*
 * This test case pushes an error RC on to the mock stack for the
 * tpm2_context_flushsave function. The flushsave_context function
 * in the RM will then successfully find the phandle in the provided entry
 * but the call to the Tpm2 will fail.
 * The function under test should propagate this RC to the caller (us) and
 * should *NOT* update the phandle in the provided entry.
 */
static void
resource_manager_flushsave_context_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    HandleMapEntry *entry;
    TPM2_HANDLE vhandle = TPM2_HR_TRANSIENT + 0x1, phandle = TPM2_HR_TRANSIENT + 0x2;

    entry = handle_map_entry_new (phandle, vhandle);
    will_return (__wrap_tpm2_context_saveflush, TPM2_RC_INITIALIZE);
    resource_manager_flushsave_context (entry, data->resource_manager);
    assert_int_equal (handle_map_entry_get_phandle (entry), phandle);
}
/*
 */
static void
resource_manager_virt_to_phys_test (void **state)
{
    test_data_t    *data = (test_data_t*)*state;
    HandleMapEntry *entry;
    TPM2_HANDLE      phandle = TPM2_HR_TRANSIENT + 0x1, vhandle = 0;
    TPM2_HANDLE      handle_ret = 0;
    TSS2_RC         rc = TSS2_RC_SUCCESS;

    will_return (__wrap_tpm2_context_load, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_context_load, phandle);
    /* and the rest of it */

    /* create & populate HandleMap for transient handle */
    vhandle = tpm2_command_get_handle (data->command, 0);
    entry = handle_map_entry_new (0, vhandle);
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
resource_manager_load_handles_test(void **state)
{
    test_data_t    *data = (test_data_t*)*state;
    HandleMapEntry *entry;
    GSList         *entry_slist;
    HandleMap      *map;
    TPM2_HANDLE      phandles [3] = {
        TPM2_HR_TRANSIENT + 0xeb,
        TPM2_HR_TRANSIENT + 0xbe,
        0
    };
    TPM2_HANDLE      vhandles [3] = { 0, 0, 0 };
    TPM2_HANDLE      handle_ret;
    TSS2_RC         rc = TSS2_RC_SUCCESS;
    size_t          handle_count = 2, i;

    will_return (__wrap_tpm2_context_load, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_context_load, phandles [0]);
    will_return (__wrap_tpm2_context_load, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_context_load, phandles [1]);

    tpm2_command_get_handles (data->command, vhandles, &handle_count);
    map = connection_get_trans_map (data->connection);
    if (handle_count > 2) {
        assert (FALSE);
    }
    for (i = 0; i < handle_count; ++i) {
        entry = handle_map_entry_new (0, vhandles [i]);
        handle_map_insert (map, vhandles [i], entry);
        g_object_unref (entry);
    }
    rc = resource_manager_load_handles (data->resource_manager,
                                        data->command,
                                        &entry_slist);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_true (handle_count <= 2);
    for (i = 0; i < handle_count; ++i) {
        handle_ret = tpm2_command_get_handle (data->command, i);
        assert_int_equal (phandles [i], handle_ret);
    }
}
/*
 * This setup function calls the 'resource_manager_setup' function to create
 * the ResourceManager object etc. It then creates a Tpm2Response object
 * approprite for use in testing the 'get_cap_post_process' function.
 */
int
resource_manager_setup_getcap (void **state)
{
    int setup_ret;
    test_data_t *data = NULL;
    size_t offset = 0, buf_size = 0;
    uint8_t *buf = NULL;
    TSS2_RC rc;
    /* structures that define the Tpm2Response object used in GetCap tests */
    TPMI_YES_NO yes_no = TPM2_NO;
    TPMS_CAPABILITY_DATA cap_data = {
        .capability = TPM2_CAP_TPM_PROPERTIES,
        /* TPMU_CAPABILITIES, union type defined by 'capability' selector */
        .data = {
            /* TPML_TAGGED_TPM_PROPERTY */
            .tpmProperties = {
                .count = 1,
                /* TPMS_TAGGED_PROPERTY */
                .tpmProperty = {
                    {
                      .property = TPM2_PT_CONTEXT_GAP_MAX,
                      .value = 0xff,
                    }
                }
            }
        }
    };

    setup_ret = resource_manager_setup (state);
    if (setup_ret != 0) {
        g_critical ("%s: resource_manager_setup failed", __func__);
        return setup_ret;
    }
    data = (test_data_t*)*state;
    buf_size = TPM2_MAX_RESPONSE_SIZE;
    g_debug ("%s: sizeof buffer required for TPMS_CAPABILITY_DATA: 0x%zx",
             __func__, offset);

    buf = calloc (1, buf_size);
    if (buf == NULL) {
        g_critical ("%s: failed to allocate buffer: %s", __func__,
                    strerror (errno));
        return 1;
    }
    offset = TPM_HEADER_SIZE;
    rc = Tss2_MU_BYTE_Marshal (yes_no, buf, buf_size, &offset);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("%s: failed to marshal TPMI_YES_NO into response body, "
                    "rc: 0x%" PRIx32, __func__, rc);
        return 1;
    }
    rc = Tss2_MU_TPMS_CAPABILITY_DATA_Marshal (&cap_data, buf, buf_size, &offset);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("%s: failed to marshal TPMS_CAPABILITY_DATA into response "
                    "body, rc: 0x%" PRIx32, __func__, rc);
        return 1;
    }
    rc = tpm2_header_init (buf,
                           buf_size,
                           TPM2_ST_NO_SESSIONS,
                           offset,
                           TSS2_RC_SUCCESS);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("%s: failed to initialize TPM2 Header, rc: 0x%" PRIx32,
                    __func__, rc);
        return 1;
    }
    data->response = tpm2_response_new (data->connection, buf, offset, TPM2_CC_GetCapability);
    return setup_ret;
}
/*
 * Test 'getcap_post_process' function to ensure that TPM2_PT_CONTEXT_GAP_MAX
 * property is properly modified from its initial value of UINT8_MAX to
 * UINT32_MAX.
 */
void
resource_manager_getcap_gap_max_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    size_t offset = TPM_HEADER_SIZE, buf_size;
    uint8_t *buf;
    TSS2_RC rc;
    TPMI_YES_NO yes_no = TPM2_NO;
    TPMS_CAPABILITY_DATA cap_data = { 0 };

    /* get buffer & size from Tpm2Response created in setup function */
    buf = tpm2_response_get_buffer (data->response);
    buf_size = tpm2_response_get_size (data->response);
    assert_non_null (buf);
    /* verify defaults from setup function */
    rc = Tss2_MU_BYTE_Unmarshal (buf, buf_size, &offset, &yes_no);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    rc = Tss2_MU_TPMS_CAPABILITY_DATA_Unmarshal (buf,
                                                 buf_size,
                                                 &offset,
                                                 &cap_data);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (cap_data.data.tpmProperties.tpmProperty [0].value, UINT8_MAX);
    offset = TPM_HEADER_SIZE;
    /* execute function under test */
    rc = get_cap_post_process (data->response);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    rc = Tss2_MU_BYTE_Unmarshal (buf, buf_size, &offset, &yes_no);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    rc = Tss2_MU_TPMS_CAPABILITY_DATA_Unmarshal (buf,
                                                 buf_size,
                                                 &offset,
                                                 &cap_data);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    /* verify property was modified by the RM */
    assert_int_equal (cap_data.data.tpmProperties.tpmProperty [0].value, UINT32_MAX);
}
int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (resource_manager_type_test,
                                         resource_manager_setup,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_sink_enqueue_test,
                                         resource_manager_setup,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_process_tpm2_command_success_test,
                                         resource_manager_setup,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_flushsave_context_test,
                                         resource_manager_setup,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_flushsave_context_same_entries_test,
                                         resource_manager_setup,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_flushsave_context_fail_test,
                                         resource_manager_setup,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_virt_to_phys_test,
                                         resource_manager_setup_two_transient_handles,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_load_handles_test,
                                         resource_manager_setup_two_transient_handles,
                                         resource_manager_teardown),
        cmocka_unit_test_setup_teardown (resource_manager_getcap_gap_max_test,
                                         resource_manager_setup_getcap,
                                         resource_manager_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
