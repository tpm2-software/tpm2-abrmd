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
#include <stdarg.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"
#include "tss2-tcti-echo.h"
#include "tss2-tcti-echo-priv.h"

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define TSS2_TCTI_POLL_HANDLE_ZERO_INIT { .fd = 0, .events = 0, .revents = 0 }
#else
#define TSS2_TCTI_POLL_HANDLE_ZERO_INIT { 0 }
#endif

typedef struct test_data {
    TSS2_TCTI_CONTEXT *tcti_context;
} test_data_t;
/**
 * Test setup function: allocate structure to hold test data, initialize
 * some value in said structure.
 */
static int
tss2_tcti_echo_setup (void **state)
{
    test_data_t *data;

    data = calloc (1, sizeof (test_data_t));

    *state = data;
    return 0;
}
/**
 * Test teardown function: deallocate whatever resources are allocated in
 * the setup and test functions.
 */
static int
tss2_tcti_echo_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    free (data);

    *state = NULL;
    return 0;
}
/**
 * A test: verify that something functioned properly.
 */
static void
tss2_tcti_echo_get_size_unit (void **state)
{
    TSS2_RC rc;
    size_t  size = 0;
    UNUSED_PARAM(state);

    rc = tss2_tcti_echo_init (NULL, &size, TSS2_TCTI_ECHO_MAX_BUF);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_true (size > 0);
}
/**
 * A test: Verify that a NULL tcti_context and 'size' parameter produce a
 * TSS2_TCTI_RC_BAD_VALUE RC.
 */
static void
tss2_tcti_echo_null_ctx_and_size_unit (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tss2_tcti_echo_init (NULL, NULL, TSS2_TCTI_ECHO_MAX_BUF);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Verify that a request for a buffer less than the minimum results
 * in a TSS2_TCTI_RC_BAD_VALUE RC.
 */
static void
tss2_tcti_echo_init_buf_lt_min_unit (void **state)
{
    TSS2_RC rc;
    size_t size = 0;
    UNUSED_PARAM(state);

    rc = tss2_tcti_echo_init (NULL, &size, TSS2_TCTI_ECHO_MIN_BUF - 1);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Verify that a request for a buffer greater than the maximum
 * results in a TSS2_TCTI_RC_BAD_VALUE RC.
 */
static void
tss2_tcti_echo_init_buf_gt_max_unit (void **state)
{
    TSS2_RC rc;
    size_t size = 0;
    UNUSED_PARAM(state);

    rc = tss2_tcti_echo_init (NULL, &size, TSS2_TCTI_ECHO_MAX_BUF + 1);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Successful initialization of the echo TCTI.
 */
static void
tss2_tcti_echo_init_success_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    size_t size = 0;

    rc = tss2_tcti_echo_init (NULL, &size, TSS2_TCTI_ECHO_MAX_BUF);
    assert_int_equal (rc, TSS2_RC_SUCCESS);

    data->tcti_context = calloc (1, size);
    assert_non_null (data->tcti_context);

    rc = tss2_tcti_echo_init (data->tcti_context,
                              &size,
                              TSS2_TCTI_ECHO_MAX_BUF);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    free (data->tcti_context);
}
/**
 * Test initialization routine that additionally initializes the echo TCTI.
 * This should only be used after the initialization tests have passed.
 */
static int
tss2_tcti_echo_init_setup (void **state)
{
    test_data_t *data;
    TSS2_RC rc;
    size_t size = 0;

    data = calloc (1, sizeof (test_data_t));
    assert_non_null (data);

    rc = tss2_tcti_echo_init (NULL, &size, TSS2_TCTI_ECHO_MAX_BUF);
    assert_int_equal (rc, TSS2_RC_SUCCESS);

    data->tcti_context = calloc (1, size);
    assert_non_null (data->tcti_context);

    rc = tss2_tcti_echo_init (data->tcti_context,
                              &size,
                              TSS2_TCTI_ECHO_MAX_BUF);
    assert_int_equal (rc, TSS2_RC_SUCCESS);

    *state = data;
    return 0;
}
/**
 * Test teardown function to finalize and free the echo TCTI context as
 * well as the test data structure.
 */
static int
tss2_tcti_echo_init_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    Tss2_Tcti_Finalize (data->tcti_context);
    free (data->tcti_context);
    free (data);

    *state = NULL;
    return 0;
}
/**
 * A test: Cancel a command sent to the Echo TCTI. This is currently
 * unimplemented but we put this here so that if it ever is implemented
 * then this test will fail and prompt a proper test to be written.
 */
static void
tss2_tcti_echo_cancel_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;

    rc = Tss2_Tcti_Cancel (data->tcti_context);
    assert_int_equal (rc, TSS2_TCTI_RC_NOT_IMPLEMENTED);
}
/**
 * A test: get_poll_handles a command sent to the Echo TCTI. This is
 * currently unimplemented but we put this here so that if it ever is
 * implemented then this test will fail and prompt a proper test to be
 * written.
 */
static void
tss2_tcti_echo_get_poll_handles_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    TSS2_TCTI_POLL_HANDLE handles[3] = {
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT, TSS2_TCTI_POLL_HANDLE_ZERO_INIT,
        TSS2_TCTI_POLL_HANDLE_ZERO_INIT
    };
    size_t  num_handles = 3;

    rc = Tss2_Tcti_GetPollHandles (data->tcti_context,
                                     handles,
                                     &num_handles);
    assert_int_equal (rc, TSS2_TCTI_RC_NOT_IMPLEMENTED);
}
/**
 * A test: set_locality a command sent to the Echo TCTI. This is currently
 * unimplemented but we put this here so that if it ever is implemented
 * then this test will fail and prompt a proper test to be written.
 */
static void
tss2_tcti_echo_set_locality_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    uint8_t locality = 0;

    rc = Tss2_Tcti_SetLocality (data->tcti_context, locality);
    assert_int_equal (rc, TSS2_TCTI_RC_NOT_IMPLEMENTED);
}
/**
 * A test: Call transmit with a NULL context. Ensure we get the right error
 * code back. According to the spec this value would be
 * TSS2_TCTI_RC_BAD_REFERENCE but it currently returns
 * TSS2_TCTI_RC_BAD_CONTEXT.
 */
static void
tss2_tcti_echo_transmit_null_context_unit (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = Tss2_Tcti_Transmit (NULL, 0, NULL);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
}
/**
 * A test: Call transmit with a NULL command buffer. Ensure we get the
 * right error code back.
 */
static void
tss2_tcti_echo_transmit_null_command_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;

    rc = Tss2_Tcti_Transmit (data->tcti_context, 5, NULL);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Call transmit with a size less than 0. Ensure we get the
 * right error code back.
 */
static void
tss2_tcti_echo_transmit_negative_size_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    uint8_t buffer [TSS2_TCTI_ECHO_MAX_BUF] = { 0, };

    rc = Tss2_Tcti_Transmit (data->tcti_context, -1, buffer);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Call transmit with an invalid size. Ensure we get the
 * right error code back.
 */
static void
tss2_tcti_echo_transmit_size_gt_max_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    uint8_t buffer [TSS2_TCTI_ECHO_MAX_BUF] = { 0, };

    rc = Tss2_Tcti_Transmit (data->tcti_context,
                             TSS2_TCTI_ECHO_MAX_BUF + 1,
                             buffer);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Call transmit with the context state machine in the wrong state.
 * Ensure that we get the right error code back. This requires that we
 * muck around in the private TCTI context structure.
 */
static void
tss2_tcti_echo_transmit_bad_sequence_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)data->tcti_context;
    uint8_t buffer [TSS2_TCTI_ECHO_MAX_BUF] = { 0, };
    TSS2_RC rc;

    echo_context->state = CAN_RECEIVE;
    rc = Tss2_Tcti_Transmit (data->tcti_context,
                             TSS2_TCTI_ECHO_MAX_BUF,
                             buffer);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_SEQUENCE);
}
/**
 * A test: Test the proper functioning of the transmit call. We transmit
 * a buffer with the maximum data. We then ensure that we get the right
 * return code, that the memory transmitted was copied into the echo TCTI
 * context blob properly, that the echo TCTI 'data_size' member is set to
 * the same size as the data we passed in, and that the 'state' member
 * reflects that data has been sent.
 */
static void
tss2_tcti_echo_transmit_success_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)data->tcti_context;
    uint8_t buffer [TSS2_TCTI_ECHO_MAX_BUF] = { 0xde, 0xad, 0xbe, 0xef, 0x0 };
    TSS2_RC rc;

    rc = Tss2_Tcti_Transmit (data->tcti_context,
                             TSS2_TCTI_ECHO_MAX_BUF,
                             buffer);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_memory_equal (buffer, echo_context->buf, echo_context->buf_size);
    assert_int_equal (echo_context->data_size, TSS2_TCTI_ECHO_MAX_BUF);
    assert_int_equal (echo_context->state, CAN_RECEIVE);
}
/**
 * A test: Call receive with a NULL context. Ensure we get the right error
 * code back. According to the spec this value would be
 * TSS2_TCTI_RC_BAD_REFERENCE but it currently returns
 * TSS2_TCTI_RC_BAD_CONTEXT.
 */
static void
tss2_tcti_echo_receive_null_context_unit (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = Tss2_Tcti_Receive (NULL, NULL, NULL, 0);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
}
/**
 * A test: Call receive with a NULL size. Ensure we get the right error
 * code back.
 */
static void
tss2_tcti_echo_receive_null_size_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)data->tcti_context;
    TSS2_RC rc;

    echo_context->state = CAN_RECEIVE;
    rc = Tss2_Tcti_Receive (data->tcti_context, NULL, NULL, 0);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Call the receive function while passing it a 'size' parameter that
 * indicates the buffer passed in is only 5 bytes in size. We trick the
 * echo tcti into thinking it has TCTI_ECHO_MAX_BUF bytes to return to the
 * caller so it should reply indicating that the caller's response buffer is
 * too small. It should also set the size to the buffer size required to
 * hold the response.
 */
static void
tss2_tcti_echo_receive_insufficient_size_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)data->tcti_context;
    TSS2_RC rc;
    size_t size = 5;

    echo_context->data_size = TSS2_TCTI_ECHO_MAX_BUF;
    echo_context->state = CAN_RECEIVE;
    rc = Tss2_Tcti_Receive (data->tcti_context,
                            &size,
                            NULL,
                            TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_INSUFFICIENT_BUFFER);
    assert_int_equal (size, echo_context->data_size);
}
/**
 * A test: Call receive with a NULL response buffer. Ensure we get the right
 * error code back.
 */
static void
tss2_tcti_echo_receive_null_response_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)data->tcti_context;
    TSS2_RC rc;
    size_t size = TSS2_TCTI_ECHO_MAX_BUF;

    echo_context->state = CAN_RECEIVE;
    rc = Tss2_Tcti_Receive (data->tcti_context, &size, NULL, 0);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Call receive with a "bad" timeout value. Ensure we get the right
 * error code back.
 */
static void
tss2_tcti_echo_receive_negative_timeout_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)data->tcti_context;
    TSS2_RC rc;
    size_t size = 5;
    uint8_t buffer [TSS2_TCTI_ECHO_MAX_BUF] = { 0, };

    echo_context->state = CAN_RECEIVE;
    rc = Tss2_Tcti_Receive (data->tcti_context, &size, buffer, -99);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/**
 * A test: Call receive with a "bad" timeout value. Ensure we get the right
 * error code back.
 */
static void
tss2_tcti_echo_receive_success_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)data->tcti_context;
    TSS2_RC rc;
    size_t size = TSS2_TCTI_ECHO_MAX_BUF;
    uint8_t buffer [TSS2_TCTI_ECHO_MAX_BUF] = { 0, };

    echo_context->state = CAN_RECEIVE;
    echo_context->data_size = 4;
    echo_context->buf[0] = 0xde;
    echo_context->buf[1] = 0xad;
    echo_context->buf[2] = 0xbe;
    echo_context->buf[3] = 0xef;

    rc = Tss2_Tcti_Receive (data->tcti_context,
                            &size,
                            buffer,
                            TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_memory_equal (buffer, echo_context->buf, 4);
    assert_int_equal (size, 4);
}
/**
 * Test driver.
 */
int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_get_size_unit,
                                         tss2_tcti_echo_setup,
                                         tss2_tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_null_ctx_and_size_unit,
                                         tss2_tcti_echo_setup,
                                         tss2_tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_init_buf_lt_min_unit,
                                         tss2_tcti_echo_setup,
                                         tss2_tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_init_buf_gt_max_unit,
                                         tss2_tcti_echo_setup,
                                         tss2_tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_init_success_unit,
                                         tss2_tcti_echo_setup,
                                         tss2_tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_cancel_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_get_poll_handles_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_set_locality_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_transmit_null_context_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_transmit_null_command_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_transmit_negative_size_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_transmit_size_gt_max_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_transmit_bad_sequence_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_transmit_success_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_receive_null_context_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_receive_null_size_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_receive_insufficient_size_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_receive_null_response_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_receive_negative_timeout_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
        cmocka_unit_test_setup_teardown (tss2_tcti_echo_receive_success_unit,
                                         tss2_tcti_echo_init_setup,
                                         tss2_tcti_echo_init_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
