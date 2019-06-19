/* SPDX-License-Identifier: BSD-2-Clause */
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tcti-mock.h"
#include "tcti-factory.h"
#include "util.h"

#define NAME_CONF "foo:bar"
TSS2_TCTI_CONTEXT_COMMON_V2 v2_ctx = { 0, };

TSS2_RC
__wrap_Tss2_TctiLdr_Initialize (char *name_conf,
                                TSS2_TCTI_CONTEXT **tcti_ctx)
{
    TSS2_RC rc = mock_type (TSS2_RC);
    UNUSED_PARAM (name_conf);
    assert_non_null (tcti_ctx);
    if (rc == TSS2_RC_SUCCESS)
        *tcti_ctx = mock_type (TSS2_TCTI_CONTEXT*);
    return rc;
}
void
__wrap_Tss2_TctiLdr_Finalize (TSS2_TCTI_CONTEXT **tcti_ctx)
{
    UNUSED_PARAM (tcti_ctx);
}

static int
tcti_factory_setup (void **state)
{
    *state = tcti_factory_new (NAME_CONF);

    return 0;
}

static int
tcti_factory_teardown (void **state)
{
    TctiFactory *factory = TCTI_FACTORY (*state);

    g_clear_object (&factory);
    return 0;
}

static void
tcti_factory_type_test (void **state)
{
    assert_true (IS_TCTI_FACTORY (*state));
}

static void
tcti_factory_create_fail (void **state)
{
    TctiFactory *factory = TCTI_FACTORY (*state);
    Tcti *tcti;

    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_TCTI_RC_BAD_VALUE);
    tcti = tcti_factory_create (factory);
    assert_null (tcti);
}

static void
tcti_factory_create_success (void **state)
{
    TctiFactory *factory = TCTI_FACTORY (*state);
    Tcti *tcti;

    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_TctiLdr_Initialize, &v2_ctx);
    tcti = tcti_factory_create (factory);
    assert_non_null (tcti);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tcti_factory_type_test,
                                         tcti_factory_setup,
                                         tcti_factory_teardown),
        cmocka_unit_test_setup_teardown (tcti_factory_create_fail,
                                         tcti_factory_setup,
                                         tcti_factory_teardown),
        cmocka_unit_test_setup_teardown (tcti_factory_create_success,
                                         tcti_factory_setup,
                                         tcti_factory_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
