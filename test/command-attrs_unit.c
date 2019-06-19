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
#include "access-broker.h"
#include "command-attrs.h"
#include "tcti.h"
#include "tcti-mock.h"

typedef struct test_data {
    AccessBroker *access_broker;
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
    data->access_broker = access_broker_new (tcti);
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
    will_return (__wrap_access_broker_lock_sapi, 1);
    will_return (__wrap_access_broker_get_max_command, 2);
    will_return (__wrap_access_broker_get_max_command, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, &command_attributes);
    will_return (__wrap_Tss2_Sys_GetCapability, TSS2_RC_SUCCESS);

    ret = command_attrs_init_tpm (data->command_attrs, data->access_broker);
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
TSS2_SYS_CONTEXT*
__wrap_access_broker_lock_sapi (AccessBroker *access_broker)
{
    UNUSED_PARAM(access_broker);
    return mock_ptr_type (TSS2_SYS_CONTEXT*);
}
/* */
TSS2_RC
__wrap_access_broker_get_max_command (AccessBroker *access_broker,
                                      guint        *value)
{
    TSS2_RC rc;
    UNUSED_PARAM(access_broker);

    *value = mock_type (guint);
    g_debug ("value: 0x%" PRIx32, *value);
    rc = mock_type (TSS2_RC);
    g_debug ("rc: 0x%" PRIx32, rc);

    return rc;
}
/* */
TSS2_RC
__wrap_Tss2_Sys_GetCapability (TSS2_SYS_CONTEXT         *sysContext,
                               TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
                               TPM2_CAP                   capability,
                               UINT32                    property,
                               UINT32                    propertyCount,
                               TPMI_YES_NO              *moreData,
                               TPMS_CAPABILITY_DATA     *capabilityData,
                               TSS2L_SYS_AUTH_RESPONSE  *rspAuthsArray)
{
    TPMA_CC *command_attrs;
    unsigned int i;
    UNUSED_PARAM(sysContext);
    UNUSED_PARAM(cmdAuthsArray);
    UNUSED_PARAM(capability);
    UNUSED_PARAM(property);
    UNUSED_PARAM(moreData);
    UNUSED_PARAM(rspAuthsArray);

    capabilityData->data.command.count = propertyCount;
    command_attrs = mock_ptr_type (TPMA_CC*);
    for (i = 0; i < capabilityData->data.command.count; ++i)
        capabilityData->data.command.commandAttributes[i] = command_attrs[i];

    return mock_type (TSS2_RC);
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

    will_return (__wrap_access_broker_lock_sapi, 1);
    will_return (__wrap_access_broker_get_max_command, 2);
    will_return (__wrap_access_broker_get_max_command, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, &command_attributes);
    will_return (__wrap_Tss2_Sys_GetCapability, TSS2_RC_SUCCESS);

    ret = command_attrs_init_tpm (data->command_attrs, data->access_broker);
    assert_int_equal (ret, 0);
    assert_memory_equal (command_attributes,
                         data->command_attrs->command_attrs,
                         sizeof (TPMA_CC) * data->command_attrs->count);
}
/*
 * Test case that exercises the error handling path in the CommandAttrs
 * init function when a NULL SAPI context is returned by the AccessBroker.
 */
static void
command_attrs_init_tpm_null_sapi_test (void **state)
{
    test_data_t *data = *state;
    gint         ret = -1;

    will_return (__wrap_access_broker_get_max_command, 2);
    will_return (__wrap_access_broker_get_max_command, TSS2_RC_SUCCESS);
    will_return (__wrap_access_broker_lock_sapi, NULL);
    ret = command_attrs_init_tpm (data->command_attrs, data->access_broker);
    assert_int_equal (ret, -1);
}
/*
 * Test case that exercises the error handling path for a failed call to
 * the access_broker_get_max_command function call in the CommandAttrs
 * init function.
 */
static void
command_attrs_init_tpm_fail_get_max_command_test (void **state)
{
    test_data_t *data = *state;
    gint         ret = -1;

    will_return (__wrap_access_broker_get_max_command, 2);
    will_return (__wrap_access_broker_get_max_command, 1);

    ret = command_attrs_init_tpm (data->command_attrs, data->access_broker);
    assert_int_equal (ret, -1);
}
/*
 * Test case that exercises the error handling path for a successful call
 * to the access_broker_get_max_command function that returns 0 commands.
 * This would mean that the TPM supports no commands.
 */
static void
command_attrs_init_tpm_zero_get_max_command_test (void **state)
{
    test_data_t *data = *state;
    gint         ret = -1;

    will_return (__wrap_access_broker_get_max_command, 0);
    will_return (__wrap_access_broker_get_max_command, TSS2_RC_SUCCESS);

    ret = command_attrs_init_tpm (data->command_attrs, data->access_broker);
    assert_int_equal (ret, -1);
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
    TPMA_CC      command_attributes [2] = { 0xdeadbeef, 0xfeebdaed };

    will_return (__wrap_access_broker_lock_sapi, 1);
    will_return (__wrap_access_broker_get_max_command, 2);
    will_return (__wrap_access_broker_get_max_command, TSS2_RC_SUCCESS);
    will_return (__wrap_Tss2_Sys_GetCapability, &command_attributes);
    will_return (__wrap_Tss2_Sys_GetCapability, 1);

    ret = command_attrs_init_tpm (data->command_attrs, data->access_broker);
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
        cmocka_unit_test_setup_teardown (command_attrs_init_tpm_null_sapi_test,
                                         command_attrs_setup,
                                         command_attrs_teardown),
        cmocka_unit_test_setup_teardown (command_attrs_init_tpm_fail_get_max_command_test,
                                         command_attrs_setup,
                                         command_attrs_teardown),
        cmocka_unit_test_setup_teardown (command_attrs_init_tpm_zero_get_max_command_test,
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
