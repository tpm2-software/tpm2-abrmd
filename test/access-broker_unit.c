/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "access-broker.h"
#include "tpm2-header.h"
#include "tpm2-response.h"

#include "tcti-mock.h"
#include "util.h"

#define MAX_COMMAND_VALUE 2048
#define MAX_RESPONSE_VALUE 1024

#define TAGGED_PROPS_INIT { \
        .count = 2, \
        .tpmProperty = { \
            { \
                .property = TPM2_PT_MAX_COMMAND_SIZE, \
                .value = MAX_COMMAND_VALUE, \
            }, \
            { \
                .property = TPM2_PT_MAX_RESPONSE_SIZE, \
                .value = MAX_RESPONSE_VALUE, \
            }, \
        } \
    };

typedef struct test_data {
    AccessBroker *broker;
    Connection  *connection;
    Tpm2Command  *command;
    Tpm2Response *response;
    gboolean      acquired_lock;
} test_data_t;

TSS2_RC
__wrap_Tss2_Sys_FlushContext (TSS2_SYS_CONTEXT *sysContext,
                              TPM2_HANDLE handle)
{
    UNUSED_PARAM (sysContext);
    UNUSED_PARAM (handle);

    return mock_type (TSS2_RC);
}

TSS2_RC
__wrap_Tss2_Sys_ContextSave (TSS2_SYS_CONTEXT *sysContext,
                             TPM2_HANDLE handle,
                             TPMS_CONTEXT *context)
{
    UNUSED_PARAM (sysContext);
    UNUSED_PARAM (handle);
    UNUSED_PARAM (context);

    return mock_type (TSS2_RC);
}

TSS2_RC
__wrap_Tss2_Sys_ContextLoad (TSS2_SYS_CONTEXT *sysContext,
                             TPMS_CONTEXT *context,
                             TPM2_HANDLE *handle)
{
    UNUSED_PARAM (sysContext);
    UNUSED_PARAM (context);
    UNUSED_PARAM (handle);

    return mock_type (TSS2_RC);
}
TSS2_RC
__wrap_Tss2_Sys_Initialize (TSS2_SYS_CONTEXT *sysContext,
                            size_t contextSize,
                            TSS2_TCTI_CONTEXT *tctiContext,
                            TSS2_ABI_VERSION *abiVersion)
{
    UNUSED_PARAM (sysContext);
    UNUSED_PARAM (contextSize);
    UNUSED_PARAM (tctiContext);
    UNUSED_PARAM (abiVersion);

    return mock_type (TSS2_RC);
}
/**
 * The access broker initialize function calls the Startup function so we
 * must mock it here.
 */
TSS2_RC
__wrap_Tss2_Sys_Startup (TSS2_SYS_CONTEXT *sapi_context,
                         TPM2_SU           startup_type)
{
    TSS2_RC rc;
    UNUSED_PARAM(sapi_context);
    UNUSED_PARAM(startup_type);

    rc = mock_type (TSS2_RC);
    g_debug ("__wrap_Tss2_Sys_Startup returning: 0x%x", rc);
    return rc;
}
/*
 * mock stack:
 *   - TSS2_RC: an RC indicating failure will be returned immediately
 *   - TPML: the TPML from the TPMU_CAPABILITIES structure to be returned
 */
TSS2_RC
__wrap_Tss2_Sys_GetCapability (TSS2_SYS_CONTEXT         *sysContext,
                               TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
                               TPM2_CAP                  capability,
                               UINT32                    property,
                               UINT32                    propertyCount,
                               TPMI_YES_NO              *moreData,
                               TPMS_CAPABILITY_DATA     *capabilityData,
                               TSS2L_SYS_AUTH_RESPONSE  *rspAuthsArray)

{
    UNUSED_PARAM(sysContext);
    UNUSED_PARAM(cmdAuthsArray);
    UNUSED_PARAM(property);
    UNUSED_PARAM(propertyCount);
    UNUSED_PARAM(moreData);
    UNUSED_PARAM(rspAuthsArray);

    TPML_TAGGED_TPM_PROPERTY *tpmProperties;
    TPML_HANDLE *handles;
    TSS2_RC rc;

    rc = mock_type (TSS2_RC);
    g_debug ("__wrap_Tss2_Sys_GetCapability returning: 0x%x", rc);
    if (rc != TSS2_RC_SUCCESS)
        goto out;

    switch (capability) {
    case TPM2_CAP_TPM_PROPERTIES:
        capabilityData->capability = TPM2_CAP_TPM_PROPERTIES;
        tpmProperties = mock_type (TPML_TAGGED_TPM_PROPERTY*);
        memcpy (&capabilityData->data.tpmProperties,
                tpmProperties,
                sizeof (*tpmProperties));
        break;
    case TPM2_CAP_HANDLES:
        capabilityData->capability = TPM2_CAP_HANDLES;
        handles = mock_type (TPML_HANDLE*);
        memcpy (&capabilityData->data.handles,
                handles,
                sizeof (*handles));
        break;
    default:
        g_error ("%s does not understand this capability type", __func__);
    }

out:
    return rc;
}

static Tcti*
mock_tcti_setup (void)
{
    TSS2_TCTI_CONTEXT *context;

    context = tcti_mock_init_full ();
    if (context == NULL) {
        g_critical ("tcti_mock_init_full failed");
        return NULL;
    }

    return tcti_new (context);
}
/**
 * Do the minimum setup required by the AccessBroker object. This does not
 * call the access_broker_init_tpm function intentionally. We test that function
 * in a separate test.
 */
static int
access_broker_setup (void **state)
{
    test_data_t *data;
    Tcti *tcti = NULL;

    data = calloc (1, sizeof (test_data_t));
    tcti = mock_tcti_setup ();
    will_return (__wrap_Tss2_Sys_Initialize, TSS2_RC_SUCCESS);
    data->broker = access_broker_new (tcti);

    g_clear_object (&tcti);
    *state = data;
    return 0;
}
/*
 * This setup function chains up to the base setup function. Additionally
 * it done some special handling to wrap the TCTI transmit / receive
 * function since the standard linker tricks don't work with the TCTI
 * function pointers. Finally we also invoke the AccessBroker init function
 * while preparing return values for various SAPI commands that it invokes.
 */
static int
access_broker_setup_with_init (void **state)
{
    test_data_t *data;
    TPML_TAGGED_TPM_PROPERTY tagged_data = TAGGED_PROPS_INIT;

    access_broker_setup (state);
    data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_Startup, TPM2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, TPM2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, &tagged_data);
    access_broker_init_tpm (data->broker);
    return 0;
}
/*
 * This setup function chains up to the 'setup_with_init' function.
 * Additionally it creates a Connection and Tpm2Command object for use in
 * the unit test.
 */
static int
access_broker_setup_with_command (void **state)
{
    test_data_t *data;
    gint client_fd;
    guint8 *buffer;
    size_t  buffer_size;
    HandleMap *handle_map;
    GIOStream *iostream;

    access_broker_setup_with_init (state);
    data = (test_data_t*)*state;
    buffer_size = TPM_HEADER_SIZE;
    buffer = calloc (1, buffer_size);
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    data->command = tpm2_command_new (data->connection,
                                      buffer,
                                      buffer_size,
                                      (TPMA_CC){ 0, });
    return 0;
}
/*
 * Function to teardown data created for tests.
 */
static int
access_broker_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->broker);
    if (G_IS_OBJECT (data->connection))
        g_object_unref (data->connection);
    if (G_IS_OBJECT (data->command))
        g_object_unref (data->command);
    if (G_IS_OBJECT (data->response))
        g_object_unref (data->response);
    free (data);
    return 0;
}
/**
 * Test to ensure that the GObject type system gives us back the type that
 * we expect.
 */
static void
access_broker_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (IS_ACCESS_BROKER (data->broker));
}
static void
access_broker_startup_fail (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_Startup, TSS2_SYS_RC_BAD_SIZE);
    assert_int_equal (access_broker_send_tpm_startup (data->broker),
                      TSS2_SYS_RC_BAD_SIZE);
}
/**
 * Test the initialization route for the AccessBroker. A successful call
 * to the init function should return TSS2_RC_SUCCESS and the 'initialized'
 * flag should be set to 'true'.
 */
static void
access_broker_init_tpm_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPML_TAGGED_TPM_PROPERTY tagged_data = TAGGED_PROPS_INIT;

    will_return (__wrap_Tss2_Sys_Startup, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, &tagged_data);
    assert_int_equal (access_broker_init_tpm (data->broker), TSS2_RC_SUCCESS);
    assert_true (data->broker->initialized);
}

static void
access_broker_init_tpm_initialized (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (access_broker_init_tpm (data->broker), TSS2_RC_SUCCESS);
    assert_true (data->broker->initialized);
}

static void
access_broker_init_tpm_startup_fail (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_Startup, TPM2_RC_FAILURE);
    assert_int_equal (access_broker_init_tpm (data->broker), TPM2_RC_FAILURE);
}

static void
access_broker_get_max_command_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    guint32 value;

    assert_int_equal (access_broker_get_max_command (data->broker, &value),
                      TSS2_RC_SUCCESS);
    assert_int_equal (value, MAX_COMMAND_VALUE);
}

static void
access_broker_get_max_response_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    guint32 value;

    assert_int_equal (access_broker_get_max_response (data->broker, &value),
                      TSS2_RC_SUCCESS);
    assert_int_equal (value, MAX_RESPONSE_VALUE);
}

static void*
lock_thread (void *param)
{
    test_data_t *data = (test_data_t*)param;

    access_broker_lock (data->broker);
    data->acquired_lock = TRUE;
    access_broker_unlock (data->broker);

    return NULL;
}

static void
access_broker_lock_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    pthread_t thread_id;

    data->acquired_lock = FALSE;
    access_broker_lock (data->broker);
    assert_int_equal (pthread_create (&thread_id, NULL, lock_thread, data),
                      0);
    sleep (1);
    assert_false (data->acquired_lock);
    access_broker_unlock (data->broker);
    pthread_join (thread_id, NULL);
}
/**
 * Here we're testing the internals of the 'access_broker_send_command'
 * function. We're wrapping the tcti_transmit command in the TCTI that
 * the access broker is using. This test causes the TCTI transmit
 * function to return an arbitrary response code. In this case we should
 * receive a NULL pointer back in place of the Tpm2Response and the
 * out parameter RC is set to the RC value.
 */
static void
access_broker_send_command_tcti_transmit_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc = TSS2_RC_SUCCESS, rc_expected = 99;
    Connection *connection;

    will_return (tcti_mock_transmit, rc_expected);
    data->response = access_broker_send_command (data->broker, data->command, &rc);
    assert_int_equal (rc, rc_expected);
    /* the Tpm2Response object we get back should have the same RC */
    assert_int_equal (tpm2_response_get_code (data->response), rc_expected);
    /**
     * the Tpm2Response object we get back should have the same connection as
     * the Tpm2Command we tried to send.
     */
    connection = tpm2_response_get_connection (data->response);
    assert_int_equal (connection, data->connection);
    g_object_unref (connection);
}
/*
 * This test exercises a failure path through the access_broker_send_command
 * function. We use the 'mock' pattern to cause the tcti_receive command to
 * return an RC indicating failure.
 */
static void
access_broker_send_command_tcti_receive_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc = TSS2_RC_SUCCESS, rc_expected = 99;
    Connection *connection;

    will_return (tcti_mock_transmit, TSS2_RC_SUCCESS);
    will_return (tcti_mock_receive, NULL);
    will_return (tcti_mock_receive, 0);
    will_return (tcti_mock_receive, rc_expected);
    data->response = access_broker_send_command (data->broker, data->command, &rc);
    /* the response code should be the one we passed to __wrap_tcti_echo_receive */
    assert_int_equal (rc, rc_expected);
    /* the Tpm2Response object we get back should have the same RC */
    assert_int_equal (tpm2_response_get_code (data->response), rc_expected);
    /**
     * the Tpm2Response object we get back should have the same connection as
     * the Tpm2Command we tried to send.
     */
    connection = tpm2_response_get_connection (data->response);
    assert_int_equal (connection, data->connection);
    g_object_unref (connection);
}
/*
 * This test exercises the success path through the
 * access_broker_send_command function.
 */
static void
access_broker_send_command_success (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    Connection *connection;
    uint8_t buf [1] = { 0 };
    size_t size = sizeof (buf);

    will_return (tcti_mock_transmit, TSS2_RC_SUCCESS);
    will_return (tcti_mock_receive, buf);
    will_return (tcti_mock_receive, size);
    will_return (tcti_mock_receive, TSS2_RC_SUCCESS);
    data->response = access_broker_send_command (data->broker, data->command, &rc);
    /* the response code should be the one we passed to __wrap_tcti_echo_receive */
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    /* the Tpm2Response object we get back should have the same RC */
    assert_int_equal (tpm2_response_get_code (data->response), TSS2_RC_SUCCESS);
    /**
     * the Tpm2Response object we get back should have the same connection as
     * the Tpm2Command we tried to send.
     */
    connection = tpm2_response_get_connection (data->response);
    assert_int_equal (connection, data->connection);
    g_object_unref (connection);
}

static void
access_broker_get_trans_object_count_caps_fail (void **state)
{
    TSS2_RC rc;
    uint32_t count = 0;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_GetCapability, TPM2_RC_DISABLED);
    rc = access_broker_get_trans_object_count (data->broker, &count);
    assert_int_equal (rc, TPM2_RC_DISABLED);
}

static void
access_broker_get_trans_object_count_success (void **state)
{
    TSS2_RC rc;
    uint32_t count = 0;
    test_data_t *data = (test_data_t*)*state;
    TPML_HANDLE handle = {
        .count = 2,
       .handle = { 555, 444 },
    };

    will_return (__wrap_Tss2_Sys_GetCapability, TPM2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, &handle);
    rc = access_broker_get_trans_object_count (data->broker, &count);
    assert_int_equal (rc, TPM2_RC_SUCCESS);
    assert_int_equal (count, handle.count);
}

static void
access_broker_sapi_context_init_fail (void **state)
{
    UNUSED_PARAM (state);

    TSS2_SYS_CONTEXT *ctx = NULL;
    Tcti *tcti = NULL;

    tcti = mock_tcti_setup ();
    will_return (__wrap_Tss2_Sys_Initialize, TPM2_RC_FAILURE);
    ctx = sapi_context_init (tcti);
    assert_null (ctx);
    g_clear_object (&tcti);
}

static void
access_broker_context_load_test (void **state)
{
    TSS2_RC rc;
    TPMS_CONTEXT context = { 0, };
    TPM2_HANDLE handle = 0;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_ContextLoad, TSS2_RC_SUCCESS);
    rc = access_broker_context_load (data->broker, &context, &handle);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
}

static void
access_broker_context_load_fail (void **state)
{
    TSS2_RC rc;
    TPMS_CONTEXT context = { 0, };
    TPM2_HANDLE handle = 0;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_ContextLoad, TPM2_RC_FAILURE);
    rc = access_broker_context_load (data->broker, &context, &handle);
    assert_int_equal (rc, TPM2_RC_FAILURE);
}

static void
access_broker_context_save_test (void **state)
{
    TSS2_RC rc;
    TPMS_CONTEXT context = { 0, };
    TPM2_HANDLE handle = 0;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_ContextSave, TPM2_RC_SUCCESS);
    rc = access_broker_context_save (data->broker, handle, &context);
    assert_int_equal (rc, TPM2_RC_SUCCESS);
}

static void
access_broker_context_flush_fail (void **state)
{
    TSS2_RC rc;
    TPM2_HANDLE handle = 0;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_FlushContext, TPM2_RC_FAILURE);
    rc = access_broker_context_flush (data->broker, handle);
    assert_int_equal (rc, TPM2_RC_FAILURE);
}

static void
access_broker_context_saveflush_save_fail (void **state)
{
    TSS2_RC rc;
    TPMS_CONTEXT context = { 0, };
    TPM2_HANDLE handle = 0;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_ContextSave, TPM2_RC_FAILURE);
    rc = access_broker_context_saveflush (data->broker, handle, &context);
    assert_int_equal (rc, TPM2_RC_FAILURE);
}

static void
access_broker_context_saveflush_flush_fail (void **state)
{
    TSS2_RC rc;
    TPMS_CONTEXT context = { 0, };
    TPM2_HANDLE handle = 0;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_ContextSave, TPM2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_FlushContext, TPM2_RC_FAILURE);
    rc = access_broker_context_saveflush (data->broker, handle, &context);
    assert_int_equal (rc, TPM2_RC_FAILURE);
}

static void
access_broker_flush_all_unlocked_getcap_fail (void **state)
{
    TSS2_RC rc;
    TSS2_SYS_CONTEXT *sys_ctx = (TSS2_SYS_CONTEXT*)0x1;
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_GetCapability, TPM2_RC_FAILURE);
    rc = access_broker_flush_all_unlocked (data->broker,
                                           sys_ctx,
                                           TPM2_ACTIVE_SESSION_FIRST,
                                           TPM2_ACTIVE_SESSION_LAST);
    assert_int_equal (rc, TPM2_RC_FAILURE);
}

static void
access_broker_flush_all_unlocked_flush_fail (void **state)
{
    TSS2_RC rc;
    TSS2_SYS_CONTEXT *sys_ctx = (TSS2_SYS_CONTEXT*)0x1;
    test_data_t *data = (test_data_t*)*state;
    TPML_HANDLE handle = {
        .count = 1,
       .handle = { 555 },
    };

    will_return (__wrap_Tss2_Sys_GetCapability, TPM2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, &handle);
    will_return (__wrap_Tss2_Sys_FlushContext, TPM2_RC_FAILURE);
    rc = access_broker_flush_all_unlocked (data->broker,
                                           sys_ctx,
                                           TPM2_ACTIVE_SESSION_FIRST,
                                           TPM2_ACTIVE_SESSION_LAST);
    assert_int_equal (rc, TPM2_RC_SUCCESS);
}

int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (access_broker_type_test,
                                         access_broker_setup,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_startup_fail,
                                         access_broker_setup,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_init_tpm_test,
                                         access_broker_setup,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_init_tpm_initialized,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_init_tpm_startup_fail,
                                         access_broker_setup,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_get_max_command_test,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_get_max_response_test,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_lock_test,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_send_command_tcti_transmit_fail_test,
                                         access_broker_setup_with_command,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_send_command_tcti_receive_fail_test,
                                         access_broker_setup_with_command,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_send_command_success,
                                         access_broker_setup_with_command,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_get_trans_object_count_caps_fail,
                                         access_broker_setup_with_command,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_get_trans_object_count_success,
                                         access_broker_setup_with_command,
                                         access_broker_teardown),
        cmocka_unit_test (access_broker_sapi_context_init_fail),
        cmocka_unit_test_setup_teardown (access_broker_context_load_test,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_context_load_fail,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_context_save_test,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_context_flush_fail,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_context_saveflush_save_fail,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_context_saveflush_flush_fail,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_flush_all_unlocked_getcap_fail,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
        cmocka_unit_test_setup_teardown (access_broker_flush_all_unlocked_flush_fail,
                                         access_broker_setup_with_init,
                                         access_broker_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
