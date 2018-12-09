/* SPDX-License-Identifier: BSD-2 */
#include <glib.h>
#include <inttypes.h>

#include "access-broker.h"
#include "resource-manager.h"
#include "resource-manager-session.h"
#include "session-list.h"
#include "session-entry.h"
#include "tabrmd.h"
#include "tpm2-command.h"
#include "tpm2-header.h"
#include "tpm2-response.h"
#include "util.h"

/*
 * Load the TPMS_CONTEXT associated with the provided SessionEntry*.
 * This function will always return a Tpm2Response object (never NULL)
 * though the response object may indicate a failure in the response code.
 */
Tpm2Response*
load_session (ResourceManager *resmgr,
              SessionEntry *entry)
{
    Tpm2Command *cmd = NULL;
    Tpm2Response *resp = NULL;
    size_buf_t *size_buf = NULL;
    TSS2_RC rc = TSS2_RC_SUCCESS;

    g_debug ("%s: SessionEntry 0x%" PRIxPTR, __func__, (uintptr_t)entry);
    size_buf = session_entry_get_context (entry);
    cmd = tpm2_command_new_context_load (size_buf->buf, size_buf->size);
    if (cmd == NULL) {
        g_critical ("%s: failed to allcoate ContextLoad Tpm2Command",
                    __func__);
        resp = tpm2_response_new_rc (NULL, TSS2_RESMGR_RC_GENERAL_FAILURE);
        goto out;
    }
    resp = access_broker_send_command (resmgr->access_broker, cmd, &rc);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("%s: TCTI failed while loading session context from "
                   "SessionEntry 0x%" PRIxPTR ", got RC 0x%" PRIx32,
                   __func__, (uintptr_t)entry, rc);
        resp = tpm2_response_new_rc (NULL, rc);
        goto out;
    }
    rc = tpm2_response_get_code (resp);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: failed to ContextLoad SessionEntry 0x%" PRIxPTR ", got "
                   "RC 0x%" PRIx32, __func__, (uintptr_t)entry, rc);
        goto out;
    }
    session_entry_set_state (entry, SESSION_ENTRY_LOADED);
out:
    g_clear_object (&cmd);
    return resp;
}
/*
 * This function flushes the context associated with the provided SessionEntry
 * from the TPM. Once a session is flushed it cannot be re-loaded and so we
 * remove the SessionEntry from the session_list as well.
 */
gboolean
flush_session (ResourceManager *resmgr,
               SessionEntry *entry)
{
    g_assert_nonnull (resmgr);
    g_assert_nonnull (entry);

    TPM2_HANDLE handle = session_entry_get_handle (entry);
    TSS2_RC rc;

    g_debug ("%s: flushing stale SessionEntry: 0x%" PRIxPTR " with "
             "handle: 0x%08" PRIx32,
             __func__, (uintptr_t)entry, handle);
    rc = access_broker_context_flush (resmgr->access_broker, handle);
    session_list_remove (resmgr->session_list, entry);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: failed to flush session context with "
                   "handle 0x%" PRIx32 ": 0x%" PRIx32,
                   __func__, handle, rc);
        return FALSE;
    }
    return TRUE;
}
/*
 * Saves context of the provided SessionEntry. If successful the
 * SessionEntry state will be udpated and the context blob updated. The
 * function will always return a Tpm2Response. If there was any error
 * encounterd it will be reflected in the Tpm2Response object RC.
 */
Tpm2Response*
save_session (ResourceManager *resmgr,
              SessionEntry *entry)
{
    Tpm2Command *cmd = NULL;
    Tpm2Response *resp = NULL;
    TSS2_RC rc = TSS2_RC_SUCCESS;

    g_assert_nonnull (resmgr);
    g_assert_nonnull (entry);
    g_debug ("%s: SessionEntry: 0x%" PRIxPTR, __func__, (uintptr_t)entry);
    if (session_entry_get_state (entry) != SESSION_ENTRY_LOADED) {
        g_critical ("%s: SessionEntry 0x%" PRIxPTR " already loaded",
                    __func__, (uintptr_t)entry);
        resp = tpm2_response_new_rc (NULL, TSS2_RESMGR_RC_GENERAL_FAILURE);
        goto out;
    }
    cmd = tpm2_command_new_context_save (session_entry_get_handle (entry));
    if (cmd == NULL) {
        g_critical ("%s: failed to allocate ContextSave Tpm2Command",
                    __func__);
        resp = tpm2_response_new_rc (NULL, TSS2_RESMGR_RC_GENERAL_FAILURE);
        goto out;
    }
    resp = access_broker_send_command (resmgr->access_broker, cmd, &rc);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("%s: TCTI failed while saving session context from "
                   "SessionEntry 0x%" PRIxPTR ", got RC 0x%" PRIx32,
                   __func__, (uintptr_t)entry, rc);
        resp = tpm2_response_new_rc (NULL, rc);
        goto out;
    }
    rc = tpm2_response_get_code (resp);
    if (rc != TSS2_RC_SUCCESS) {
        g_info ("%s: failed to ContextSave SessionEntry 0x%" PRIxPTR ", got "
                "RC 0x%" PRIx32, __func__, (uintptr_t)entry, rc);
        goto out;
    }
    session_entry_set_context (entry,
                               &tpm2_response_get_buffer (resp)[TPM_HEADER_SIZE],
                               tpm2_response_get_size (resp) - TPM_HEADER_SIZE);
    session_entry_set_state (entry, SESSION_ENTRY_SAVED_RM);
out:
    g_clear_object (&cmd);
    return resp;
}
