/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cmocka.h>

#include "handle-map.h"
#include "handle-map-entry.h"

#define PHANDLE 0xdeadbeef
#define VHANDLE 0xfeebdaed

typedef struct {
    HandleMap         *map;
    HandleMapEntry    *entry;
} test_data_t;
/*
 * Setup and teardown functions.
 */
static int
handle_map_setup_base (void **state)
{
    test_data_t *data = NULL;

    data = calloc (1, sizeof (test_data_t));
    data->map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    data->entry = handle_map_entry_new (PHANDLE, VHANDLE);

    *state = data;
    return 0;
}
static int
handle_map_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    if (data->map)
        g_object_unref (data->map);
    if (data->entry)
        g_object_unref (data->entry);
    free (data);
    return 0;
}
static int
handle_map_setup_with_entry (void **state)
{
    test_data_t *data = NULL;

    handle_map_setup_base (state);
    data = (test_data_t*)*state;
    handle_map_insert (data->map,
                       handle_map_entry_get_vhandle (data->entry),
                       data->entry);
    return 0;
}
/*
 * This test ensures that the object instances created in the setup function
 * is a valid GObject and is a HandleMap to boot.
 */
static void
handle_map_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_non_null (data);
    assert_non_null (data->entry);
    assert_non_null (data->map);
    assert_true (G_IS_OBJECT (data->map));
    assert_true (IS_HANDLE_MAP (data->map));
}
/*
 * This test ensures that the HandleMapEntry created in the setup function
 * can be inserted into the HandleMap. We prove to ourselves that this was
 * successful by asserting the size is 0 before the call, and 1 after.
 */
static void
handle_map_insert_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (handle_map_size (data->map), 0);
    handle_map_insert (data->map,
                       VHANDLE,
                       data->entry);
    assert_int_equal (handle_map_size (data->map), 1);
}
/*
 * This test ensures that we can remove a known entry from the map. The
 * EntryMap must already have an entry and it must be keyed to the
 * physical handle.
 */
static void
handle_map_remove_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    gint ret;

    ret = handle_map_remove (data->map, VHANDLE);
    assert_int_equal (ret, TRUE);
    assert_int_equal (0, handle_map_size (data->map));
}
/*
 * This test ensures that we can lookup a known entry from the map. The
 * EntryMap must already have an entry and it must be keyed to the virtual
 * handle.
 */
static void
handle_map_vlookup_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    HandleMapEntry *entry_out;

    entry_out = HANDLE_MAP_ENTRY (handle_map_vlookup (data->map, VHANDLE));
    assert_int_equal (data->entry, entry_out);
    g_object_unref (entry_out);
}
/*
 * This test ensures that the 'handle_map_next_vhandle' function returns
 * a different handle across two invocations. This is the lowest bar we can
 * set for this function. Additional tests should cover roll over and
 * recovery.
 */
static void
handle_map_next_vhandle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM2_HANDLE handle1, handle2;

    handle1 = handle_map_next_vhandle (data->map);
    handle2 = handle_map_next_vhandle (data->map);
    assert_true (handle2 != handle1);
}
int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (handle_map_type_test,
                                         handle_map_setup_base,
                                         handle_map_teardown),
        cmocka_unit_test_setup_teardown (handle_map_insert_test,
                                         handle_map_setup_base,
                                         handle_map_teardown),
        cmocka_unit_test_setup_teardown (handle_map_remove_test,
                                         handle_map_setup_with_entry,
                                         handle_map_teardown),
        cmocka_unit_test_setup_teardown (handle_map_vlookup_test,
                                         handle_map_setup_with_entry,
                                         handle_map_teardown),
        cmocka_unit_test_setup_teardown (handle_map_next_vhandle_test,
                                         handle_map_setup_with_entry,
                                         handle_map_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
