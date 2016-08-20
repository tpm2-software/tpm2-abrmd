#include <glib.h>
#include <stdlib.h>
#include <tpm20.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-echo.h"
#include "tab.h"

static void
tab_allocate_deallocate_test (void **state)
{
    Tcti *tcti;
    Tab  *tab;

    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    tab  = tab_new (tcti);
    g_object_unref (tab);
}
static void
tab_start_stop_test_setup (void **state)
{
    Tcti *tcti;
    Tab  *tab;

    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
    tab  = tab_new (tcti);
    *state = tab;
}
static void
tab_start_stop_test_teardown (void **state)
{
    Tab *tab = *state;

    tab_cancel (tab);
    tab_join (tab);
    g_object_unref (tab);
}
static void
tab_start_stop_test (void **state)
{
    Tab *tab = *state;
    tab_start (tab);
}
static void
tab_add_no_remove_test_setup (void **state)
{
    Tcti *tcti;
    Tab  *tab  = *state;

    tcti = TCTI (tcti_echo_new (TCTI_ECHO_MIN_BUF));
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
    Tab         *tab = *state;

    msg = data_message_new (NULL, NULL, 0);
    tab_send_command (tab, G_OBJECT (msg));
    g_object_unref (msg);
}
static void
tab_add_remove_test (void **state)
{
    Tab *tab = *state;
    DataMessage *msg_in = NULL, *msg_out = NULL;
    GObject *obj_out = NULL;
    char *data = malloc(5);

    msg_in = data_message_new (NULL, data, 5);
    tab_send_command (tab, G_OBJECT (msg_in));
    g_object_unref (msg_in);
    obj_out = tab_get_timeout_response (tab, 1e6);
    assert_int_equal (msg_in, obj_out);
    msg_out = DATA_MESSAGE (obj_out);
    assert_int_equal (msg_in, msg_out);
    g_object_unref (msg_out);
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
