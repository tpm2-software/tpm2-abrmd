#include <glib.h>
#include <stdlib.h>
#include <tpm20.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tab.h"

static void
tab_allocate_deallocate_test (void **state)
{
    TSS2_TCTI_CONTEXT *tcti = NULL;
    tab_t *tab;
    tab = tab_new (tcti);
    tab_free (tab);
}
static void
tab_start_stop_test_setup (void **state)
{
    TSS2_TCTI_CONTEXT *tcti = NULL;
    tab_t *tab;

    tab = tab_new (tcti);
    *state = tab;
}
static void
tab_start_stop_test_teardown (void **state)
{
    tab_t *tab = *state;

    tab_cancel (tab);
    tab_join (tab);
    tab_free (tab);
}
static void
tab_start_stop_test (void **state)
{
    tab_t *tab = *state;
    tab_start (tab);
}
static void
tab_add_no_remove_test_setup (void **state)
{
    TSS2_TCTI_CONTEXT *tcti = NULL;
    tab_t *tab = *state;

    tab = tab_new (tcti);
    tab_start (tab);
    *state = tab;
}
/* This test adds a message to the tab but doesn't remove it.
 * The tab is expected to free this message as part of its destruction.
 */
static void
tab_add_no_remove_test (void **state)
{
    DataMessage *msg = NULL;
    tab_t *tab = *state;

    msg = data_message_new (NULL, NULL, 0);
    tab_send_command (tab, G_OBJECT (msg));
}
static void
tab_add_remove_test (void **state)
{
    tab_t *tab = *state;
    DataMessage *msg_in = NULL, *msg_out = NULL;
    GObject *obj_out = NULL;

    msg_in = data_message_new (NULL, NULL, 0);
    g_print ("tab_send_command on tab 0x%x, msg 0x%x", tab, msg_in);
    tab_send_command (tab, G_OBJECT (msg_in));
    obj_out = tab_get_timeout_response (tab, 1e6);
    assert_int_equal (msg_in, obj_out);
    msg_out = DATA_MESSAGE (obj_out);
    assert_int_equal (msg_in, msg_out);
    g_object_unref (msg_in);
}

gint
main (gint   argc,
      gchar *argv[])
{
    const UnitTest tests[] = {
        unit_test (tab_allocate_deallocate_test),
        unit_test_setup_teardown (tab_start_stop_test,
                                  tab_start_stop_test_setup,
                                  tab_start_stop_test_teardown),
        unit_test_setup_teardown (tab_add_no_remove_test,
                                  tab_add_no_remove_test_setup,
                                  tab_start_stop_test_teardown),
        unit_test_setup_teardown (tab_add_remove_test,
                                  tab_add_no_remove_test_setup,
                                  tab_start_stop_test_teardown),
    };
    return run_tests (tests);
}
