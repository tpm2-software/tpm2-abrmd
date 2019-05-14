/* SPDX-License-Identifier: BSD-2-Clause */
#include <inttypes.h>
#include <glib.h>
#include <string.h>

#include <tss2/tss2_sys.h>

#include "common.h"
#include "test.h"
#define PRIxHANDLE "08" PRIx32
#define PRIxRC PRIx32

#define FIRST_CONTEXT_SEQUENCE 0x4

#define MS_SIMULATOR_GAP_MAX UINT8_MAX

/*
 * This test exercises the resource manager's handling of the session gap
 * mechanism. Currently the RM does not 'virtualize' the GetCapability
 * TPM2_PT_CONTEXT_GAP_MAX property. This allows us to use the value
 * returned by the simulator to detect the proper function of the
 * ResourceManager. We do this by detecting a rollover in the context
 * sequence number.
 *
 * Under normal circumstances (when run against the simulator w/o the RM)
 * we will cause a TPM_RC_CONTEXT_GAP error before the sequence number of
 * the last saved session exceeds that of the first + GAP_MAX. If we do not,
 * then we consider the test a success.
 *
 * NOTE: The value of GAP_MAX returned by the RM will be UINT32_MAX while
 * the MS simulator (no RM) will return UINT8_MAX (counter is 1 byte). In
 * this case we cannot use the GAP_MAX property returned by the RM and
 * must assume GAP_MAX to be UINT8_MAX. This test will then become invalid
 * if / when the MS simulator changes the size of their GAP counter.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC rc;
    TPMI_SH_AUTH_SESSION session_handle = 0, session_handle_trans = 0;
    TPMS_CONTEXT context = { 0 }, context_tmp = { 0 };
    UINT32 gap_max = 0;
    UINT64 initial_sequence = 0, last_sequence = 0;

    rc = get_context_gap_max (sapi_context, &gap_max);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("%s: get_context_gap_max failed with RC: 0x%" PRIx32,
                 __func__, rc);
    }
    g_info ("%s: GAP_MAX is 0x%" PRIx32, __func__, gap_max);
    if (gap_max == UINT32_MAX) {
        g_warning ("%s: GAP_MAX is UINT32_MAX, implies RM implements context "
                   "gap handling, assuming GAP value is same used by MS "
                   "smiulator: 0x%" PRIx32, __func__, MS_SIMULATOR_GAP_MAX);
        gap_max = MS_SIMULATOR_GAP_MAX;
    }
    g_info ("%s: Starting unbound, unsalted auth session", __func__);
    rc = start_auth_session (sapi_context, &session_handle);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Tss2_Sys_StartAuthSession failed: 0x%" PRIxRC, rc);
    }
    g_info ("%s: StartAuthSession for TPM_SE_POLICY success! Session handle: "
            "0x%" PRIxHANDLE, __func__, session_handle);
    rc = Tss2_Sys_ContextSave (sapi_context, session_handle, &context);
    if (rc != TSS2_RC_SUCCESS) {
        g_error ("Failed to save context for initial session: 0x%" PRIxRC, rc);
    }
    g_info ("%s: Saved context for session with handle: 0x%" PRIxHANDLE " and "
            "sequence: 0x%" PRIx64, __func__, session_handle, context.sequence);
    initial_sequence = last_sequence = context.sequence;
    g_debug ("%s: initial_sequence: 0x%" PRIx64 ", last_sequence: 0x%"
             PRIx64 ", gap_max: 0x%" PRIx32, __func__, initial_sequence,
             last_sequence, gap_max);
    for (last_sequence = initial_sequence;
         last_sequence - initial_sequence < gap_max;
         last_sequence = context_tmp.sequence)
    {
        /*
         * NOTE: To trigger the GAP RC it's not strictly necessary for us
         * to save the session context in this loop as the RM will save it
         * as part of its normal operation. To make this test work against
         * the simulator however we must do the save operation here even
         * though it's redundant when running against the RM.
         */
        g_debug ("%s: last_sequence: %" PRIx64, __func__, last_sequence);
        g_debug ("%s: initial_sequence: %" PRIx64, __func__, initial_sequence);
        rc = start_auth_session (sapi_context, &session_handle_trans);
        if (rc != TSS2_RC_SUCCESS) {
            g_error ("Tss2_Sys_StartAuthSession failed: 0x%" PRIxRC, rc);
        }
        memset (&context_tmp, 0, sizeof (TPMS_CONTEXT));
        g_info ("%s: StartAuthSession for TPM_SE_POLICY success! Session handle: "
                "0x%" PRIxHANDLE, __func__, session_handle_trans);
        rc = Tss2_Sys_ContextSave (sapi_context,
                                   session_handle_trans,
                                   &context_tmp);
        if (rc != TSS2_RC_SUCCESS) {
            g_error ("%s: Tss2_Sys_ContextSave failed for handle: 0x%" PRIxHANDLE
                     " rc: 0x%" PRIxRC, __func__, session_handle_trans, rc);
        }
        g_debug ("%s: context_tmp.sequence: 0x%" PRIx64, __func__, context_tmp.sequence);
        g_info ("%s: Saved context for session with handle: 0x%" PRIxHANDLE " and "
                "sequence: 0x%" PRIx64, __func__, session_handle_trans, context_tmp.sequence);
        rc = Tss2_Sys_FlushContext (sapi_context, session_handle_trans);
        if (rc != TSS2_RC_SUCCESS) {
            g_error ("%s: Tss2_Sys_FlushContext failed: 0x%" PRIxRC, __func__, rc);
        }
    }
    g_info("%s: success: last_sequence - initial_sequence: 0x%"
           PRIx64 ", gap_max - 1: 0x%" PRIx32, __func__,
           last_sequence - initial_sequence, gap_max - 1);

    return 0;
}
