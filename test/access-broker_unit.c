/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <glib.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "access-broker.h"
#include "tcti-echo.h"
#include "tpm2-header.h"
#include "tpm2-response.h"

#define MAX_COMMAND_VALUE 2048
#define MAX_RESPONSE_VALUE 1024

typedef struct test_data {
    AccessBroker *broker;
    Connection  *connection;
    TctiEcho     *tcti;
    Tpm2Command  *command;
    Tpm2Response *response;
    gboolean      acquired_lock;
} test_data_t;
/**
 * The access broker initialize function calls the Startup function so we
 * must mock it here.
 */
TSS2_RC
__wrap_Tss2_Sys_Startup (TSS2_SYS_CONTEXT *sapi_context,
                         TPM_SU            startup_type)
{
    TSS2_RC rc;

    rc = (TSS2_RC)mock ();
    g_debug ("__wrap_Tss2_Sys_Startup returning: 0x%x", rc);
    return rc;
}
/**
 * The AccessBroker initialize function calls the GetCapability function to
 * get fixed TPM properties. The only ones it cares about are the MAX sizes
 * for command / response buffers. Hardcode those here.
 */
TSS2_RC
__wrap_Tss2_Sys_GetCapability (TSS2_SYS_CONTEXT         *sysContext,
                               TSS2_SYS_CMD_AUTHS const *cmdAuthsArray,
                               TPM_CAP                   capability,
                               UINT32                    property,
                               UINT32                    propertyCount,
                               TPMI_YES_NO              *moreData,
                               TPMS_CAPABILITY_DATA     *capabilityData,
                               TSS2_SYS_RSP_AUTHS       *rspAuthsArray)

{
    TSS2_RC rc;

    capabilityData->capability = TPM_CAP_TPM_PROPERTIES;
    capabilityData->data.tpmProperties.tpmProperty[0].property = TPM_PT_MAX_COMMAND_SIZE;
    capabilityData->data.tpmProperties.tpmProperty[0].value    = (guint32)mock ();
    capabilityData->data.tpmProperties.tpmProperty[1].property = TPM_PT_MAX_RESPONSE_SIZE;
    capabilityData->data.tpmProperties.tpmProperty[1].value    = (guint32)mock ();
    capabilityData->data.tpmProperties.count = 2;

    rc = (TSS2_RC)mock ();
    g_debug ("__wrap_Tss2_Sys_GetCapability returning: 0x%x", rc);
    return rc;
}
TSS2_RC
__wrap_tcti_echo_transmit (TSS2_TCTI_CONTEXT *tcti_context,
                           size_t             size,
                           uint8_t           *command)
{
    TSS2_RC rc;

    rc = (TSS2_RC)mock ();
    g_debug ("__wrap_tcti_echo_transmit returning: 0x%x", rc);

    return rc;
}
TSS2_RC
__wrap_tcti_echo_receive (TSS2_TCTI_CONTEXT *tcti_context,
                          size_t            *size,
                          uint8_t           *response,
                          int32_t            timeout)
{
    TSS2_RC rc;

    rc = (TSS2_RC)mock ();
    g_debug ("__wrap_tcti_echo_receive returning: 0x%x", rc);

    return rc;
}
/**
 * Do the minimum setup required by the AccessBroker object. This does not
 * call the access_broker_init_tpm function intentionally. We test that function
 * in a separate test.
 */
static void
access_broker_setup (void **state)
{
    test_data_t *data;
    TSS2_RC      rc;

    data = calloc (1, sizeof (test_data_t));
    data->tcti = tcti_echo_new (MAX_COMMAND_VALUE);
    rc = tcti_echo_initialize (data->tcti);
    if (rc != TSS2_RC_SUCCESS)
        g_error ("failed to initialize the echo TCTI");
    data->broker = access_broker_new (TCTI (data->tcti));

    *state = data;
}
/*
 * This setup function chains up to the base setup function. Additionally
 * it done some special handling to wrap the TCTI transmit / receive
 * function since the standard linker tricks don't work with the TCTI
 * function pointers. Finally we also invoke the AccessBroker init function
 * while preparing return values for various SAPI commands that it invokes.
 */
static void
access_broker_setup_with_init (void **state)
{
    test_data_t *data;

    access_broker_setup (state);
    data = (test_data_t*)*state;
    /* can't wrap the tss2_tcti_transmit using linker tricks */
    TSS2_TCTI_RECEIVE  (tcti_peek_context (TCTI (data->tcti))) = __wrap_tcti_echo_receive;
    TSS2_TCTI_TRANSMIT (tcti_peek_context (TCTI (data->tcti))) = __wrap_tcti_echo_transmit;

    will_return (__wrap_Tss2_Sys_Startup, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, MAX_COMMAND_VALUE);
    will_return (__wrap_Tss2_Sys_GetCapability, MAX_RESPONSE_VALUE);
    will_return (__wrap_Tss2_Sys_GetCapability, TSS2_RC_SUCCESS);
    access_broker_init_tpm (data->broker);
}
/*
 * This setup function chains up to the 'setup_with_init' function.
 * Additionally it creates a Connection and Tpm2Command object for use in
 * the unit test.
 */
static void
access_broker_setup_with_command (void **state)
{
    test_data_t *data;
    gint fds [2] = { 0 };
    guint8 *buffer;
    HandleMap *handle_map;

    access_broker_setup_with_init (state);
    data = (test_data_t*)*state;
    buffer = calloc (1, TPM_HEADER_SIZE);
    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    data->connection = connection_new (&fds[0], &fds[1], 0, handle_map);
    g_object_unref (handle_map);
    data->command = tpm2_command_new (data->connection, buffer, (TPMA_CC){ 0, });
}
/*
 * Function to teardown data created for tests.
 */
static void
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
/**
 * Test the initialization route for the AccessBroker. A successful call
 * to the init functhion should return TSS2_RC_SUCCESS and the 'initialized'
 * flag should be set to 'true'.
 */
static void
access_broker_init_tpm_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    will_return (__wrap_Tss2_Sys_Startup, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, MAX_COMMAND_VALUE);
    will_return (__wrap_Tss2_Sys_GetCapability, MAX_RESPONSE_VALUE);
    will_return (__wrap_Tss2_Sys_GetCapability, TSS2_RC_SUCCESS);
    assert_int_equal (access_broker_init_tpm (data->broker), TSS2_RC_SUCCESS);
    assert_true (data->broker->initialized);
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

    assert_int_equal (access_broker_lock (data->broker), 0);
    data->acquired_lock = TRUE;
    assert_int_equal (access_broker_unlock (data->broker), 0);

    return NULL;
}

static void
access_broker_lock_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    pthread_t thread_id;

    data->acquired_lock = FALSE;
    assert_int_equal (access_broker_lock (data->broker), 0);
    assert_int_equal (pthread_create (&thread_id, NULL, lock_thread, data),
                      0);
    sleep (1);
    assert_false (data->acquired_lock);
    assert_int_equal (access_broker_unlock (data->broker), 0);
    pthread_join (thread_id, NULL);
}
/**
 * Here we're testing the internals of the 'access_broker_send_command'
 * function. We're wrapping the tcti_transmit command in the TCTI that
 * the access broker is using. This test causes the TCTI transmit
 * function to return an arbitrary respose code. In this case we should
 * receive a NULL pointer back in place of the Tpm2Response and the
 * out parameter RC is set to the RC value.
 */
static void
access_broker_send_command_tcti_transmit_fail_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc = TSS2_RC_SUCCESS, rc_expected = 99;
    Connection *connection;

    will_return (__wrap_tcti_echo_transmit, rc_expected);
    data->response = access_broker_send_command (data->broker, data->command, &rc);
    /* the response code should be the one we passed to __wrap_tcti_echo_transmit */
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

    will_return (__wrap_tcti_echo_transmit, TSS2_RC_SUCCESS);
    will_return (__wrap_tcti_echo_receive, rc_expected);
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

    will_return (__wrap_tcti_echo_transmit, TSS2_RC_SUCCESS);
    will_return (__wrap_tcti_echo_receive,  TSS2_RC_SUCCESS);
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

int
main (int   argc,
      char *argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (access_broker_type_test,
                                  access_broker_setup,
                                  access_broker_teardown),
        unit_test_setup_teardown (access_broker_init_tpm_test,
                                  access_broker_setup,
                                  access_broker_teardown),
        unit_test_setup_teardown (access_broker_get_max_command_test,
                                  access_broker_setup_with_init,
                                  access_broker_teardown),
        unit_test_setup_teardown (access_broker_get_max_response_test,
                                  access_broker_setup_with_init,
                                  access_broker_teardown),
        unit_test_setup_teardown (access_broker_lock_test,
                                  access_broker_setup_with_init,
                                  access_broker_teardown),
        unit_test_setup_teardown (access_broker_send_command_tcti_transmit_fail_test,
                                  access_broker_setup_with_command,
                                  access_broker_teardown),
        unit_test_setup_teardown (access_broker_send_command_tcti_receive_fail_test,
                                  access_broker_setup_with_command,
                                  access_broker_teardown),
        unit_test_setup_teardown (access_broker_send_command_success,
                                  access_broker_setup_with_command,
                                  access_broker_teardown),
    };
    return run_tests (tests);
}
