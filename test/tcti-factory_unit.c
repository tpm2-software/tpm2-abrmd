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

#define NAME "foo"
#define CONF "bar"

TSS2_RC
__wrap_tcti_util_discover_info (const char *filename,
                                const TSS2_TCTI_INFO **info,
                                void **tcti_dl_handle)
{
    UNUSED_PARAM (filename);
    *info = mock_type (const TSS2_TCTI_INFO*);
    *tcti_dl_handle = mock_type (void *);
    return mock_type (TSS2_RC);
}

TSS2_RC
__wrap_tcti_util_dynamic_init (const TSS2_TCTI_INFO *info,
                               const char *conf,
                               TSS2_TCTI_CONTEXT **context)
{
    UNUSED_PARAM (info);
    UNUSED_PARAM (conf);
    *context = mock_type (TSS2_TCTI_CONTEXT*);
    return mock_type (TSS2_RC);
}

#define TCTI_UTIL_UNIT_HANDLE    (uintptr_t)0xcafebabecafe
#if !defined (DISABLE_DLCLOSE)
int __real_dlclose(void *handle);
int
__wrap_dlclose (void *handle)
{
    if ((uintptr_t)handle != TCTI_UTIL_UNIT_HANDLE) {
        return __real_dlclose (handle);
    }
    return mock_type (int);
}
#endif

static int
tcti_factory_setup (void **state)
{
    *state = tcti_factory_new (NAME, CONF);

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
tcti_factory_create_discover_info_fail (void **state)
{
    TctiFactory *factory = TCTI_FACTORY (*state);
    Tcti *tcti;
    TSS2_TCTI_INFO info = { 0 };

    will_return (__wrap_tcti_util_discover_info, &info);
    will_return (__wrap_tcti_util_discover_info, TCTI_UTIL_UNIT_HANDLE);
    will_return (__wrap_tcti_util_discover_info, 32);
    tcti = tcti_factory_create (factory);
    assert_null (tcti);
}

static void
tcti_factory_create_dynamic_init_fail (void **state)
{
    TctiFactory *factory = TCTI_FACTORY (*state);
    Tcti *tcti = NULL;
    TSS2_TCTI_INFO info = { 0 };

    will_return (__wrap_tcti_util_discover_info, &info);
    will_return (__wrap_tcti_util_discover_info, TCTI_UTIL_UNIT_HANDLE);
    will_return (__wrap_tcti_util_discover_info, TSS2_RC_SUCCESS);
    will_return (__wrap_tcti_util_dynamic_init, NULL);
    will_return (__wrap_tcti_util_dynamic_init, 32);
#if !defined (DISABLE_DLCLOSE)
    will_return (__wrap_dlclose, 0);
#endif

    tcti = tcti_factory_create (factory);
    assert_null (tcti);
}

static void
tcti_factory_create_test (void **state)
{
    TctiFactory *factory = TCTI_FACTORY (*state);
    Tcti *tcti = NULL;
    TSS2_TCTI_INFO info = { 0 };
    TSS2_TCTI_CONTEXT *tcti_ctx = NULL;

    tcti_ctx = tcti_mock_init_full ();
    will_return (__wrap_tcti_util_discover_info, &info);
    will_return (__wrap_tcti_util_discover_info, TCTI_UTIL_UNIT_HANDLE);
    will_return (__wrap_tcti_util_discover_info, TSS2_RC_SUCCESS);
    will_return (__wrap_tcti_util_dynamic_init, tcti_ctx);
    will_return (__wrap_tcti_util_dynamic_init, TSS2_RC_SUCCESS);
#if !defined (DISABLE_DLCLOSE)
    will_return (__wrap_dlclose, 0);
#endif

    tcti = tcti_factory_create (factory);
    assert_non_null (tcti);
    g_clear_object (&tcti);
}
gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tcti_factory_type_test,
                                         tcti_factory_setup,
                                         tcti_factory_teardown),
        cmocka_unit_test_setup_teardown (tcti_factory_create_discover_info_fail,
                                         tcti_factory_setup,
                                         tcti_factory_teardown),
        cmocka_unit_test_setup_teardown (tcti_factory_create_dynamic_init_fail,
                                         tcti_factory_setup,
                                         tcti_factory_teardown),
        cmocka_unit_test_setup_teardown (tcti_factory_create_test,
                                         tcti_factory_setup,
                                         tcti_factory_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
