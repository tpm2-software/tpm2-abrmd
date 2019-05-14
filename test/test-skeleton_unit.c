/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <stdarg.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#define EXPECTED_VALUE 1

typedef struct test_data {
    int value;
} test_data_t;
/**
 * Test setup function: allocate structure to hold test data, initialize
 * some value in said structure.
 */
static int
test_setup (void **state)
{
    test_data_t *data;

    data = calloc (1, sizeof (test_data_t));
    data->value = EXPECTED_VALUE;

    *state = data;
    return 0;
}
/**
 * Test teardown function: deallocate whatever resources are allocated in
 * the setup and test functions.
 */
static int
test_teardown (void **state)
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
test_unit (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (data->value, EXPECTED_VALUE);
}
/**
 * Test driver.
 */
int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (test_unit,
                                         test_setup,
                                         test_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
