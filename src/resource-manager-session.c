/* SPDX-License-Identifier: BSD-2-Clause */
#include <glib.h>
#include <inttypes.h>

#include "tpm2.h"
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

    size_buf = session_entry_get_context (entry);
    cmd = tpm2_command_new_context_load (size_buf->buf, size_buf->size);
    if (cmd == NULL) {
        g_critical ("%s: failed to allcoate ContextLoad Tpm2Command",
                    __func__);
        resp = tpm2_response_new_rc (NULL, TSS2_RESMGR_RC_GENERAL_FAILURE);
        goto out;
    }
    resp = tpm2_send_command (resmgr->tpm2, cmd, &rc);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("%s: TCTI failed while loading session context from "
                   "SessionEntry, got RC 0x%" PRIx32, __func__, rc);
        resp = tpm2_response_new_rc (NULL, rc);
        goto out;
    }
    rc = tpm2_response_get_code (resp);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: failed to ContextLoad SessionEntry, got RC 0x%"
                   PRIx32, __func__, rc);
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

    g_debug ("%s: flushing stale SessionEntry with handle: 0x%08" PRIx32,
             __func__, handle);
    rc = tpm2_context_flush (resmgr->tpm2, handle);
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
    if (session_entry_get_state (entry) != SESSION_ENTRY_LOADED) {
        g_critical ("%s: SessionEntry already loaded", __func__);
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
    resp = tpm2_send_command (resmgr->tpm2, cmd, &rc);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("%s: TCTI failed while saving session context from "
                    "SessionEntry, got RC 0x%" PRIx32, __func__, rc);
        resp = tpm2_response_new_rc (NULL, rc);
        goto out;
    }
    rc = tpm2_response_get_code (resp);
    if (rc != TSS2_RC_SUCCESS) {
        g_info ("%s: failed to ContextSave SessionEntry, got RC 0x%" PRIx32,
                __func__, rc);
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
/*
 * This function will send the required commands in order to bring a
 * session back into the gap interval. This means loading and saving
 * the associated context. If the SessionEntry is not saved then this
 * function is a no-op.
 */
gboolean
regap_session (ResourceManager *resmgr,
               SessionEntry *entry)
{
    SessionEntryStateEnum state;
    Tpm2Response *resp = NULL;
    gboolean ret = TRUE;

    g_assert_nonnull (resmgr);
    g_assert_nonnull (entry);

    state = session_entry_get_state (entry);
    g_debug ("%s: swapping SessionEntry in state \"%s\"", __func__,
             session_entry_state_to_str (state));
    if (state == SESSION_ENTRY_SAVED_CLIENT ||
        state == SESSION_ENTRY_SAVED_CLIENT_CLOSED ||
        state == SESSION_ENTRY_SAVED_RM)
    {
        resp = load_session (resmgr, entry);
        if (tpm2_response_get_code (resp) != TSS2_RC_SUCCESS) {
            g_critical ("%s: Failed to save SessionEntry removing from list",
                        __func__);
            flush_session (resmgr, entry);
            ret = FALSE;
            goto out;
        }
        g_clear_object (&resp);
        resp = save_session (resmgr, entry);
        if (tpm2_response_get_code (resp) != TSS2_RC_SUCCESS) {
            g_critical ("%s: Failed to load SessionEntry. Got RC 0x%" PRIx32
                        ", removing from list", __func__,
                        tpm2_response_get_code (resp));
            flush_session (resmgr, entry);
            ret = FALSE;
        }
    }
out:
    g_clear_object (&resp);
    return ret;
}
