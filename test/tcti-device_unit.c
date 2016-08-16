#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-device.h"

static void
tcti_device_new_unref_test (void **state)
{
    TctiDevice *tcti_dev = NULL;

    tcti_dev = tcti_device_new ("test");
    assert_non_null (tcti_dev);
    g_object_unref (tcti_dev);
}

gint
main (gint     argc,
      gchar   *argv[])
{
    const UnitTest tests[] = {
        unit_test (tcti_device_new_unref_test),
    };
    return run_tests (tests);
}
