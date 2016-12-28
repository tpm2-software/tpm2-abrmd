#include <stdio.h>

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
static void
handle_map_setup_base (void **state)
{
    test_data_t *data = NULL;

    data = calloc (1, sizeof (test_data_t));
    data->map = handle_map_new ();
    data->entry = handle_map_entry_new (PHANDLE, VHANDLE);

    *state = data;
}
static void
handle_map_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    if (data->map)
        g_object_unref (data->map);
    if (data->entry)
        g_object_unref (data->entry);
    free (data);
}
static void
handle_map_setup_with_entry (void **state)
{
    test_data_t *data = NULL;

    handle_map_setup_base (state);
    data = (test_data_t*)*state;
    handle_map_insert (data->map,
                       handle_map_entry_get_phandle (data->entry),
                       G_OBJECT (data->entry));
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
                       PHANDLE,
                       G_OBJECT (data->entry));
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

    ret = handle_map_remove (data->map, PHANDLE);
    assert_int_equal (ret, TRUE);
    assert_int_equal (0, handle_map_size (data->map));
}
/*
 * This test ensures that we can lookup a known entry from the map. The
 * EntryMap must already have an entry and it must be keyed to the physical
 * handle.
 */
static void
handle_map_lookup_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    HandleMapEntry *entry_out;

    entry_out = HANDLE_MAP_ENTRY (handle_map_lookup (data->map, PHANDLE));
    assert_int_equal (data->entry, entry_out);
    g_object_unref (entry_out);
}
int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (handle_map_type_test,
                                  handle_map_setup_base,
                                  handle_map_teardown),
        unit_test_setup_teardown (handle_map_insert_test,
                                  handle_map_setup_base,
                                  handle_map_teardown),
        unit_test_setup_teardown (handle_map_remove_test,
                                  handle_map_setup_with_entry,
                                  handle_map_teardown),
        unit_test_setup_teardown (handle_map_lookup_test,
                                  handle_map_setup_with_entry,
                                  handle_map_teardown),
    };
    return run_tests(tests);
}
