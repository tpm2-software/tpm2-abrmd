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
