/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include "util.h"
#include "random.h"

#define ENTROPY_SRC "/dev/urandom"

typedef struct test_data {
    Random *random;
} test_data_t;

/* Setup function to allocate our Random gobject. */
static int
random_setup (void **state)
{
    test_data_t *data;

    data = calloc (1, sizeof (test_data_t));
    data->random = RANDOM (random_new ());

    *state = data;
    return 0;
}
/* Teardown function to deallocate the Random object. */
static int
random_teardown (void **state)
{
    test_data_t *data = *state;

    g_object_unref (data->random);
    free (data);
    return 0;
}
/* wrap function for the 'open' system call */
int __real_open (const char *path, int flags, int mode);
int
__wrap_open(const char *pathname,
            int         flags,
            mode_t      mode)
{
    /*
     * Mock calls to 'open' only for operations on the default entropy
     * source used by the Random object (ENTROPY_SRC).
     */
    if (strcmp (pathname, ENTROPY_SRC)) {
        return __real_open (pathname, flags, mode);
    }
    return mock_type (int);
}
/* wrap function for the 'read' system call */
ssize_t
__wrap_read (int     fd,
             void   *buf,
             size_t  count)
{
    UNUSED_PARAM(fd);
    UNUSED_PARAM(buf);
    UNUSED_PARAM(count);
    return mock_type (ssize_t);
}
/* wrap function for the 'close' system call */
int
__wrap_close (int fd)
{
    UNUSED_PARAM(fd);
    return mock_type (int);
}

/* Simple test to test type checking macros. */
static void
random_type_test (void **state)
{
    test_data_t *data = *state;

    assert_true (G_IS_OBJECT (data->random));
    assert_true (IS_RANDOM (data->random));
}
/*
 * Seed the Random object from a file source. The file will never actually
 * be opened or read. All syscalls are wrapped. The wrap functions are
 * prepared such that the random_seed_from_file function executes the
 * common / success code path.
 */
static void
random_seed_from_file_success_test (void **state)
{
    test_data_t *data = *state;
    int ret;

    will_return (__wrap_open, 5);
    will_return (__wrap_read, sizeof (long int));
    will_return (__wrap_close, 0);
    ret = random_seed_from_file (data->random, ENTROPY_SRC);
    assert_int_equal (ret, 0);
}
/*
 * Test a failure condition for the random_seed_from_file function. We
 * cause the 'open' system call to fail and we ensure that the function
 * under test returns an integer indicating the error.
 */
static void
random_seed_from_file_open_fail_test (void **state)
{
    test_data_t *data = *state;
    int ret;

    will_return (__wrap_open, -1);
    ret = random_seed_from_file (data->random, ENTROPY_SRC);
    assert_int_equal (ret, -1);
}
/*
 * Test a failure condition for the random_seed_from_file function. We
 * cause the 'read' system call to fail and we ensure that the function
 * under test returns an integer indicating the error. Additionally we must
 * mock a successful call to 'open' first, and the 'close' as well.
 */
static void
random_seed_from_file_read_fail_test (void **state)
{
    test_data_t *data = *state;
    int ret;

    will_return (__wrap_open, 5);
    will_return (__wrap_read, -1);
    will_return (__wrap_close, 0);
    ret = random_seed_from_file (data->random, ENTROPY_SRC);
    assert_int_equal (ret, -1);
}
/*
 * Test a success condition for the random_seed_from_file function, but
 * force the 'close' system call to fail. In this case we've successfully
 * opened and read from the entropy source we just weren't able to close
 * the file. The response code from the function should still indicate
 * sucess since we got all of the data we need. A warning message will
 * however be output.
 */
static void
random_seed_from_file_close_fail_test (void **state)
{
    test_data_t *data = *state;
    int ret;

    will_return (__wrap_open, 5);
    will_return (__wrap_read, sizeof (long int));
    will_return (__wrap_close, -1);
    ret = random_seed_from_file (data->random, ENTROPY_SRC);
    assert_int_equal (ret, 0);
}
/*
 * Test a failure condition for the random_seed_from_file_function. In
 * this case both the 'read' and 'close' functions fail. The function under
 * test should then return -1.
 */
static void
random_seed_from_file_read_close_fail_test (void **state)
{
    test_data_t *data = *state;
    int ret;

    will_return (__wrap_open, 5);
    will_return (__wrap_read, -1);
    will_return (__wrap_close, -1);
    ret = random_seed_from_file (data->random, ENTROPY_SRC);
    assert_int_equal (ret, -1);
}
/*
 * Test a failure condition for the random_seed_from_file function. In
 * this case the 'read' function succeeds but reads less data than
 * requested / necessary. This should result in the function under test
 * returning an error indicator.
 */
static void
random_seed_from_file_read_short_test (void **state)
{
    test_data_t *data = *state;
    int ret;

    will_return (__wrap_open, 5);
    will_return (__wrap_read, sizeof (long int) - 1);
    will_return (__wrap_close, 0);
    ret = random_seed_from_file (data->random, ENTROPY_SRC);
    assert_int_equal (ret, -1);
}
static int
random_get_bytes_setup (void **state)
{
    test_data_t *data;
    int ret;

    random_setup (state);
    data = *state;

    will_return (__wrap_open, 5);
    will_return (__wrap_read, sizeof (long int));
    will_return (__wrap_close, 0);
    ret = random_seed_from_file (data->random, ENTROPY_SRC);
    assert_int_equal (ret, 0);
    return 0;
}
/*
 * Test a successful call to the random_get_bytes function.
 */
static void
random_get_bytes_success_test (void **state)
{
    test_data_t *data = *state;
    int ret;
    uint8_t dest[sizeof (long int) * 3 - 3] = { 0, };

    ret = random_get_bytes (data->random, dest, sizeof (dest));
    assert_int_equal (ret, sizeof (long int) * 3 - 3);
}

/*
 * Test case to execute a successful call to random_get_uint64.
 * If 0 or UINT64_MAX, it's very likely the data is not random.
 */
static void
random_get_uint64_success_test (void **state)
{
    test_data_t *data = *state;
    uint64_t dest = 0;

    dest = random_get_uint64 (data->random);
    assert_true (dest != 0 && dest != UINT64_MAX);
}

/*
 * Test case to execute a successful call to random_get_uint32.
 * If 0 or UINT32_MAX, it's very likely the data is not random.
 */
static void
random_get_uint32_success_test (void **state)
{
    test_data_t *data = *state;
    uint32_t dest;

    dest = random_get_uint32 (data->random);
    assert_true (dest != 0 && dest != UINT32_MAX);
}

/* Test case to check that a random number is within the given range. */
static void
random_get_uint32_range_success_test (void **state)
{
    test_data_t *data = *state;
    uint32_t dest, low = 6, high = 20;

    dest = random_get_uint32_range (data->random, high, low);
    assert_true (low < dest);
    assert_true (dest < high);
}
gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (random_type_test,
                                         random_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_seed_from_file_success_test,
                                         random_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_seed_from_file_open_fail_test,
                                         random_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_seed_from_file_read_fail_test,
                                         random_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_seed_from_file_close_fail_test,
                                         random_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_seed_from_file_read_close_fail_test,
                                         random_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_seed_from_file_read_short_test,
                                         random_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_get_bytes_success_test,
                                         random_get_bytes_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_get_uint64_success_test,
                                         random_get_bytes_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_get_uint32_success_test,
                                         random_get_bytes_setup,
                                         random_teardown),
        cmocka_unit_test_setup_teardown (random_get_uint32_range_success_test,
                                         random_get_bytes_setup,
                                         random_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
