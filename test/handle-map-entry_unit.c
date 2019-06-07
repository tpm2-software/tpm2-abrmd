/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "handle-map-entry.h"
#include "util.h"

#define PHANDLE 0xdeadbeef
#define VHANDLE 0xfeebdaed

typedef struct {
    HandleMapEntry *handle_map_entry;
} test_data_t;
/*
 * Setup function
 */
static int
handle_map_entry_setup (void **state)
{
    test_data_t *data   = NULL;

    data = calloc (1, sizeof (test_data_t));
    data->handle_map_entry = handle_map_entry_new (PHANDLE, VHANDLE);

    *state = data;
    return 0;
}
/**
 * Tear down all of the data from the setup function. We don't have to
 * free the data buffer (data->buffer) since the Tpm2Command frees it as
 * part of its finalize function.
 */
static int
handle_map_entry_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->handle_map_entry);
    free (data);
    return 0;
}
/*
 * This is a test for memory management / reference counting. The setup
 * function does exactly that so when we get the Tpm2Command object we just
 * check to be sure it's a GObject and then we unref it. This test will
 * probably only fail when run under valgrind if the reference counting is
 * off.
 */
static void
handle_map_entry_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (G_IS_OBJECT (data->handle_map_entry));
    assert_true (IS_HANDLE_MAP_ENTRY (data->handle_map_entry));
}
/*
 * This test assures us that the physical handle presented to the constructor
 * is the same one returned to us by the 'get_phandle' accessor function.
 */
static void
handle_map_entry_get_phandle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (PHANDLE,
                      handle_map_entry_get_phandle (data->handle_map_entry));
}
/*
 * This test assures us that the virtual handle presented to the constructor
 * is the same one returned to us by the 'get_vhandle' accessor function.
 */
static void
handle_map_entry_get_vhandle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (VHANDLE,
                      handle_map_entry_get_vhandle (data->handle_map_entry));
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (handle_map_entry_type_test,
                                         handle_map_entry_setup,
                                         handle_map_entry_teardown),
        cmocka_unit_test_setup_teardown (handle_map_entry_get_phandle_test,
                                         handle_map_entry_setup,
                                         handle_map_entry_teardown),
        cmocka_unit_test_setup_teardown (handle_map_entry_get_vhandle_test,
                                         handle_map_entry_setup,
                                         handle_map_entry_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
