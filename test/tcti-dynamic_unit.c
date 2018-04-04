/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "tcti-dynamic.h"
#include "tabrmd.h"

#define TCTI_DYNAMIC_UNIT_FILE_NAME "tss2-mytcti.so"
#define TCTI_DYNAMIC_UNIT_CONF_STR  "tss2-mytcti-conf-str"
#define TCTI_DYNAMIC_UNIT_HANDLE    (uintptr_t)0xcafebabecafe

#define TCTI_DYNAMIC_UNIT_INIT_1_FAIL_RC 0xdeadbeef
#define TCTI_DYNAMIC_UNIT_INIT_2_FAIL_RC 0xbeefdead
#define TCTI_DYNAMIC_UNIT_INIT_SIZE 10

void* __real_dlopen (const char *filename, int flags);
void*
__wrap_dlopen (const char *file_name,
               int flags)
{
    if (strcmp (file_name, TCTI_DYNAMIC_UNIT_FILE_NAME)) {
        return __real_dlopen (file_name, flags);
    }
    return mock_type (void*);
}

void* __real_dlsym(void *handle, const char *symbol);
void*
__wrap_dlsym (void *handle, const char *symbol)
{
    if ((uintptr_t)handle != TCTI_DYNAMIC_UNIT_HANDLE || strcmp (symbol, TSS2_TCTI_INFO_SYMBOL)) {
        return __real_dlsym (handle, symbol);
    }
    return mock_type (void*);
}

#if !defined (DISABLE_DLCLOSE)
int __real_dlclose(void *handle);
int
__wrap_dlclose (void *handle)
{
    if ((uintptr_t)handle != TCTI_DYNAMIC_UNIT_HANDLE) {
        return __real_dlclose (handle);
    }
    return mock_type (int);
}
#endif

static TSS2_TCTI_INFO*
tcti_dynamic_get_info_null (void)
{
    return NULL;
}
/*
 * This dummy structure is returned by the fake info function below. It is
 * a dummy value used in testing the `tcti_dynamic_discover_info` function.
 */
static TSS2_TCTI_INFO tcti_info_empty = {
    .init = NULL,
};
/*
 * This function is used when we mock the dlsym function. We cause dlsym to
 * return a reference to this function as a way to have the mock function
 * return a valid reference to an info function.
 */
static TSS2_TCTI_INFO*
tcti_dynamic_get_info_empty (void)
{
    return &tcti_info_empty;
}
/*
 * This is a fake TCTI initialization function that returns an error code
 * unique to this test module. This is how we test error handling around
 * the execution of the init function.
 */
static TSS2_RC
tcti_dynamic_init_fail (TSS2_TCTI_CONTEXT *context,
                        size_t *size,
                        const char *conf_str)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(size);
    UNUSED_PARAM(conf_str);
    return TCTI_DYNAMIC_UNIT_INIT_1_FAIL_RC;
}
/*
 * This is an info struct populated with an initialization function that
 * fails in a predictable way.
 */
static TSS2_TCTI_INFO tcti_info_init_1_fail = {
    .init = tcti_dynamic_init_fail,
};
/*
 * This function conforms to the TSS2_TCTI_INFO_FUNC prototype. It returns an
 * info structure populated with an init function that fails with an RC unique
 * to this test.
 */
static TSS2_TCTI_INFO*
tcti_dynamic_get_info_init_1_fail (void)
{
    return &tcti_info_init_1_fail;
}
/*
 * Just like the init_fail function above this init function will return an error
 * but only if the context provided is non-null. This is how we test error
 * handling around the second invocation of the init function.
 */
static TSS2_RC
tcti_dynamic_init_2_fail (TSS2_TCTI_CONTEXT *context,
                          size_t *size,
                          const char *conf_str)
{
    UNUSED_PARAM(conf_str);

    if (context == NULL) {
        *size = TCTI_DYNAMIC_UNIT_INIT_SIZE;
        return TSS2_RC_SUCCESS;
    } else {
        return TCTI_DYNAMIC_UNIT_INIT_2_FAIL_RC;
    }
}
static TSS2_TCTI_INFO tcti_info_init_2_fail = {
    .init = tcti_dynamic_init_2_fail,
};
static TSS2_TCTI_INFO*
tcti_dynamic_get_info_init_2_fail (void)
{
    return &tcti_info_init_2_fail;
}
/*
 * cmocka setup function. Just allocate the object and store in state.
 */
static int
tcti_dynamic_setup (void **state)
{
    *state = tcti_dynamic_new (TCTI_DYNAMIC_UNIT_FILE_NAME,
                               TCTI_DYNAMIC_UNIT_CONF_STR);
    return 0;
}
/*
 * cmocka teardown function. Unref the object.
 */
static int
tcti_dynamic_teardown (void **state)
{
    TctiDynamic *tcti_dynamic = TCTI_DYNAMIC (*state);

#if !defined (DISABLE_DLCLOSE)
    if (tcti_dynamic->tcti_dl_handle != NULL) {
        will_return (__wrap_dlclose, 0);
    }
#endif
    g_object_unref (tcti_dynamic);
    return 0;
}
/*
 * Test object life cycle: create new object then unref it.
 */
static void
tcti_dynamic_new_unref_test (void **state)
{
    assert_non_null (*state);
    assert_true (IS_TCTI (*state));
    assert_true (IS_TCTI_DYNAMIC (*state));
}
/*
 * Cause a failure in the tcti_dynamic_discover_info function by causing
 * dlsym to return a reference to a valid TCTI info function, but one that
 * returns a NULL info struct. This shouldn't happen and if it does the
 * TCTI is violating the spec but checking for it isn't a bad thing.
 */
static void
tcti_dynamic_initialize_bad_info_test (void **state)
{
    TctiDynamic *tcti_dynamic = TCTI_DYNAMIC (*state);
    TSS2_RC rc;

    will_return (__wrap_dlopen, TCTI_DYNAMIC_UNIT_HANDLE);
    will_return (__wrap_dlsym, tcti_dynamic_get_info_null);
    rc = tcti_dynamic_initialize (tcti_dynamic);
    assert_int_equal (rc, TSS2_RESMGR_RC_BAD_VALUE);
}
/*
 * This test exercises a test for a NULL init function pointer returned by
 * a TCTI modules 'info' function.
 */
static void
tcti_dynamic_initialize_null_init_test (void **state)
{
    TctiDynamic *tcti_dynamic = TCTI_DYNAMIC (*state);
    TSS2_RC rc;

    will_return (__wrap_dlopen, TCTI_DYNAMIC_UNIT_HANDLE);
    will_return (__wrap_dlsym, tcti_dynamic_get_info_empty);
    rc = tcti_dynamic_initialize (tcti_dynamic);
    assert_int_equal (rc, TSS2_RESMGR_RC_BAD_VALUE);
}
/*
 * This test causes the first call to the TCTI init function returned by
 * way of the INFO structure to fail with a known RC.
 */
static void
tcti_dynamic_initialize_init_1_fail_test (void **state)
{
    TctiDynamic *tcti_dynamic = TCTI_DYNAMIC (*state);
    TSS2_RC rc;

    will_return (__wrap_dlopen, TCTI_DYNAMIC_UNIT_HANDLE);
    will_return (__wrap_dlsym, tcti_dynamic_get_info_init_1_fail);
    rc = tcti_dynamic_initialize (tcti_dynamic);
    assert_int_equal (rc, TCTI_DYNAMIC_UNIT_INIT_1_FAIL_RC);
}
/*
 * This tests failure handling around the second invocation of the init function.
 */
static void
tcti_dynamic_initialize_init_2_fail_test (void **state)
{
    TctiDynamic *tcti_dynamic = TCTI_DYNAMIC (*state);
    TSS2_RC rc;

    will_return (__wrap_dlopen, TCTI_DYNAMIC_UNIT_HANDLE);
    will_return (__wrap_dlsym, tcti_dynamic_get_info_init_2_fail);
    rc = tcti_dynamic_initialize (tcti_dynamic);
    assert_int_equal (rc, TCTI_DYNAMIC_UNIT_INIT_2_FAIL_RC);
}
gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (tcti_dynamic_new_unref_test,
                                         tcti_dynamic_setup,
                                         tcti_dynamic_teardown),
        cmocka_unit_test_setup_teardown (tcti_dynamic_initialize_bad_info_test,
                                         tcti_dynamic_setup,
                                         tcti_dynamic_teardown),
        cmocka_unit_test_setup_teardown (tcti_dynamic_initialize_null_init_test,
                                         tcti_dynamic_setup,
                                         tcti_dynamic_teardown),
        cmocka_unit_test_setup_teardown (tcti_dynamic_initialize_init_1_fail_test,
                                         tcti_dynamic_setup,
                                         tcti_dynamic_teardown),
        cmocka_unit_test_setup_teardown (tcti_dynamic_initialize_init_2_fail_test,
                                         tcti_dynamic_setup,
                                         tcti_dynamic_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
