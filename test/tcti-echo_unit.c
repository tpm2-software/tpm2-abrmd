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
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-echo.h"
#include "tss2-tcti-echo.h"
#include "tss2-tcti-echo-priv.h"

typedef struct test_data {
    Tcti              *tcti;
    TctiEcho          *tcti_echo;
} test_data_t;
/**
 * Setup function to create all necessary stuff for our tests.
 */
static int
tcti_echo_setup (void **state)
{
    test_data_t *data;

    data = calloc (1, sizeof (test_data_t));

    data->tcti_echo      = tcti_echo_new (TSS2_TCTI_ECHO_MAX_BUF);
    data->tcti           = TCTI (data->tcti_echo);

    *state = data;
    return 0;
}
/**
 * Setup function that additionally calls the tcti_initialize function.
 */
static int
tcti_echo_setup_with_init (void **state)
{
    test_data_t *data;
    TSS2_RC rc;

    tcti_echo_setup ((void**)&data);
    rc = tcti_initialize (data->tcti);
    assert_int_equal (rc, TSS2_RC_SUCCESS);

    *state = data;
    return 0;
}
/**
 * Teardown function to deallocate everything allocated in the setup.
 */
static int
tcti_echo_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->tcti_echo);
    free (data);
    return 0;
}
/**
 * Test object life cycle: create new object then unref it.
 */
static void
tcti_echo_new_unref_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (IS_TCTI (data->tcti));
    assert_true (IS_TCTI_ECHO (data->tcti_echo));
}
/**
 * A test: call the initialization function by way of the TCTI interface.
 */
static void
tcti_echo_initialize_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;


    rc = tcti_initialize (data->tcti);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
}
/**
 * A test: call the transmit function with a known buffer. Verify that we get
 * a successful RC and then we reach into the TCTI context structure for the
 * echo TCTI, locate the internal buffer and be sure it's got the same data
 * as the buffer we wrote.
 */
static void
tcti_echo_transmit_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    uint8_t buffer [TSS2_TCTI_ECHO_MAX_BUF] = { 0xde, 0xad, 0xbe, 0xef, 0x0 };
    size_t size = 5;
    TCTI_ECHO_CONTEXT *echo_context;

    rc = tcti_transmit (data->tcti, size, buffer);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    echo_context = (TCTI_ECHO_CONTEXT*)tcti_peek_context (data->tcti);
    assert_memory_equal (buffer, echo_context->buf, size);
}
/**
 * A test: call the receive function after manually writing the internal
 * TCTI context. Verify that we get the expected results;
 */
static void
tcti_echo_receive_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc;
    uint8_t buffer_out [TSS2_TCTI_ECHO_MAX_BUF] = { 0, };
    size_t size_out = TSS2_TCTI_ECHO_MAX_BUF;
    TCTI_ECHO_CONTEXT *echo_context;

    echo_context = (TCTI_ECHO_CONTEXT*)tcti_peek_context (data->tcti);
    /* populate the buffer in the TCTI context structure faking a write */
    echo_context->buf[0] = 0xde;
    echo_context->buf[1] = 0xad;
    echo_context->buf[2] = 0xbe;
    echo_context->buf[3] = 0xef;
    echo_context->buf[4] = 0x0;
    echo_context->data_size = 5;
    echo_context->state = CAN_RECEIVE;

    rc = tcti_receive (data->tcti,
                            &size_out,
                            buffer_out,
                            TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (size_out, 5);
    assert_memory_equal (buffer_out, echo_context->buf, size_out);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tcti_echo_new_unref_test,
                                         tcti_echo_setup,
                                         tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tcti_echo_initialize_test,
                                         tcti_echo_setup,
                                         tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tcti_echo_transmit_test,
                                         tcti_echo_setup_with_init,
                                         tcti_echo_teardown),
        cmocka_unit_test_setup_teardown (tcti_echo_receive_test,
                                         tcti_echo_setup_with_init,
                                         tcti_echo_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
