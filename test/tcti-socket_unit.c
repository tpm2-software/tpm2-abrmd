#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-socket.h"

static void
tcti_socket_new_unref_test (void **state)
{
    TctiSocket *tcti_sock = NULL;

    tcti_sock = tcti_socket_new ("test", 10);
    assert_non_null (tcti_sock);
    g_object_unref (tcti_sock);
}

gint
main (gint     argc,
      gchar   *argv[])
{
    const UnitTest tests[] = {
        unit_test (tcti_socket_new_unref_test),
    };
    return run_tests (tests);
}
