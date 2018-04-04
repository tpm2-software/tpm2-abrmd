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
#include <dlfcn.h>
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "tcti-util.h"
#include "tabrmd.h"

#define TCTI_UTIL_UNIT_FILE_NAME "tss2-mytcti.so"
#define TCTI_UTIL_UNIT_CONF_STR  "tss2-mytcti-conf-str"
#define TCTI_UTIL_UNIT_HANDLE    (uintptr_t)0xcafebabecafe

#define TCTI_UTIL_UNIT_INIT_1_FAIL_RC 0xdeadbeef
#define TCTI_UTIL_UNIT_INIT_2_FAIL_RC 0xbeefdead
#define TCTI_UTIL_UNIT_INIT_SIZE 10

void* __real_dlopen (const char *filename, int flags);
void*
__wrap_dlopen (const char *file_name,
               int flags)
{
    if (strcmp (file_name, TCTI_UTIL_UNIT_FILE_NAME)) {
        return __real_dlopen (file_name, flags);
    }
    return mock_type (void*);
}

void* __real_dlsym(void *handle, const char *symbol);
void*
__wrap_dlsym (void *handle, const char *symbol)
{
    if ((uintptr_t)handle != TCTI_UTIL_UNIT_HANDLE || strcmp (symbol, TSS2_TCTI_INFO_SYMBOL)) {
        return __real_dlsym (handle, symbol);
    }
    return mock_type (void*);
}

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

static TSS2_TCTI_INFO*
tcti_util_get_info_null (void)
{
    return NULL;
}
/*
 * This dummy structure is returned by the fake info function below. It is
 * a dummy value used in testing the `tcti_util_discover_info` function.
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
tcti_util_get_info_empty (void)
{
    return &tcti_info_empty;
}
/*
 * This is a fake TCTI initialization function that returns an error code
 * unique to this test module. This is how we test error handling around
 * the execution of the init function.
 */
static TSS2_RC
tcti_util_init_fail (TSS2_TCTI_CONTEXT *context,
                      size_t *size,
                      const char *conf_str)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(size);
    UNUSED_PARAM(conf_str);
    return TCTI_UTIL_UNIT_INIT_1_FAIL_RC;
}
/*
 * This is an info struct populated with an initialization function that
 * fails in a predictable way.
 */
static TSS2_TCTI_INFO tcti_info_init_1_fail = {
    .init = tcti_util_init_fail,
};
/*
 * This function conforms to the TSS2_TCTI_INFO_FUNC prototype. It returns an
 * info structure populated with an init function that fails with an RC unique
 * to this test.
 */
static TSS2_TCTI_INFO*
tcti_util_get_info_init_1_fail (void)
{
    return &tcti_info_init_1_fail;
}
/*
 * Just like the init_fail function above this init function will return an error
 * but only if the context provided is non-null. This is how we test error
 * handling around the second invocation of the init function.
 */
static TSS2_RC
tcti_util_init_2_fail (TSS2_TCTI_CONTEXT *context,
                       size_t *size,
                       const char *conf_str)
{
    UNUSED_PARAM(conf_str);
    if (context == NULL) {
        *size = TCTI_UTIL_UNIT_INIT_SIZE;
        return TSS2_RC_SUCCESS;
    } else {
        return TCTI_UTIL_UNIT_INIT_2_FAIL_RC;
    }
}
static TSS2_TCTI_INFO tcti_info_init_2_fail = {
    .init = tcti_util_init_2_fail,
};
static TSS2_TCTI_INFO*
tcti_util_get_info_init_2_fail (void)
{
    return &tcti_info_init_2_fail;
}
/*
 * Cause a failure (NULL context) to be returned by dlopen while the
 * TctiDynamic is trying to load the requested shared object.
 */
static void
tcti_util_discover_info_dlopen_fail_test (void **state)
{
    void *tcti_dl_handle;
    const TSS2_TCTI_INFO *info;
    TSS2_RC rc;
    UNUSED_PARAM(state);

    will_return (__wrap_dlopen, NULL);
    rc = tcti_util_discover_info (TCTI_UTIL_UNIT_FILE_NAME,
                                  &info,
                                  &tcti_dl_handle);
    assert_int_equal (rc, TSS2_RESMGR_RC_BAD_VALUE);
}
/*
 * Cause a failure in the tcti_util_discover_info function by causing the
 * dlsym function to return a NULL pointer.
 */
static void
tcti_util_discover_info_dlsym_fail_test (void **state)
{
    void *tcti_dl_handle;
    const TSS2_TCTI_INFO *info;
    TSS2_RC rc;
    UNUSED_PARAM(state);

    will_return (__wrap_dlopen, TCTI_UTIL_UNIT_HANDLE);
    will_return (__wrap_dlsym, NULL);
#if !defined (DISABLE_DLCLOSE)
    will_return (__wrap_dlclose, 0);
#endif
    rc = tcti_util_discover_info (TCTI_UTIL_UNIT_FILE_NAME,
                                  &info,
                                  &tcti_dl_handle);
    assert_int_equal (rc, TSS2_RESMGR_RC_BAD_VALUE);
}
/*
 * Cause the util_tcti_discover_info function to execute successfully.
 * This requires that we:
 * 1) mock a successful call to dlopen
 * 2) mock a call to dlsym such that it returns a fake TCTI info function that
 *    will return a valid INFO structure.
 * 3) mock a successful call to dlclose
 * 4) check the return type etc
 */
static void
tcti_util_discover_info_func_success_test (void **state)
{
    void *tcti_dl_handle;
    const TSS2_TCTI_INFO *info;
    TSS2_RC rc;
    UNUSED_PARAM(state);

    will_return (__wrap_dlopen, TCTI_UTIL_UNIT_HANDLE);
    will_return (__wrap_dlsym, tcti_util_get_info_empty);
    rc = tcti_util_discover_info (TCTI_UTIL_UNIT_FILE_NAME,
                                  &info,
                                  &tcti_dl_handle);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_ptr_equal (info, &tcti_info_empty);
}
typedef struct {
    void *tcti_dl_handle;
    const TSS2_TCTI_INFO *info;
    TSS2_TCTI_CONTEXT *context;
} tcti_util_t;

/*
 * This setup function shouldn't be used directly. Instead it should be
 * called from another setup function after the dlsym stack has been
 * set so there's a function to return the info structure to the
 * discover_info function.
 */
static int
tcti_util_init_setup (void **state)
{
    tcti_util_t *data;
    TSS2_RC rc;

    data = calloc (1, sizeof (tcti_util_t));
    will_return (__wrap_dlopen, TCTI_UTIL_UNIT_HANDLE);
    rc = tcti_util_discover_info (TCTI_UTIL_UNIT_FILE_NAME,
                                  &data->info,
                                  &data->tcti_dl_handle);
    if (rc == TSS2_RC_SUCCESS) {
        *state = data;
        return 0;
    } else {
        return 1;
    }
}
static int
tcti_util_init_teardown (void **state)
{
    tcti_util_t *data = (tcti_util_t*)*state;
    free (data);
    return 0;
}
/*
 * Cause a failure in the tcti_util_init function by causing
 * dlsym to return a reference to a valid TCTI info function, but one that
 * returns a NULL info struct. This shouldn't happen and if it does the
 * TCTI is violating the spec but checking for it isn't a bad thing.
 */
static int
tcti_util_init_setup_info_null (void **state)
{
    will_return (__wrap_dlsym, tcti_util_get_info_null);
    return tcti_util_init_setup (state);
}
static void
tcti_util_initialize_info_null_test (void **state)
{
    tcti_util_t *data = (tcti_util_t*)*state;
    TSS2_RC rc;

    rc = tcti_util_dynamic_init (data->info,
                                 TCTI_UTIL_UNIT_CONF_STR,
                                 &data->context);
    assert_int_equal (rc, TSS2_RESMGR_RC_BAD_VALUE);
}
/*
 * This test exercises a test for a NULL init function pointer returned by
 * a TCTI modules 'info' function.
 */
static int
tcti_util_init_setup_null_init (void **state)
{
    will_return (__wrap_dlsym, tcti_util_get_info_empty);
    return tcti_util_init_setup (state);
}
static void
tcti_util_initialize_null_init_test (void **state)
{
    tcti_util_t *data = (tcti_util_t*)*state;
    TSS2_RC rc;

    rc = tcti_util_dynamic_init (data->info,
                                 TCTI_UTIL_UNIT_CONF_STR,
                                 &data->context);
    assert_int_equal (rc, TSS2_RESMGR_RC_BAD_VALUE);
}
/*
 * This test causes the first call to the TCTI init function returned by
 * way of the INFO structure to fail with a known RC.
 */
static int
tcti_util_init_setup_init_1_fail (void **state)
{
    will_return (__wrap_dlsym, tcti_util_get_info_init_1_fail);
    return tcti_util_init_setup (state);
}
static void
tcti_util_initialize_init_1_fail_test (void **state)
{
    tcti_util_t *data = (tcti_util_t*)*state;
    TSS2_RC rc;

    rc = tcti_util_dynamic_init (data->info,
                                 TCTI_UTIL_UNIT_CONF_STR,
                                 &data->context);
    assert_int_equal (rc, TCTI_UTIL_UNIT_INIT_1_FAIL_RC);
}
/*
 * This tests failure handling around the second invocation of the init function.
 */
static int
tcti_util_init_setup_init_2_fail (void **state)
{
    will_return (__wrap_dlsym, tcti_util_get_info_init_2_fail);
    return tcti_util_init_setup (state);
}
static void
tcti_util_initialize_init_2_fail_test (void **state)
{
    tcti_util_t *data = (tcti_util_t*)*state;
    TSS2_RC rc;

    rc = tcti_util_dynamic_init (data->info,
                                 TCTI_UTIL_UNIT_CONF_STR,
                                 &data->context);
    assert_int_equal (rc, TCTI_UTIL_UNIT_INIT_2_FAIL_RC);
}
gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (tcti_util_discover_info_dlopen_fail_test),
        cmocka_unit_test (tcti_util_discover_info_dlsym_fail_test),
        cmocka_unit_test (tcti_util_discover_info_func_success_test),
        cmocka_unit_test_setup_teardown (tcti_util_initialize_info_null_test,
                                         tcti_util_init_setup_info_null,
                                         tcti_util_init_teardown),
        cmocka_unit_test_setup_teardown (tcti_util_initialize_null_init_test,
                                         tcti_util_init_setup_null_init,
                                         tcti_util_init_teardown),
        cmocka_unit_test_setup_teardown (tcti_util_initialize_init_1_fail_test,
                                         tcti_util_init_setup_init_1_fail,
                                         tcti_util_init_teardown),
        cmocka_unit_test_setup_teardown (tcti_util_initialize_init_2_fail_test,
                                         tcti_util_init_setup_init_2_fail,
                                         tcti_util_init_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
