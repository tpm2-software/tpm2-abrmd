#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#ifdef HAVE_TCTI_DEVICE
#include "tcti-device.h"
#endif
#ifdef HAVE_TCTI_SOCKET
#include "tcti-socket.h"
#endif
#include "tcti-options.h"

/**
 * Very simple setup / teardown functions to instantiate a TctiOptions
 * object and unref it.
 */
static void
tcti_options_setup (void **state)
{
    *state = tcti_options_new ();
}
static void
tcti_options_teardown (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    g_object_unref (tcti_options);
}
/**
 * Test the object lifecycle. The setup /teardown functions must be invoked
 * for this to be a valid test. This is mostly useful when combined with
 * valgrind.
 */
static void
tcti_options_new_unref_test (void **state)
{
    TctiOptions *tcti_opts = TCTI_OPTIONS (*state);

    assert_non_null (tcti_opts);
}
/**
 * Tests to ensure that the default values for the TCTI specific
 * configuration options are set properly. These test are conditional
 * and should only be invoked if the TCTI is enalbed by ./configure.
 */
#ifdef HAVE_TCTI_DEVICE
static void
tcti_options_defaults_device_test (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    assert_string_equal (tcti_options->device_name, TCTI_DEVICE_DEFAULT_FILE);
}
#endif
#ifdef HAVE_TCTI_SOCKET
static void
tcti_options_defaults_socket_address_test (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    assert_string_equal (tcti_options->socket_address,
                         TCTI_SOCKET_DEFAULT_HOST);
}
static void
tcti_options_defaults_socket_port_test (void **state)
{
    TctiOptions *tcti_options = TCTI_OPTIONS (*state);

    assert_int_equal (tcti_options->socket_port,
                      TCTI_SOCKET_DEFAULT_PORT);
}
#endif

gint
main (gint     argc,
      gchar   *argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (tcti_options_new_unref_test,
                                  tcti_options_setup,
                                  tcti_options_teardown),
#ifdef HAVE_TCTI_DEVICE
        unit_test_setup_teardown (tcti_options_defaults_device_test,
                                  tcti_options_setup,
                                  tcti_options_teardown),
#endif
#ifdef HAVE_TCTI_SOCKET
        unit_test_setup_teardown (tcti_options_defaults_socket_address_test,
                                  tcti_options_setup,
                                  tcti_options_teardown),
        unit_test_setup_teardown (tcti_options_defaults_socket_port_test,
                                  tcti_options_setup,
                                  tcti_options_teardown),
#endif
    };
    return run_tests (tests);
}
