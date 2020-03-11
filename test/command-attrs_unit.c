/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"
#include "tpm2.h"
#include "command-attrs.h"
#include "tcti.h"
#include "tcti-mock.h"

typedef struct test_data {
    Tpm2 *tpm2;
    CommandAttrs *command_attrs;
} test_data_t;

/* Setup function to allocate our Random gobject. */
static int
command_attrs_setup (void **state)
{
    test_data_t *data;
    TSS2_TCTI_CONTEXT *context;
    Tcti *tcti = NULL;

    context = tcti_mock_init_full ();
    if (context == NULL) {
        g_critical ("tcti_mock_init_full failed");
        return 1;
    }
    data = calloc (1, sizeof (test_data_t));
    tcti = tcti_new (context);
    data->tpm2 = tpm2_new (tcti);
    data->command_attrs = command_attrs_new ();
    g_clear_object (&tcti);

    *state = data;
    return 0;
}
/* Setup function to allocate and initialize the object. */
static int
command_attrs_init_tpm_setup (void **state)
{
    test_data_t *data;
    gint         ret;
    TPMA_CC      hierarchy_attrs  = TPM2_CC_HierarchyControl + 0xff0000;
    TPMA_CC      change_pps_attrs = TPM2_CC_ChangePPS + 0xff0000;
    TPMA_CC      command_attributes [2] = { hierarchy_attrs, change_pps_attrs };

    command_attrs_setup (state);
    data = *state;
    will_return (__wrap_tpm2_get_command_attrs, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_get_command_attrs, 2);
    will_return (__wrap_tpm2_get_command_attrs, command_attributes);

    ret = command_attrs_init_tpm (data->command_attrs, data->tpm2);
    assert_int_equal (ret, 0);
    return 0;
}
/* Teardown function to deallocate the Random object. */
static int
command_attrs_teardown (void **state)
{
    test_data_t *data = *state;

    g_object_unref (data->command_attrs);
    free (data);
    return 0;
}
/* Simple test to test type checking macros. */
static void
command_attrs_type_test (void **state)
{
    test_data_t *data = *state;

    assert_true (G_IS_OBJECT (data->command_attrs));
    assert_true (IS_COMMAND_ATTRS (data->command_attrs));
}

/* */
TSS2_RC
__wrap_tpm2_get_command_attrs (Tpm2 *tpm2,
                                        UINT32 *count,
                                        TPMA_CC **attrs)
{
    TSS2_RC rc;
    TPMA_CC *command_attrs;
    UNUSED_PARAM(tpm2);

    rc = mock_type (TSS2_RC);
    if (rc != TSS2_RC_SUCCESS)
        return rc;

    *count = mock_type (UINT32);
    command_attrs = mock_ptr_type (TPMA_CC*);
    *attrs = g_malloc0 (*count * sizeof (TPMA_CC));
    memcpy (*attrs, command_attrs, *count * sizeof (TPMA_CC));

    return rc;
}
/*
 * Test case that initializes the CommandAttrs object. All wrapped
 * function calls return the values expected by the function under test.
 */
static void
command_attrs_init_tpm_success_test (void **state)
{
    test_data_t *data = *state;
    gint         ret = -1;
    TPMA_CC      command_attributes [2] = { 0xdeadbeef, 0xfeebdaed };

    will_return (__wrap_tpm2_get_command_attrs, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_get_command_attrs, 2);
    will_return (__wrap_tpm2_get_command_attrs, command_attributes);

    ret = command_attrs_init_tpm (data->command_attrs, data->tpm2);
    assert_int_equal (ret, 0);
    assert_memory_equal (command_attributes,
                         data->command_attrs->command_attrs,
                         sizeof (TPMA_CC) * data->command_attrs->count);
}
/*
 * Test case taht exercises the error handling path for a failed call to
 * Tss2_Sys_GetCapability.
 */
static void
command_attrs_init_tpm_fail_get_capability_test (void **state)
{
    test_data_t *data = *state;
    gint         ret = -1;

    will_return (__wrap_tpm2_get_command_attrs, TPM2_RC_FAILURE);

    ret = command_attrs_init_tpm (data->command_attrs, data->tpm2);
    assert_int_equal (ret, -1);
}
/*
 * Test a successful call to the command_attrs_from_cc function. This relies
 * on command_attrs_init_tpm_setup to call the _init function successfully which
 * populates the CommandAttrs object with TPMA_CCs.
 */
static void
command_attrs_from_cc_success_test (void **state)
{
    test_data_t *data = *state;
    TPMA_CC      ret_attrs;

    /*
     * TPM2_CC_HierarchyControl *is* one of the TPM2_CCs populated in the
     * init_setup function.
     */
    ret_attrs = command_attrs_from_cc (data->command_attrs,
                                       TPM2_CC_HierarchyControl);
    assert_int_equal (ret_attrs & 0x7fff, TPM2_CC_HierarchyControl);
}
/*
 * Test a failed call to the command_attrs_from_cc function. This relies
 * on command_attrs_init_tpm_setup to call the _init function successfully which
 * populates the CommandAttrs object with TPMA_CCs. This time we supply a
 * TPM2_CC that isn't populated in the _init function so the call fails.
 */
static void
command_attrs_from_cc_fail_test (void **state)
{
    test_data_t *data = *state;
    TPMA_CC      ret_attrs;

    /*
     * TPM2_CC_EvictControl is *not* one of the TPM2_CCs populated in the
     * init_setup function
     */
    ret_attrs = command_attrs_from_cc (data->command_attrs,
                                       TPM2_CC_EvictControl);
    assert_int_equal (ret_attrs, 0);
}
gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (command_attrs_type_test,
                                         command_attrs_setup,
                                         command_attrs_teardown),
        cmocka_unit_test_setup_teardown (command_attrs_init_tpm_success_test,
                                         command_attrs_setup,
                                         command_attrs_teardown),
        cmocka_unit_test_setup_teardown (command_attrs_init_tpm_fail_get_capability_test,
                                         command_attrs_setup,
                                         command_attrs_teardown),
        cmocka_unit_test_setup_teardown (command_attrs_from_cc_success_test,
                                         command_attrs_init_tpm_setup,
                                         command_attrs_teardown),
        cmocka_unit_test_setup_teardown (command_attrs_from_cc_fail_test,
                                         command_attrs_init_tpm_setup,
                                         command_attrs_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
