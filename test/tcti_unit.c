/* SPDX-License-Identifier: BSD-2-Clause */
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti.h"
#include "tcti-mock.h"
#include "util.h"

typedef struct {
    TSS2_TCTI_CONTEXT *context;
    Tcti *tcti;
} test_data_t;

static int
tcti_setup (void **state)
{
    test_data_t *data;

    data = calloc (1, sizeof (test_data_t));
    if (data == NULL) {
        g_critical ("%s: failed to allocate test_data_t", __func__);
        return 1;
    }
    data->context = tcti_mock_init_full ();
    if (data->context == NULL) {
        g_critical ("tcti_mock_init_full failed");
        return 1;
    }
    data->tcti = tcti_new (data->context);
    *state = data;

    return 0;
}

static int
tcti_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_clear_object (&data->tcti);
    g_clear_pointer (&data, free);
    return 0;
}

static void
tcti_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (IS_TCTI (data->tcti));
}

static void
tcti_peek_context_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_ptr_equal (data->context, tcti_peek_context (data->tcti));
}

static void
tcti_transmit_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc, rc_expected = TSS2_RC_SUCCESS;
    size_t size = 10;
    uint8_t command [10] = { 0 };

    will_return (tcti_mock_transmit, rc_expected);
    rc = tcti_transmit (data->tcti, size, command);
    assert_int_equal (rc, rc_expected);
}

static void
tcti_receive_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TSS2_RC rc, rc_expected = TSS2_RC_SUCCESS;
    size_t size = 20;
    uint8_t response [20] = { 0 }, response_expected [10] = { 0x1, };

    ((TCTI_MOCK_CONTEXT*)data->context)->state = RECEIVE;
    will_return (tcti_mock_receive, response_expected);
    will_return (tcti_mock_receive, sizeof (response_expected));
    will_return (tcti_mock_receive, rc_expected);
    rc = tcti_receive (data->tcti, &size, response, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_memory_equal (response, response_expected, size);
    assert_int_equal (size, sizeof (response_expected));
    assert_int_equal (rc, rc_expected);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tcti_type_test,
                                         tcti_setup,
                                         tcti_teardown),
        cmocka_unit_test_setup_teardown (tcti_peek_context_test,
                                         tcti_setup,
                                         tcti_teardown),
        cmocka_unit_test_setup_teardown (tcti_transmit_test,
                                         tcti_setup,
                                         tcti_teardown),
        cmocka_unit_test_setup_teardown (tcti_receive_test,
                                         tcti_setup,
                                         tcti_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
