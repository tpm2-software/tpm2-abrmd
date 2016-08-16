#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-options.h"

static void
tcti_options_new_unref_test (void **state)
{
    TctiOptions *tcti_opts = NULL;

    tcti_opts = tcti_options_new ();
    assert_non_null (tcti_opts);
    g_object_unref (tcti_opts);
}

gint
main (gint     argc,
      gchar   *argv[])
{
    const UnitTest tests[] = {
        unit_test (tcti_options_new_unref_test),
    };
    return run_tests (tests);
}
