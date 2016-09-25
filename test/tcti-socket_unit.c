#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-socket.h"

TSS2_RC
__wrap_InitSocketTcti (TSS2_TCTI_CONTEXT *tcti_context,
                       size_t *size,
                       TCTI_SOCKET_CONF *config,
                       uint8_t serverSockets)
{
    *size = (size_t) mock ();
    return (TSS2_RC) mock ();
}

static void
tcti_socket_setup (void **state)
{
    *state = tcti_socket_new ("test", 10);
}

static void
tcti_socket_teardown (void **state)
{
    TctiSocket *tcti_sock = *state;

    g_object_unref (tcti_sock);
}
/**
 * Test object life cycle: create new object then unref it.
 */
static void
tcti_socket_new_unref_test (void **state)
{
    TctiSocket *tcti_sock;

    tcti_socket_setup ((void**)&tcti_sock);
    tcti_socket_teardown ((void**)&tcti_sock);
}

/**
 * Calling the initialize function causes two calls to InitSocketTcti. The
 * first gets the size of the context structure to allocate, the second
 * does the initialization. Inbetween the function allocates the context.
 */
static void
tcti_socket_initialize_success_unit (void **state)
{
    Tcti       *tcti      = *state;
    TctiSocket *tcti_sock = *state;
    TSS2_RC     rc = TSS2_RC_SUCCESS;

    will_return (__wrap_InitSocketTcti, 512);
    will_return (__wrap_InitSocketTcti, TSS2_RC_SUCCESS);

    will_return (__wrap_InitSocketTcti, 512);
    will_return (__wrap_InitSocketTcti, TSS2_RC_SUCCESS);

    rc = tcti_socket_initialize (tcti);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
}
/**
 * Same as the previous case but this time we use the interface function.
 */
static void
tcti_socket_initialize_success_interface_unit (void **state)
{
    Tcti       *tcti      = *state;
    TctiSocket *tcti_sock = *state;
    TSS2_RC     rc = TSS2_RC_SUCCESS;

    will_return (__wrap_InitSocketTcti, 512);
    will_return (__wrap_InitSocketTcti, TSS2_RC_SUCCESS);

    will_return (__wrap_InitSocketTcti, 512);
    will_return (__wrap_InitSocketTcti, TSS2_RC_SUCCESS);

    rc = tcti_initialize (tcti);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
}
/**
 * Cause first call to InitSocketTcti in tcti_socket_initialize to fail.
 * We should get the RC that we provide in the second will_return sent
 * back to us.
 */
static void
tcti_socket_initialize_fail_on_first_init_unit (void **state)
{
    Tcti       *tcti      = *state;
    TctiSocket *tcti_sock = *state;
    TSS2_RC     rc        = TSS2_RC_SUCCESS;

    will_return (__wrap_InitSocketTcti, 0);
    will_return (__wrap_InitSocketTcti, TSS2_TCTI_RC_GENERAL_FAILURE);

    rc = tcti_socket_initialize (tcti);
    assert_int_equal (rc, TSS2_TCTI_RC_GENERAL_FAILURE);
}
/**
 * Cause the second call to InitSocketTcti in the tcti_socket_initialize to
 * fail. We should get the RC taht we provide in the 4th will_return sent
 * back to us.
 */
static void
tcti_socket_initialize_fail_on_second_init_unit (void **state)
{
    Tcti       *tcti      = *state;
    TctiSocket *tcti_sock = *state;
    TSS2_RC     rc = TSS2_RC_SUCCESS;

    will_return (__wrap_InitSocketTcti, 512);
    will_return (__wrap_InitSocketTcti, TSS2_RC_SUCCESS);

    will_return (__wrap_InitSocketTcti, 0);
    will_return (__wrap_InitSocketTcti, TSS2_TCTI_RC_GENERAL_FAILURE);

    rc = tcti_socket_initialize (tcti);
    assert_int_equal (rc, TSS2_TCTI_RC_GENERAL_FAILURE);
}
gint
main (gint     argc,
      gchar   *argv[])
{
    const UnitTest tests[] = {
        unit_test (tcti_socket_new_unref_test),
        unit_test_setup_teardown (tcti_socket_initialize_success_unit,
                                  tcti_socket_setup,
                                  tcti_socket_teardown),
        unit_test_setup_teardown (tcti_socket_initialize_success_interface_unit,
                                  tcti_socket_setup,
                                  tcti_socket_teardown),
        unit_test_setup_teardown (tcti_socket_initialize_fail_on_first_init_unit,
                                  tcti_socket_setup,
                                  tcti_socket_teardown),
        unit_test_setup_teardown (tcti_socket_initialize_fail_on_second_init_unit,
                                  tcti_socket_setup,
                                  tcti_socket_teardown),
    };
    return run_tests (tests);
}
