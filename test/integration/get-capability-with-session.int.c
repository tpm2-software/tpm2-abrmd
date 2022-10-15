/* SPDX-License-Identifier: BSD-2-Clause */
#include <glib.h>

#include "common.h"

/* test that TPM2_GetCapability works with an audit session and fails
 * with a password session.
 */
int
test_invoke (TSS2_SYS_CONTEXT *sapi_context) {
    TSS2_RC rc = 0, expected_rc =
        (TPM2_RC_HANDLE | TPM2_RC_S | TPM2_RC_1);
    TPM2_HANDLE session;
    TSS2L_SYS_AUTH_COMMAND cmdauths = {.count = 1 };
    TSS2L_SYS_AUTH_RESPONSE respauths;
    TPMI_YES_NO more = 0;
    TPMS_CAPABILITY_DATA capdata;

    cmdauths.auths[0].sessionHandle = TPM2_RH_PW;

    rc = Tss2_Sys_GetCapability(sapi_context,
				&cmdauths,
				TPM2_CAP_PCRS,
				0,
				0,
				&more,
				&capdata,
				&respauths);

    if (rc != (TPM2_RC_HANDLE | TPM2_RC_S | TPM2_RC_1)) {
        g_warning("Tss2_Sys_GetCapability with password session "
		  "failed with 0x%x, expected 0x%x", rc, expected_rc);
        return rc;
    }

    rc = start_auth_session_hmac(sapi_context, &session);
    if (rc) {
        g_warning("start_auth_session_hmac failed with 0x%x", rc);
        return rc;
    }

    cmdauths.auths[0].sessionHandle = session;
    cmdauths.auths[0].sessionAttributes |= TPMA_SESSION_AUDIT;

    rc = Tss2_Sys_GetCapability(sapi_context,
				&cmdauths,
				TPM2_CAP_PCRS,
				0,
				0,
				&more,
				&capdata,
				&respauths);
    if (rc) {
        g_warning("Tss2_Sys_GetCapability with audit session failed "
		  "with 0x%x", rc);
    return rc;
    }

    return 0;
}
