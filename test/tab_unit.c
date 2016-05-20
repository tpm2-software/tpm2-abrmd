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
gint
main (gint   argc,
      gchar *argv[])
{
    const UnitTest tests[] = {
        unit_test (tab_allocate_deallocate_test),
    };
    return run_tests (tests);
}
