/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <inttypes.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include <tss2/tss2_tpm2_types.h>

#include "common.h"
#include "tpm2-struct-init.h"
#include "test-options.h"
#include "context-util.h"

#define PRIxHANDLE "08" PRIx32

/*
 * This test exercises the session tracking logic, specifically it creates an
 * auth session over one connection to the RM, saves it, then closes down said
 * connection. A new connection is then created and the same session is loaded
 * from this new connection.
 * In this way we're testing the RMs ability to have a saved session live on
 * beyond the session that created it.
 */
int
main ()
{

	unsigned i;
	/*
	 * 5 is where the failures first start, things work at 4.
	 */
	for (i=0; i < 5; i++) {
		TSS2_RC rc;
		TSS2_SYS_CONTEXT *sapi_context;
		TPMI_SH_AUTH_SESSION  session_handle = 0, session_handle_load = 0;
        TPMS_CONTEXT          context = TPMS_CONTEXT_ZERO_INIT;
		test_opts_t           opts = TEST_OPTS_DEFAULT_INIT;

		get_test_opts_from_env (&opts);
		if (sanity_check_test_opts (&opts) != 0)
			exit (1);
		g_info ("Creating first SAPI context");
		sapi_context = sapi_init_from_opts (&opts);
		if (sapi_context == NULL) {
			g_error ("Failed to create SAPI context.");
		}
		g_info ("Got SAPI context: 0x%" PRIxPTR, (uintptr_t)sapi_context);
		/* create an auth session */
		g_info ("Starting unbound, unsalted auth session");
		rc = start_auth_session (sapi_context, &session_handle);
		if (rc != TSS2_RC_SUCCESS) {
			g_error ("Tss2_Sys_StartAuthSession failed: 0x%" PRIxHANDLE, rc);
		}
		g_info ("StartAuthSession for TPM_SE_POLICY success! Session handle: "
				"0x%08" PRIx32, session_handle);

		/* save context */
		g_info ("Saving context for session: 0x%" PRIxHANDLE, session_handle);
		rc = Tss2_Sys_ContextSave (sapi_context, session_handle, &context);
		if (rc != TSS2_RC_SUCCESS) {
			g_error ("Tss2_Sys_ContextSave failed: 0x%" PRIxHANDLE, rc);
		}
		prettyprint_context (&context);
		g_info ("Tearing down SAPI connection 0x%" PRIxPTR,
				(uintptr_t)sapi_context);
		sapi_teardown_full (sapi_context);

		g_info ("Creating second SAPI context");
		sapi_context = sapi_init_from_opts (&opts);
		if (sapi_context == NULL) {
			g_error ("Failed to create SAPI context.");
		}
		g_info ("Got SAPI context: 0x%" PRIxPTR, (uintptr_t)sapi_context);
		/* reload the session through new connection */
		g_info ("Loading context for session: 0x%" PRIxHANDLE, session_handle);
		rc = Tss2_Sys_ContextLoad (sapi_context, &context, &session_handle_load);
		if (rc != TSS2_RC_SUCCESS) {
			g_error ("Tss2_Sys_ContextLoad failed: 0x%" PRIxHANDLE, rc);
		}
		g_info ("Successfully loaded context for session: 0x%" PRIxHANDLE,
				session_handle_load);
		if (session_handle_load != session_handle) {
			g_error ("session_handle != session_handle_load");
		}
		sapi_teardown_full (sapi_context);
	}
    return 0;
}
