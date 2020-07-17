/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <inttypes.h>
#include <string.h>

#include <glib.h>

#include <tss2/tss2_mu.h>

#include "connection.h"
#include "connection-manager.h"
#include "control-message.h"
#include "logging.h"
#include "message-queue.h"
#include "resource-manager-session.h"
#include "resource-manager.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "tabrmd.h"
#include "tpm2-header.h"
#include "tpm2-command.h"
#include "tpm2-response.h"
#include "util.h"

#define MAX_ABANDONED 4

static void resource_manager_sink_interface_init   (gpointer g_iface);
static void resource_manager_source_interface_init (gpointer g_iface);

G_DEFINE_TYPE_WITH_CODE (
    ResourceManager,
    resource_manager,
    TYPE_THREAD,
    G_IMPLEMENT_INTERFACE (TYPE_SINK,
                           resource_manager_sink_interface_init);
    G_IMPLEMENT_INTERFACE (TYPE_SOURCE,
                           resource_manager_source_interface_init);
    );


enum {
    PROP_0,
    PROP_QUEUE_IN,
    PROP_SINK,
    PROP_TPM2,
    PROP_SESSION_LIST,
    N_PROPERTIES
};
static GParamSpec *obj_properties [N_PROPERTIES] = { NULL, };
/*
 * This is a helper function that does everything required to convert
 * a virtual handle to a physical one in a Tpm2Command object.
 * - load the context from the provided HandleMapEntry
 * - store the newly assigned TPM handle (physical handle) in the entry
 * - set this handle in the comamnd at the position indicated by
 *   'handle_number' (0-based index)
 */
TSS2_RC
resource_manager_virt_to_phys (ResourceManager *resmgr,
                               Tpm2Command     *command,
                               HandleMapEntry  *entry,
                               guint8           handle_number)
{
    TPM2_HANDLE    phandle = 0;
    TPMS_CONTEXT *context;
    TSS2_RC       rc = TSS2_RC_SUCCESS;

    context = handle_map_entry_get_context (entry);
    if (handle_map_entry_get_phandle(entry)) {
        phandle = handle_map_entry_get_phandle(entry);
        g_debug ("remembered phandle: 0x%" PRIx32, phandle);
        tpm2_command_set_handle (command, phandle, handle_number);
        return TSS2_RC_SUCCESS;
    }

    rc = tpm2_context_load (resmgr->tpm2, context, &phandle);
    g_debug ("loaded phandle: 0x%" PRIx32, phandle);
    if (rc == TSS2_RC_SUCCESS) {
        handle_map_entry_set_phandle (entry, phandle);
        tpm2_command_set_handle (command, phandle, handle_number);
    } else {
        g_warning ("Failed to load context: 0x%" PRIx32, rc);
    }
    return rc;
}
TSS2_RC
resource_manager_load_transient (ResourceManager  *resmgr,
                                 Tpm2Command      *command,
                                 GSList          **entry_slist,
                                 TPM2_HANDLE        handle,
                                 guint8            handle_index)
{
    HandleMap    *map;
    HandleMapEntry *entry;
    Connection  *connection;
    TSS2_RC       rc = TSS2_RC_SUCCESS;

    g_debug ("processing TPM2_HT_TRANSIENT: 0x%" PRIx32, handle);
    connection = tpm2_command_get_connection (command);
    map = connection_get_trans_map (connection);
    g_object_unref (connection);
    g_debug ("handle 0x%" PRIx32 " is virtual TPM2_HT_TRANSIENT, "
             "loading", handle);
    /* we don't unref the entry since we're adding it to the entry_slist below */
    entry = handle_map_vlookup (map, handle);
    if (entry) {
        g_debug ("mapped virtual handle 0x%" PRIx32 " to entry", handle);
    } else {
        g_warning ("No HandleMapEntry for vhandle: 0x%" PRIx32, handle);
        goto out;
    }
    rc = resource_manager_virt_to_phys (resmgr, command, entry, handle_index);
    if (rc != TSS2_RC_SUCCESS) {
        goto out;
    }
    *entry_slist = g_slist_prepend (*entry_slist, entry);
out:
    g_object_unref (map);
    return rc;
}
/*
 * This structure is used by the regap_session_callback to allow the
 * response code from the callbacks to be passed to the caller.
 */
typedef struct {
    ResourceManager *resmgr;
    gboolean ret;
} regap_session_data_t;
/*
 * This is a version of the 'regap_session' function with the with the
 * appropriate prototype for use with GList 'foreach' iterator.
 */
void
regap_session_callback (gpointer data_entry,
                        gpointer data_user)
{
    regap_session_data_t *data = (regap_session_data_t*)data_user;
    SessionEntry *entry  = SESSION_ENTRY (data_entry);

    g_debug ("%s: SessionEntry", __func__);
    if (data->ret == TRUE) {
        data->ret = regap_session (data->resmgr, entry);
    } else {
        g_critical ("%s: previous attempt to regap failed, skipping"
                    " SessionEntry", __func__);
    }
}
/*
 * This function is a handler for response codes that we may get from the
 * TPM in response to commands. It may result in addtional commands being
 * sent to the TPM so use it with caution.
 */
gboolean
handle_rc (ResourceManager *resmgr,
           TSS2_RC rc)
{
    regap_session_data_t data = {
        .resmgr = resmgr,
        .ret = TRUE,
    };
    gboolean ret;

    g_debug ("%s: handling  RC 0x%" PRIx32, __func__, rc);
    switch (rc) {
    case TPM2_RC_CONTEXT_GAP:
        g_debug ("%s: handling TPM2_RC_CONTEXT_GAP", __func__);
        session_list_foreach (resmgr->session_list,
                              regap_session_callback,
                              &data);
        ret = data.ret;
        break;
    default:
        g_debug ("%s: Unable to recover gracefully from RC 0x%" PRIx32,
                 __func__, rc);
        ret = TRUE;
        break;
    }

    return ret;
}/*
 * This is a somewhat generic function used to load session contexts into
 * the TPM2 device. The ResourceManager uses this function when loading
 * sessions in the handle or auth area of a command. It will refuse to load
 * sessions that:
 * - aren't tracked by the ResourceManager (must have SessionEntry in
 *   SessionList)
 * - that were last saved by the client instead of the RM
 * - aren't owned by the Connection object associated with the Tpm2Command
 */
TSS2_RC
resource_manager_load_session_from_handle (ResourceManager *resmgr,
                                           Connection      *command_conn,
                                           TPM2_HANDLE       handle,
                                           gboolean         will_flush)
{
    Connection   *entry_conn = NULL;
    SessionEntry *session_entry = NULL;
    Tpm2Response *response = NULL;
    SessionEntryStateEnum session_entry_state;
    TSS2_RC        rc = TSS2_RC_SUCCESS;

    session_entry = session_list_lookup_handle (resmgr->session_list,
                                                handle);
    if (session_entry == NULL) {
        g_debug ("no session with handle 0x%08" PRIx32 " known to "
                 "ResourceManager.", handle);
        goto out;
    }
    g_debug ("%s: mapped session handle 0x%08" PRIx32 " to "
             "SessionEntry", __func__, handle);
    entry_conn = session_entry_get_connection (session_entry);
    if (command_conn != entry_conn) {
        g_warning ("%s: Connection from Tpm2Command and SessionEntry do not "
                   "match. Refusing to load.", __func__);
        goto out;
    }
    session_entry_state = session_entry_get_state (session_entry);
    if (session_entry_state != SESSION_ENTRY_SAVED_RM) {
        g_warning ("%s: Handle in handle area references SessionEntry "
                   "for session in state \"%s\". Must be in state: "
                   "SESSION_ENTRY_SAVED_RM for us manage it, ignorning.",
                   __func__, session_entry_state_to_str (session_entry_state));
        goto out;
    }
    response = load_session (resmgr, session_entry);
    rc = tpm2_response_get_code (response);
    if (rc != TSS2_RC_SUCCESS) {
        if (handle_rc (resmgr, rc) != TRUE) {
            g_warning ("Failed to load context for session with handle "
                       "0x%08" PRIx32 " RC: 0x%" PRIx32, handle, rc);
            flush_session (resmgr, session_entry);
            goto out;
        }
        response = load_session (resmgr, session_entry);
        rc = tpm2_response_get_code (response);
        if (rc != TSS2_RC_SUCCESS) {
            flush_session (resmgr, session_entry);
        }
    }
    if (will_flush) {
        g_debug ("%s: will_flush: removing SessionEntry from SessionList",
                 __func__);
        session_list_remove (resmgr->session_list, session_entry);
    }
out:
    g_clear_object (&entry_conn);
    g_clear_object (&response);
    g_clear_object (&session_entry);

    return rc;
}
typedef struct {
    ResourceManager *resmgr;
    Tpm2Command     *command;
} auth_callback_data_t;
void
resource_manager_load_auth_callback (gpointer auth_offset_ptr,
                                     gpointer user_data)
{
    Connection *connection = NULL;
    TPM2_HANDLE handle;
    auth_callback_data_t *data = (auth_callback_data_t*)user_data;
    TPMA_SESSION attrs;
    gboolean will_flush = TRUE;
    size_t auth_offset = *(size_t*)auth_offset_ptr;

    handle = tpm2_command_get_auth_handle (data->command, auth_offset);
    switch (handle >> TPM2_HR_SHIFT) {
    case TPM2_HT_HMAC_SESSION:
    case TPM2_HT_POLICY_SESSION:
        attrs = tpm2_command_get_auth_attrs (data->command, auth_offset);
        if (attrs & TPMA_SESSION_CONTINUESESSION) {
            will_flush = FALSE;
        }
        connection = tpm2_command_get_connection (data->command);
        resource_manager_load_session_from_handle (data->resmgr,
                                                   connection,
                                                   handle,
                                                   will_flush);
        break;
    default:
        g_debug ("not loading object with handle: 0x%08" PRIx32 " from "
                 "command auth area: not a session", handle);
        break;
    }
    g_clear_object (&connection);
}
/*
 * This function operates on the provided command. It iterates over each
 * handle in the commands handle area. For each relevant handle it loads
 * the related context and fixes up the handles in the command.
 */
TSS2_RC
resource_manager_load_handles (ResourceManager *resmgr,
                               Tpm2Command     *command,
                               GSList         **loaded_transients)
{
    Connection *connection = NULL;
    TSS2_RC       rc = TSS2_RC_SUCCESS;
    TPM2_HANDLE    handles[TPM2_COMMAND_MAX_HANDLES] = { 0, };
    size_t        i, handle_count = TPM2_COMMAND_MAX_HANDLES;
    gboolean      handle_ret;

    g_debug ("%s", __func__);
    if (!resmgr || !command) {
        g_warning ("%s: received NULL parameter.", __func__);
        return RM_RC (TSS2_BASE_RC_GENERAL_FAILURE);
    }
    handle_ret = tpm2_command_get_handles (command, handles, &handle_count);
    if (handle_ret == FALSE) {
        g_error ("Unable to get handles from command");
    }
    g_debug ("%s: for %zu handles in command handle area",
             __func__, handle_count);
    for (i = 0; i < handle_count; ++i) {
        switch (handles [i] >> TPM2_HR_SHIFT) {
        case TPM2_HT_TRANSIENT:
            g_debug ("processing TPM2_HT_TRANSIENT: 0x%" PRIx32, handles [i]);
            rc = resource_manager_load_transient (resmgr,
                                                  command,
                                                  loaded_transients,
                                                  handles [i],
                                                  i);
            break;
        case TPM2_HT_HMAC_SESSION:
        case TPM2_HT_POLICY_SESSION:
            g_debug ("processing TPM2_HT_HMAC_SESSION or "
                     "TPM2_HT_POLICY_SESSION: 0x%" PRIx32, handles [i]);
            connection = tpm2_command_get_connection (command);
            rc = resource_manager_load_session_from_handle (resmgr,
                                                            connection,
                                                            handles [i],
                                                            FALSE);
            break;
        default:
            break;
        }
    }
    g_debug ("%s: end", __func__);
    g_clear_object (&connection);

    return rc;
}
/*
 * Remove the context associated with the provided HandleMapEntry
 * from the TPM. Only handles in the TRANSIENT range will be flushed.
 * Any entry with a context that's flushed will have the physical handle
 * to 0.
 */
void
resource_manager_flushsave_context (gpointer data_entry,
                                    gpointer data_resmgr)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (data_resmgr);
    HandleMapEntry  *entry  = HANDLE_MAP_ENTRY (data_entry);
    TPMS_CONTEXT   *context;
    TPM2_HANDLE      phandle;
    TSS2_RC         rc = TSS2_RC_SUCCESS;

    g_debug ("%s: for entry", __func__);
    if (resmgr == NULL || entry == NULL)
        g_error ("%s: passed NULL parameter", __func__);
    phandle = handle_map_entry_get_phandle (entry);
    g_debug ("%s: phandle: 0x%" PRIx32, __func__, phandle);
    switch (phandle >> TPM2_HR_SHIFT) {
    case TPM2_HT_TRANSIENT:
        if (!handle_map_entry_get_phandle (entry)) {
            g_debug ("phandle for vhandle 0x%" PRIx32 " was already flushed.",
                handle_map_entry_get_vhandle (entry));
            break;
        }
        g_debug ("%s: handle is transient, saving context", __func__);
        context = handle_map_entry_get_context (entry);
        rc = tpm2_context_saveflush (resmgr->tpm2,
                                              phandle,
                                              context);
        if (rc == TSS2_RC_SUCCESS) {
            handle_map_entry_set_phandle (entry, 0);
        } else {
            g_warning ("%s: tpm2_context_saveflush failed for "
                       "handle: 0x%" PRIx32 " rc: 0x%" PRIx32,
                       __func__, phandle, rc);
        }
        break;
    default:
        break;
    }
}
/*
 * Remove the context associated with the provided SessionEntry from the
 * TPM. Only session objects should be saved by this function.
 */
void
save_session_callback (gpointer data_entry,
                       gpointer data_resmgr)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (data_resmgr);
    SessionEntry *entry  = SESSION_ENTRY (data_entry);
    Tpm2Response *resp = NULL;
    TSS2_RC rc;

    g_debug ("%s: SessionEntry", __func__);
    if (session_entry_get_state (entry) != SESSION_ENTRY_LOADED) {
        g_debug ("%s: cannot save SessionEntry, not loaded", __func__);
        return;
    }
    resp = save_session (resmgr, entry);
    rc = tpm2_response_get_code (resp);
    if (rc != TSS2_RC_SUCCESS) {
        if (handle_rc (resmgr, rc) != TRUE) {
            g_warning ("%s: Failed to save SessionEntry",
                       __func__);
            flush_session (resmgr, entry);
            goto out;

        }
        resp = save_session (resmgr, entry);
        if (tpm2_response_get_code (resp) != TSS2_RC_SUCCESS) {
            g_critical ("%s: failed to save SessionEntry, flushing",
                        __func__);
            flush_session (resmgr, entry);
         }
    }
out:
    g_clear_object (&resp);
    return;
}
static void
dump_command (Tpm2Command *command)
{
    g_assert (command != NULL);
    g_debug ("Tpm2Command");
    g_debug_bytes (tpm2_command_get_buffer (command),
                   tpm2_command_get_size (command),
                   16,
                   4);
    g_debug_tpma_cc (tpm2_command_get_attributes (command));
}
static void
dump_response (Tpm2Response *response)
{
    g_assert (response != NULL);
    g_debug ("Tpm2Response");
    g_debug_bytes (tpm2_response_get_buffer (response),
                   tpm2_response_get_size (response),
                   16,
                   4);
    g_debug_tpma_cc (tpm2_response_get_attributes (response));
}
/*
 * This function performs the special processing required when a client
 * attempts to save a session context. In short: handling the context gap /
 * contextCounter roll over is the only reason we have to get involved. To do
 * this we must be able to load and save every active session from oldest to
 * newest. This is discussed in detail in section 30.5 from part 1 of the TPM2
 * spec.
 *
 * The recommended algorithm for doing this is documented in one of the TSS2 specs.
 */
Tpm2Response*
resource_manager_save_context_session (ResourceManager *resmgr,
                                       Tpm2Command *command)
{
    Connection *conn_cmd = NULL, *conn_entry = NULL;
    SessionEntry *entry = NULL;
    Tpm2Response *response = NULL;
    TPM2_HANDLE handle = 0;

    handle = tpm2_command_get_handle (command, 0);
    g_debug ("save_context for session handle: 0x%" PRIx32, handle);
    entry = session_list_lookup_handle (resmgr->session_list, handle);
    if (entry == NULL) {
        g_warning ("Client attempting to save unknown session.");
        goto out;
    }
    /* the lookup function should check this for us? */
    conn_cmd = tpm2_command_get_connection (command);
    conn_entry = session_entry_get_connection (entry);
    if (conn_cmd != conn_entry) {
        g_warning ("%s: session belongs to a different connection", __func__);
        goto out;
    }
    session_entry_set_state (entry, SESSION_ENTRY_SAVED_CLIENT);
    response = tpm2_response_new_context_save (conn_cmd, entry);
    g_debug ("%s: Tpm2Response from TPM2_ContextSave", __func__);
    g_debug_bytes (tpm2_response_get_buffer (response),
                   tpm2_response_get_size (response),
                   16, 4);
out:
    g_clear_object (&conn_cmd);
    g_clear_object (&conn_entry);
    g_clear_object (&entry);
    return response;
}
/*
 * This function performs the special processing associated with the
 * TPM2_ContextSave command. How much we can "virtualize of this command
 * depends on the parameters / handle type as well as how much work we
 * actually *want* to do.
 *
 * Transient objects that are tracked by the RM require no special handling.
 * It's possible that the whole of this command could be virtualized: When a
 * command is received all transient objects have been saved and flushed. The
 * saved context held by the RM could very well be returned to the caller
 * with no interaction with the TPM. This would however require that the RM
 * marshal the context object into the response buffer. This is less easy
 * than it should be and so we suffer the inefficiency to keep the RM code
 * more simple. Simple in this case means no special handling.
 *
 * Session objects are handled much in the same way with a specific caveat:
 * A session can be either loaded or saved. Unlike a transient object saving
 * it changes its state. And so for the caller to save the context it must
 * be loaded first. This is exactly what we do for transient objects, but
 * knowing that the caller wants to save the context we also know that the RM
 * can't have a saved copy as well. And so we must load the context and
 * destroy the mapping maintained by the RM. Since this function is called
 * after the session contexts are loaded we just need to drop the mapping.
 */
Tpm2Response*
resource_manager_save_context (ResourceManager *resmgr,
                               Tpm2Command     *command)
{
    TPM2_HANDLE handle = tpm2_command_get_handle (command, 0);

    g_debug ("%s", __func__);
    switch (handle >> TPM2_HR_SHIFT) {
    case TPM2_HT_HMAC_SESSION:
    case TPM2_HT_POLICY_SESSION:
        return resource_manager_save_context_session (resmgr, command);
    default:
        g_debug ("save_context: not virtualizing TPM2_CC_ContextSave for "
                 "handles: 0x%08" PRIx32, handle);
        break;
    }

    return NULL;
}
/*
 * This function performs the special processing required when handling a
 * TPM2_CC_ContextLoad command:
 * - First we look at the TPMS_CONTEXT provided in the command body. If we've
 *   never see the context then we do nothing and let the command be processed
 *   in its current form. If the context is valid and the TPM loads it the
 *   ResourceManager will intercept the response and begin tracking the
 *   session.
 * - Second we check to be sure the session can be loaded by the connection
 *   over which we received the command. This requires that it's either:
 *   - the same connection associated with the SessionEntry
 *   or
 *   - that the SessionEntry has been abandoned
 * - If the access control check pacsses we return the handle to the caller
 *   in a Tpm2Response that we craft.
 */
Tpm2Response*
resource_manager_load_context_session (ResourceManager *resmgr,
                                       Tpm2Command *command)
{
    Connection *conn_cmd = NULL, *conn_entry = NULL;
    SessionEntry *entry = NULL;
    Tpm2Response *response = NULL;

    g_debug ("%s", __func__);
    entry = session_list_lookup_context_client (resmgr->session_list,
                                                &tpm2_command_get_buffer (command) [TPM_HEADER_SIZE],
                                                tpm2_command_get_size (command) - TPM_HEADER_SIZE);
    if (entry == NULL) {
        g_debug ("%s: Tpm2Command contains unknown TPMS_CONTEXT.", __func__);
        goto out;
    }
    conn_cmd = tpm2_command_get_connection (command);
    conn_entry = session_entry_get_connection (entry);
    if (conn_cmd != conn_entry) {
        if (!session_list_claim (resmgr->session_list, entry, conn_cmd)) {
            goto out;
        }
    }
    session_entry_set_state (entry, SESSION_ENTRY_SAVED_RM);
    g_debug ("%s: SessionEntry context savedHandle: 0x%08" PRIx32, __func__,
             session_entry_get_handle (entry));
    response = tpm2_response_new_context_load (conn_cmd, entry);
out:
    g_debug ("%s: returning Tpm2Response", __func__);
    g_clear_object (&conn_cmd);
    g_clear_object (&conn_entry);
    g_clear_object (&entry);
    return response;
}
/*
 * This function performs the special processing associated with the
 * TPM2_ContextLoad command.
 */
Tpm2Response*
resource_manager_load_context (ResourceManager *resmgr,
                               Tpm2Command *command)
{
    /* why all the marshalling */
    uint8_t *buf = tpm2_command_get_buffer (command);
    TPMS_CONTEXT tpms_context;
    TSS2_RC rc;
    size_t offset = TPM_HEADER_SIZE;

    /* Need to be able to get handle from Tpm2Command */
    rc = Tss2_MU_TPMS_CONTEXT_Unmarshal (buf,
                                         tpm2_command_get_size (command),
                                         &offset,
                                         &tpms_context);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: Failed to unmarshal TPMS_CONTEXT from Tpm2Command, "
                   "rc: 0x%" PRIx32, __func__, rc);
        /* Generate Tpm2Response with "appropriate" RC */
    }
    switch (tpms_context.savedHandle >> TPM2_HR_SHIFT) {
    case TPM2_HT_HMAC_SESSION:
    case TPM2_HT_POLICY_SESSION:
        return resource_manager_load_context_session (resmgr, command);
    default:
        g_debug ("%s: not virtualizing TPM2_ContextLoad for "
                 "handles: 0x%08" PRIx32, __func__, tpms_context.savedHandle);
        break;
    }

    return NULL;
}
/*
 * This function performs the special processing associated with the
 * TPM2_FlushContext command. How much we can "virtualize" of this command
 * depends on the parameters / handle type.
 *
 * Transient objects that are tracked by the RM (stored in the transient
 * HandleMap in the Connection object) then we can simply delete the mapping
 * since transient objects are always saved / flushed after each command is
 * processed. So for this handle type we just delete the mapping, create a
 * Tpm2Response object and return it to the caller.
 *
 * Session objects are not so simple. Sessions cannot be flushed after each
 * use. The TPM will only allow us to save the context as it must maintain
 * some internal tracking information. So when the caller wishes to flush a
 * session context we must remove the entry tracking the mapping between
 * session and connection from the session_slist (if any exists). The comman
 * must then be passed to the TPM as well. We return NULL here to let the
 * caller know.
 */
Tpm2Response*
resource_manager_flush_context (ResourceManager *resmgr,
                                Tpm2Command     *command)
{
    Connection     *connection;
    HandleMap      *map;
    HandleMapEntry *entry;
    Tpm2Response   *response = NULL;
    TPM2_HANDLE      handle;
    TPM2_HT          handle_type;
    TSS2_RC         rc;

    if (tpm2_command_get_code (command) != TPM2_CC_FlushContext) {
        g_warning ("resource_manager_flush_context with wrong command");
        return NULL;
    }
    rc = tpm2_command_get_flush_handle (command, &handle);
    if (rc != TSS2_RC_SUCCESS) {
        connection = tpm2_command_get_connection (command);
        response = tpm2_response_new_rc (connection, rc);
        g_object_unref (connection);
        goto out;
    }
    g_debug ("resource_manager_flush_context handle: 0x%" PRIx32, handle);
    handle_type = handle >> TPM2_HR_SHIFT;
    switch (handle_type) {
    case TPM2_HT_TRANSIENT:
        g_debug ("handle is TPM2_HT_TRANSIENT, virtualizing");
        connection = tpm2_command_get_connection (command);
        map = connection_get_trans_map (connection);
        entry = handle_map_vlookup (map, handle);
        if (entry != NULL) {
            handle_map_remove (map, handle);
            g_object_unref (entry);
            rc = TSS2_RC_SUCCESS;
        } else {
            /*
             * If the handle doesn't map to a HandleMapEntry then it's not one
             * that we're managing and so we can't flush it. Return an error
             * indicating that error is related to a handle, that it's a parameter
             * and that it's the first parameter.
             */
            rc = RM_RC (TPM2_RC_HANDLE + TPM2_RC_P + TPM2_RC_1);
        }
        g_object_unref (map);
        response = tpm2_response_new_rc (connection, rc);
        g_object_unref (connection);
        break;
    case TPM2_HT_HMAC_SESSION:
    case TPM2_HT_POLICY_SESSION:
        g_debug ("%s: handle 0x%08" PRIx32 "is a session, removing from "
                 "SessionList", __func__, handle);
        session_list_remove_handle (resmgr->session_list, handle);
        break;
    }

out:
    return response;
}
/*
 * Ensure that executing the provided command will not exceed any of the
 * per-connection quotas enforced by the RM. This is currently limited to
 * transient objects and sessions.
 */
TSS2_RC
resource_manager_quota_check (ResourceManager *resmgr,
                              Tpm2Command     *command)
{
    HandleMap   *handle_map = NULL;
    Connection  *connection = NULL;
    TSS2_RC      rc = TSS2_RC_SUCCESS;

    switch (tpm2_command_get_code (command)) {
    /* These commands load transient objects. */
    case TPM2_CC_CreatePrimary:
    case TPM2_CC_Load:
    case TPM2_CC_LoadExternal:
        connection = tpm2_command_get_connection (command);
        handle_map = connection_get_trans_map (connection);
        if (handle_map_is_full (handle_map)) {
            g_info ("%s: Connection has exceeded transient object limit",
                    __func__);
            rc = TSS2_RESMGR_RC_OBJECT_MEMORY;
        }
        break;
    /* These commands create sessions. */
    case TPM2_CC_StartAuthSession:
        connection = tpm2_command_get_connection (command);
        if (session_list_is_full (resmgr->session_list, connection)) {
            g_info ("%s: Connectionhas exceeded session limit", __func__);
            rc = TSS2_RESMGR_RC_SESSION_MEMORY;
        }
        break;
    }
    g_clear_object (&connection);
    g_clear_object (&handle_map);

    return rc;
}
/*
 * This is a callback function invoked by the GSList foreach function. It is
 * called when the object associated with a HandleMapEntry is no longer valid
 * (usually when it is flushed) and the HandleMap shouldn't be tracking it.
 */
void
remove_entry_from_handle_map (gpointer data_entry,
                              gpointer data_connection)
{
    HandleMapEntry  *entry       = HANDLE_MAP_ENTRY (data_entry);
    Connection      *connection  = CONNECTION (data_connection);
    HandleMap       *map         = connection_get_trans_map (connection);
    TPM2_HANDLE       handle      = handle_map_entry_get_vhandle (entry);
    TPM2_HT           handle_type = 0;

    handle_type = handle >> TPM2_HR_SHIFT;
    g_debug ("remove_entry_from_handle_map");
    switch (handle_type) {
    case TPM2_HT_TRANSIENT:
        g_debug ("%s: entry is transient, removing from map", __func__);
        handle_map_remove (map, handle);
        break;
    default:
        g_debug ("%s: entry not transient, leaving entry alone", __func__);
        break;
    }
}
/*
 * This function handles the required post-processing on the HandleMapEntry
 * objects in the GSList that represent objects loaded into the TPM as part of
 * executing a command.
 */
void
post_process_loaded_transients (ResourceManager  *resmgr,
                                GSList          **transient_slist,
                                Connection       *connection,
                                TPMA_CC           command_attrs)
{
    /* if flushed bit is clear we need to flush & save contexts */
    if (!(command_attrs & TPMA_CC_FLUSHED)) {
        g_debug ("flushsave_context for %" PRIu32 " entries",
                 g_slist_length (*transient_slist));
        g_slist_foreach (*transient_slist,
                        resource_manager_flushsave_context,
                        resmgr);
    } else {
        /*
         * if flushed bit is set the transient object entry has been flushed
         * and so we just remove it
         */
        g_debug ("TPMA_CC flushed bit set");
        g_slist_foreach (*transient_slist,
                         remove_entry_from_handle_map,
                         connection);
    }
    g_slist_free_full (*transient_slist, g_object_unref);
}
/*
 * This structure is used to keep state while iterating over a list of
 * TPM2_HANDLES.
 * 'cap_data'    : this parameter is used to collect the list of handles that
 *   we want as well as the number of handles in the structure
 * 'max_count'   : is the maximum number of handles to return
 * 'more_data'   : once max_count handles have been collected this variable
 *   tells the caller whether additional handles would have been returned had
 *   max_count been larger
 * 'start_handle': is the numerically smallest handle that should be collected
 *   into cap_data.
 */
typedef struct {
    TPMS_CAPABILITY_DATA *cap_data;
    size_t                max_count;
    gboolean              more_data;
    TPM2_HANDLE            start_handle;
} vhandle_iterator_state_t;
/*
 * This callback function is invoked as part of iterating over a list of
 * handles. The first parameter is an entry from the collection being
 * traversed. The second is a reference to a vhandle_iterator_state_t
 * structure.
 * This structure is used to maintain state while iterating over the
 * collection.
 */
void
vhandle_iterator_callback (gpointer entry,
                           gpointer data)
{
    TPM2_HANDLE                vhandle  = (uintptr_t)entry;
    vhandle_iterator_state_t *state    = (vhandle_iterator_state_t*)data;
    TPMS_CAPABILITY_DATA     *cap_data = state->cap_data;

    /* if vhandle is numerically smaller than the start value just return */
    if (vhandle < state->start_handle) {
        return;
    }
    g_debug ("vhandle_iterator_callback with max_count: %zu and count: %"
             PRIu32, state->max_count, cap_data->data.handles.count);
    /* if we've collected max_count handles set 'more_data' and return */
    if (!(cap_data->data.handles.count < state->max_count)) {
        state->more_data = TRUE;
        return;
    }
    cap_data->data.handles.handle [cap_data->data.handles.count] = vhandle;
    ++cap_data->data.handles.count;
}
/*
 * This is a GCompareFunc used to sort a list of TPM2_HANDLES.
 */
int
handle_compare (gconstpointer a,
                gconstpointer b)
{
    TPM2_HANDLE handle_a = (uintptr_t)a;
    TPM2_HANDLE handle_b = (uintptr_t)b;

    if (handle_a < handle_b) {
        return -1;
    } else if (handle_a > handle_b) {
        return 1;
    } else {
        return 0;
    }
}
/*
 * The get_cap_transient function populates a TPMS_CAPABILITY_DATA structure
 * with the handles in the provided HandleMap 'map'. The 'prop' parameter
 * is the lowest numerical handle to return. The 'count' parameter is the
 * maximum number of handles to return in the capability data structure.
 * Returns:
 *   TRUE when more handles are present
 *   FALSE when there are no more handles
 */
gboolean
get_cap_handles (HandleMap            *map,
                 TPM2_HANDLE            prop,
                 UINT32                count,
                 TPMS_CAPABILITY_DATA *cap_data)
{
    GList *vhandle_list;
    vhandle_iterator_state_t state = {
        .cap_data     = cap_data,
        .max_count    = count,
        .more_data    = FALSE,
        .start_handle = prop,
    };

    cap_data->capability = TPM2_CAP_HANDLES;
    cap_data->data.handles.count = 0;

    vhandle_list = handle_map_get_keys (map);
    vhandle_list = g_list_sort (vhandle_list, handle_compare);
    g_list_foreach (vhandle_list, vhandle_iterator_callback, &state);

    g_debug ("iterating over %" PRIu32 " vhandles from g_list_foreach",
             cap_data->data.handles.count);
    size_t i;
    for (i = 0; i < cap_data->data.handles.count; ++i) {
        g_debug ("  vhandle: 0x%" PRIx32, cap_data->data.handles.handle [i]);
    }

    return state.more_data;
}
/*
 * These macros are used to set fields in a Tpm2Response buffer that we
 * create in response to the TPM2 GetCapability command. They are very
 * specifically tailored and should not be used elsewhere.
 */
#define YES_NO_OFFSET TPM_HEADER_SIZE
#define YES_NO_SET(buffer, value) \
    (*(TPMI_YES_NO*)(buffer + YES_NO_OFFSET) = value)
#define CAP_OFFSET (TPM_HEADER_SIZE + sizeof (TPMI_YES_NO))
#define CAP_SET(buffer, value) \
    (*(TPM2_CAP*)(buffer + CAP_OFFSET) = htobe32 (value))
#define HANDLE_COUNT_OFFSET (CAP_OFFSET + sizeof (TPM2_CAP))
#define HANDLE_COUNT_SET(buffer, value) \
    (*(UINT32*)(buffer + HANDLE_COUNT_OFFSET) = htobe32 (value))
#define HANDLE_OFFSET (HANDLE_COUNT_OFFSET + sizeof (UINT32))
#define HANDLE_INDEX(i) (sizeof (TPM2_HANDLE) * i)
#define HANDLE_SET(buffer, i, value) \
    (*(TPM2_HANDLE*)(buffer + HANDLE_OFFSET + HANDLE_INDEX (i)) = \
        htobe32 (value))
#define CAP_RESP_SIZE(value) \
    (TPM_HEADER_SIZE + \
     sizeof (TPMI_YES_NO) + \
     sizeof ((value)->capability) + \
     sizeof ((value)->data.handles.count) + \
     ((value)->data.handles.count * \
      sizeof ((value)->data.handles.handle [0])))
/*
 * This function is used to build a response buffer that contains the provided
 * TPMS_CAPABILITY_DATA and TPMI_YES_NO. These are the two response parameters
 * to the TPM2_GetCapability function.
 * The 'cap_data' parameter *must* have the TPMU_CAPABILITY union populated
 * with the TPM2_CAP_HANDLES selector.
 */
uint8_t*
build_cap_handles_response (TPMS_CAPABILITY_DATA *cap_data,
                            TPMI_YES_NO           more_data)
{
    size_t i;
    uint8_t *buf;

    buf = calloc (1, CAP_RESP_SIZE (cap_data));
    if (buf == NULL) {
        tabrmd_critical ("failed to allocate buffer for handle capability "
                         "response");
    }
    set_response_tag (buf, TPM2_ST_NO_SESSIONS);
    set_response_size (buf, CAP_RESP_SIZE (cap_data));
    set_response_code (buf, TSS2_RC_SUCCESS);
    YES_NO_SET (buf, more_data);
    CAP_SET (buf, cap_data->capability);
    HANDLE_COUNT_SET (buf, cap_data->data.handles.count);
    for (i = 0; i < cap_data->data.handles.count; ++i) {
        HANDLE_SET (buf, i, cap_data->data.handles.handle [i]);
    }

    return buf;
}
/*
 * In cases where the GetCapability command isn't fully virtualized we may
 * need to perform some 'post processing' of the results returned from the
 * TPM2 device. Specifically, in the case of the TPM2_PT_CONTEXT_GAP_MAX
 * property, we overrite the value in the response body. This function
 * manually unmarshals the response, iterates over the values modifying
 * them if necessary and then marshals them back into the Tpm2Response.
 */
TSS2_RC
get_cap_post_process (Tpm2Response *resp)
{
    g_assert (resp != NULL);
    g_assert (tpm2_response_get_code (resp) == TSS2_RC_SUCCESS);

    TPMS_CAPABILITY_DATA cap_data = { .capability = 0 };
    TSS2_RC rc;
    uint8_t *buf = tpm2_response_get_buffer (resp);
    size_t buf_size = tpm2_response_get_size (resp);
    size_t offset = TPM_HEADER_SIZE + sizeof (TPMI_YES_NO);
    size_t i;

    rc = Tss2_MU_TPMS_CAPABILITY_DATA_Unmarshal (buf,
                                                 buf_size,
                                                 &offset,
                                                 &cap_data);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: Failed to unmarshal TPMS_CAPABILITY_DATA", __func__);
        return rc;
    }
    g_debug ("%s: capability 0x%" PRIx32, __func__, cap_data.capability);
    switch (cap_data.capability) {
    case TPM2_CAP_TPM_PROPERTIES:
        for (i = 0; i < cap_data.data.tpmProperties.count; ++i) {
            g_debug ("%s: property 0x%" PRIx32 ", value 0x%" PRIx32,
                     __func__,
                     cap_data.data.tpmProperties.tpmProperty [i].property,
                     cap_data.data.tpmProperties.tpmProperty [i].value);
            switch (cap_data.data.tpmProperties.tpmProperty [i].property) {
            case TPM2_PT_CONTEXT_GAP_MAX:
                g_debug ("%s: changing TPM2_PT_CONTEXT_GAP_MAX, from 0x%"
                         PRIx32 " to UINT32_MAX: 0x%" PRIx32, __func__,
                         cap_data.data.tpmProperties.tpmProperty [i].value,
                         UINT32_MAX);
                cap_data.data.tpmProperties.tpmProperty [i].value = UINT32_MAX;
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    offset = TPM_HEADER_SIZE + sizeof (TPMI_YES_NO);
    rc = Tss2_MU_TPMS_CAPABILITY_DATA_Marshal (&cap_data,
                                               buf,
                                               buf_size,
                                               &offset);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("%s: Failed to unmarshal TPMS_CAPABILITY_DATA", __func__);
        return rc;
    }
    return rc;
}
/*
 * This function takes a Tpm2Command and the associated connection object
 * as parameters. The Tpm2Command *must* have the 'code' attribute set to
 * TPM2_CC_GetCapability. If it's a GetCapability command that we
 * "virtualize" then we'll build a Tpm2Response object and return it. If
 * not, then we send the command to the TPM2 device and perform whatever
 * post-processing is necessary.
 */
Tpm2Response*
get_cap_gen_response (ResourceManager *resmgr,
                      Tpm2Command *command)
{
    TPM2_CAP  cap         = tpm2_command_get_cap (command);
    UINT32   prop        = tpm2_command_get_prop (command);
    UINT32   prop_count  = tpm2_command_get_prop_count (command);
    TPM2_HT   handle_type;
    TSS2_RC rc = TSS2_RC_SUCCESS;
    Connection *connection = NULL;
    HandleMap *map;
    TPMS_CAPABILITY_DATA cap_data = { .capability = cap };
    gboolean more_data = FALSE;
    uint8_t *resp_buf;
    Tpm2Response *response = NULL;

    g_debug ("processing TPM2_CC_GetCapability with cap: 0x%" PRIx32
             " prop: 0x%" PRIx32 " prop_count: 0x%" PRIx32,
             cap, prop, prop_count);
    switch (cap) {
    case TPM2_CAP_HANDLES:
        handle_type = prop >> TPM2_HR_SHIFT;
        switch (handle_type) {
        case TPM2_HT_TRANSIENT:
            g_debug ("%s: TPM2_CAP_HANDLES && TPM2_HT_TRANSIENT", __func__);
            connection = tpm2_command_get_connection (command);
            map = connection_get_trans_map (connection);
            more_data = get_cap_handles (map,  prop, prop_count, &cap_data);
            g_object_unref (map);
            resp_buf = build_cap_handles_response (&cap_data, more_data);
            response = tpm2_response_new (connection,
                                          resp_buf,
                                          CAP_RESP_SIZE (&cap_data),
                                          tpm2_command_get_attributes (command));
            break;
        default:
            g_debug ("%s: TPM2_CAP_HANDLES not virtualized for handle type: "
                     "0x%" PRIx32, __func__, handle_type);
            break;
        }
        break;
    default:
        g_debug ("%s: cap 0x%" PRIx32 " not handled", __func__, cap);
        break;
    }

    if (response == NULL) {
        response = tpm2_send_command (resmgr->tpm2, command, &rc);
        if (response != NULL && rc == TSS2_RC_SUCCESS) {
            get_cap_post_process (response);
        }
    }

    g_clear_object (&connection);
    return response;
}
/*
 * If the provided command is something that the ResourceManager "virtualizes"
 * then this function will do so and return a Tpm2Response object that will be
 * returned to the same connection. If the command isn't something that we
 * virtualize then we just return NULL.
 *
 * NOTE: Both the ContextSave and ContextLoad commands, when sent by clients,
 * do not result in the command being executed by the TPM. This makes
 * handling the context gap unnecessary for these commands.
 */
Tpm2Response*
command_special_processing (ResourceManager *resmgr,
                            Tpm2Command     *command)
{
    Tpm2Response *response   = NULL;

    switch (tpm2_command_get_code (command)) {
    case TPM2_CC_FlushContext:
        g_debug ("processing TPM2_CC_FlushContext");
        response = resource_manager_flush_context (resmgr, command);
        break;
    case TPM2_CC_ContextSave:
        g_debug ("processing TPM2_CC_ContextSave");
        response = resource_manager_save_context (resmgr, command);
        break;
    case TPM2_CC_ContextLoad:
        g_debug ("%s: processing TPM2_CC_ContextLoad", __func__);
        response = resource_manager_load_context (resmgr, command);
        break;
    case TPM2_CC_GetCapability:
        g_debug ("processing TPM2_CC_GetCapability");
        response = get_cap_gen_response (resmgr, command);
        break;
    default:
        break;
    }

    return response;
}
/*
 * This function creates a mapping from the transient physical to a virtual
 * handle in the provided response object. This mapping is then added to
 * the transient HandleMap for the associated connection, as well as the
 * list of currently loaded transient objects.
 */
void
create_context_mapping_transient (ResourceManager  *resmgr,
                                  Tpm2Response     *response,
                                  GSList          **loaded_transient_slist)
{
    HandleMap      *handle_map;
    HandleMapEntry *handle_entry;
    TPM2_HANDLE      phandle, vhandle;
    Connection     *connection;
    UNUSED_PARAM(resmgr);

    g_debug ("create_context_mapping_transient");
    phandle = tpm2_response_get_handle (response);
    g_debug ("  physical handle: 0x%08" PRIx32, phandle);
    connection = tpm2_response_get_connection (response);
    handle_map = connection_get_trans_map (connection);
    g_object_unref (connection);
    vhandle = handle_map_next_vhandle (handle_map);
    if (vhandle == 0) {
        g_error ("vhandle rolled over!");
    }
    g_debug ("  vhandle:0x%08" PRIx32, vhandle);
    handle_entry = handle_map_entry_new (phandle, vhandle);
    if (handle_entry == NULL) {
        g_warning ("failed to create new HandleMapEntry for handle 0x%"
                   PRIx32, phandle);
    }
    *loaded_transient_slist = g_slist_prepend (*loaded_transient_slist,
                                               handle_entry);
    handle_map_insert (handle_map, vhandle, handle_entry);
    g_object_unref (handle_map);
    tpm2_response_set_handle (response, vhandle);
    g_object_ref (handle_entry);
}
/*
 * This function after a Tpm2Command is sent to the TPM and:
 * - we receive a Tpm2Response object with a handle in the response buffers
 *   handle area
 * - the handle is a session handle
 * Since there's a handle in the response handle area the caller is being
 * returned a new handle after a context was successfully created or loaded.
 * So we know that the response is to one of two possible commands:
 * - a session being loaded by a call to LoadContext
 * - a session was newly created by a call to StartAuthSession
 * We differentiate between these two situations as follows:
 * - A call to 'LoadContext' implies that the session already exists and so
 *   it must already be in the session_list.
 *   - If it's in the session_list *AND NOT* in the abandoned_session_queue
 *     then the caller is just loading a context they already own and so we
 *     set the session state to SAVED_RM and add the session to the list of
 *     loaded sessions.
 *   - If it's *NOT* in the session_list *AND* in the abandoned_session_queue
 *     then the caller is loading a context saved by a different connection
 *     and so we make the current connection the owner of the session, set
 *     the session state to SAVED_RM and add the session to the list of loaded
 *     sessions.
 * - A call to 'StartAuthSession' will return a handle for a session object
 *   that is not in either the session_list or the abandoned_session_queue.
 *   In this case we just create a new SessionEntry and add it to the
 *   session_list and the list of loaded sessions.
 * NOTE: If the response doesn't indicate 'success' then we just ignore it
 * since there's nothing useful that we can do.
 */
void
create_context_mapping_session (ResourceManager *resmgr,
                                Tpm2Response    *response,
                                TPM2_HANDLE      handle)
{
    SessionEntry *entry = NULL;
    Connection   *conn_resp = NULL, *conn_entry = NULL;

    entry = session_list_lookup_handle (resmgr->session_list, handle);
    conn_resp = tpm2_response_get_connection (response);
    if (entry != NULL) {
        g_debug ("%s: got SessionEntry that's in the SessionList", __func__);
        conn_entry = session_entry_get_connection (entry);
        if (conn_resp != conn_entry) {
            g_warning ("%s: connections do not match!", __func__);
        }
    } else {
        g_debug ("%s: handle is a session, creating entry for SessionList "
                 "and SessionList", __func__);
        entry = session_entry_new (conn_resp, handle);
        session_entry_set_state (entry, SESSION_ENTRY_LOADED);
        session_list_insert (resmgr->session_list, entry);
    }
    g_clear_object (&conn_resp);
    g_clear_object (&conn_entry);
    g_clear_object (&entry);
}
/*
 * Each Tpm2Response object can have at most one handle in it.
 * This function assumes that the handle in the parameter Tpm2Response
 * object is the handle assigned by the TPM. Depending on the type of the
 * handle we do to possible things:
 * For TPM2_HT_TRANSIENT handles we create a new virtual handle and
 * allocates a new HandleMapEntry to map the virtual handle to a
 * TPMS_CONTEXT structure when processing future commands associated
 * with the same connection. This HandleMapEntry is inserted into the
 * handle map for the connection. It is also added to the list of loaded
 * transient objects so that it can be saved / flushed by the typical code
 * path.
 * For TPM2_HT_HMAC_SESSION or TPM2_HT_POLICY_SESSION handles we create a
 * new session_entry_t object, populate the connection field with the
 * connection associated with the response object, and set the savedHandle
 * field. We then add this entry to the list of sessions we're tracking
 * (session_slist) and the list of loaded sessions (loaded_session_slist).
 */
void
resource_manager_create_context_mapping (ResourceManager  *resmgr,
                                         Tpm2Response     *response,
                                         GSList          **loaded_transient_slist)
{
    TPM2_HANDLE       handle;

    g_debug ("%s", __func__);
    if (!tpm2_response_has_handle (response)) {
        g_debug ("response has no handles");
        return;
    }
    handle = tpm2_response_get_handle (response);
    switch (handle >> TPM2_HR_SHIFT) {
    case TPM2_HT_TRANSIENT:
        create_context_mapping_transient (resmgr, response, loaded_transient_slist);
        break;
    case TPM2_HT_HMAC_SESSION:
    case TPM2_HT_POLICY_SESSION:
        create_context_mapping_session (resmgr, response, handle);
        break;
    default:
        g_debug ("  not creating context for handle: 0x%08" PRIx32, handle);
        break;
    }
}
Tpm2Response*
send_command_handle_rc (ResourceManager *resmgr,
                        Tpm2Command *cmd)
{
    regap_session_data_t data = {
        .resmgr = resmgr,
        .ret = TRUE,
    };
    Tpm2Response *resp = NULL;
    TSS2_RC rc;

    /* Send command and create response object. */
    resp = tpm2_send_command (resmgr->tpm2, cmd, &rc);
    rc = tpm2_response_get_code (resp);
    if (rc == TPM2_RC_CONTEXT_GAP) {
        g_debug ("%s: handling TPM2_RC_CONTEXT_GAP", __func__);
        session_list_foreach (resmgr->session_list,
                              regap_session_callback,
                              &data);
        g_clear_object (&resp);
        resp = tpm2_send_command (resmgr->tpm2, cmd, &rc);
    }
    return resp;
}
/**
 * This function is invoked in response to the receipt of a Tpm2Command.
 * This is the place where we send the command buffer out to the TPM
 * through the Tpm2 which will eventually get it to the TPM for us.
 * The Tpm2 will send us back a Tpm2Response that we send back to
 * the client by way of our Sink object. The flow is roughly:
 * - Receive the Tpm2Command as a parameter
 * - Load all virtualized objects required by the command.
 * - Send the Tpm2Command out through the Tpm2.
 * - Receive the response from the Tpm2.
 * - Virtualize the new objects created by the command & referenced in the
 *   response.
 * - Enqueue the response back out to the processing pipeline through the
 *   Sink object.
 * - Flush all objects loaded for the command or as part of executing the
 *   command..
 */
void
resource_manager_process_tpm2_command (ResourceManager   *resmgr,
                                       Tpm2Command       *command)
{
    Connection    *connection;
    Tpm2Response   *response;
    TSS2_RC         rc = TSS2_RC_SUCCESS;
    GSList         *transient_slist = NULL;
    TPMA_CC         command_attrs;

    command_attrs = tpm2_command_get_attributes (command);
    g_debug ("%s", __func__);
    dump_command (command);
    connection = tpm2_command_get_connection (command);
    /* If executing the command would exceed a per connection quota */
    rc = resource_manager_quota_check (resmgr, command);
    if (rc != TSS2_RC_SUCCESS) {
        response = tpm2_response_new_rc (connection, rc);
        goto send_response;
    }
    /* Do command-specific processing. */
    response = command_special_processing (resmgr, command);
    if (response != NULL) {
        goto send_response;
    }
    /* Load objects associated with the handles in the command handle area. */
    if (tpm2_command_get_handle_count (command) > 0) {
        resource_manager_load_handles (resmgr,
                                       command,
                                       &transient_slist);
    }
    /* Load objets associated with the authorizations in the command. */
    if (tpm2_command_has_auths (command)) {
        g_info ("%s, Processing auths for command", __func__);
        auth_callback_data_t auth_callback_data = {
            .resmgr = resmgr,
            .command = command,
        };
        tpm2_command_foreach_auth (command,
                                   resource_manager_load_auth_callback,
                                   &auth_callback_data);
    }
    /* Send command and create response object. */
    response = send_command_handle_rc (resmgr, command);
    dump_response (response);
    /* transform virtualized handles in Tpm2Response if necessary */
    resource_manager_create_context_mapping (resmgr,
                                             response,
                                             &transient_slist);
send_response:
    sink_enqueue (resmgr->sink, G_OBJECT (response));
    g_object_unref (response);
    /* save contexts that were previously loaded */
    session_list_foreach (resmgr->session_list,
                          save_session_callback,
                          resmgr);
    post_process_loaded_transients (resmgr, &transient_slist, connection, command_attrs);
    g_object_unref (connection);
    return;
}
/*
 * Return FALSE to terminate main thread.
 */
gboolean
resource_manager_process_control (ResourceManager *resmgr,
                                  ControlMessage *msg)
{
    ControlCode code = control_message_get_code (msg);
    Connection *conn;

    g_debug ("%s", __func__);
    switch (code) {
    case CHECK_CANCEL:
        sink_enqueue (resmgr->sink, G_OBJECT (msg));
        return FALSE;
    case CONNECTION_REMOVED:
        conn = CONNECTION (control_message_get_object (msg));
        g_debug ("%s: received CONNECTION_REMOVED message for connection",
                 __func__);
        resource_manager_remove_connection (resmgr, conn);
        sink_enqueue (resmgr->sink, G_OBJECT (msg));
        return TRUE;
    default:
        g_warning ("%s: Unknown control code: %d ... ignoring",
                   __func__, code);
        return TRUE;
    }
}
/**
 * This function acts as a thread. It simply:
 * - Blocks on the in_queue. Then wakes up and
 * - Dequeues a message from the in_queue.
 * - Processes the message (depending on TYPE)
 * - Does it all over again.
 */
gpointer
resource_manager_thread (gpointer data)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (data);
    GObject         *obj = NULL;
    gboolean done = FALSE;

    g_debug ("resource_manager_thread start");
    while (!done) {
        obj = message_queue_dequeue (resmgr->in_queue);
        g_debug ("%s: message_queue_dequeue got obj", __func__);
        if (obj == NULL) {
            g_debug ("%s: dequeued a null object", __func__);
            break;
        }
        if (IS_TPM2_COMMAND (obj)) {
            resource_manager_process_tpm2_command (resmgr, TPM2_COMMAND (obj));
        } else if (IS_CONTROL_MESSAGE (obj)) {
            gboolean ret =
                resource_manager_process_control (resmgr, CONTROL_MESSAGE (obj));
            if (ret == FALSE) {
                done = TRUE;
            }
        }
        g_object_unref (obj);
    }

    return NULL;
}
static void
resource_manager_unblock (Thread *self)
{
    ControlMessage *msg;
    ResourceManager *resmgr = RESOURCE_MANAGER (self);

    if (resmgr == NULL)
        g_error ("resource_manager_cancel passed NULL ResourceManager");
    msg = control_message_new (CHECK_CANCEL);
    g_debug ("%s: enqueuing ControlMessage", __func__);
    message_queue_enqueue (resmgr->in_queue, G_OBJECT (msg));
    g_object_unref (msg);
}
/**
 * Implement the 'enqueue' function from the Sink interface. This is how
 * new messages / commands get into the Tpm2.
 */
void
resource_manager_enqueue (Sink        *sink,
                          GObject     *obj)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (sink);

    g_debug ("%s", __func__);
    message_queue_enqueue (resmgr->in_queue, obj);
}
/**
 * Implement the 'add_sink' function from the SourceInterface. This adds a
 * reference to an object that implements the SinkInterface to this objects
 * internal structure. We pass it data.
 */
void
resource_manager_add_sink (Source *self,
                           Sink   *sink)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (self);
    GValue value = G_VALUE_INIT;

    g_debug ("%s", __func__);
    g_value_init (&value, G_TYPE_OBJECT);
    g_value_set_object (&value, sink);
    g_object_set_property (G_OBJECT (resmgr), "sink", &value);
    g_value_unset (&value);
}
/**
 * GObject property setter.
 */
static void
resource_manager_set_property (GObject        *object,
                               guint           property_id,
                               GValue const   *value,
                               GParamSpec     *pspec)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (object);

    g_debug ("%s", __func__);
    switch (property_id) {
    case PROP_QUEUE_IN:
        resmgr->in_queue = g_value_get_object (value);
        break;
    case PROP_SINK:
        if (resmgr->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        resmgr->sink = SINK (g_value_get_object (value));
        g_object_ref (resmgr->sink);
        break;
    case PROP_TPM2:
        if (resmgr->tpm2 != NULL) {
            g_warning ("  tpm2 already set");
            break;
        }
        resmgr->tpm2 = g_value_get_object (value);
        g_object_ref (resmgr->tpm2);
        break;
    case PROP_SESSION_LIST:
        resmgr->session_list = SESSION_LIST (g_value_dup_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * GObject property getter.
 */
static void
resource_manager_get_property (GObject     *object,
                               guint        property_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (object);

    g_debug ("%s", __func__);
    switch (property_id) {
    case PROP_QUEUE_IN:
        g_value_set_object (value, resmgr->in_queue);
        break;
    case PROP_SINK:
        g_value_set_object (value, resmgr->sink);
        break;
    case PROP_TPM2:
        g_value_set_object (value, resmgr->tpm2);
        break;
    case PROP_SESSION_LIST:
        g_value_set_object (value, resmgr->session_list);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/**
 * Bring down the ResourceManager as gracefully as we can.
 */
static void
resource_manager_dispose (GObject *obj)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (obj);
    Thread *thread = THREAD (obj);

    g_debug ("%s", __func__);
    if (resmgr == NULL)
        g_error ("%s: passed NULL parameter", __func__);
    if (thread->thread_id != 0)
        g_error ("%s: thread running, cancel thread first", __func__);
    g_clear_object (&resmgr->in_queue);
    g_clear_object (&resmgr->sink);
    g_clear_object (&resmgr->tpm2);
    g_clear_object (&resmgr->session_list);
    G_OBJECT_CLASS (resource_manager_parent_class)->dispose (obj);
}
static void
resource_manager_init (ResourceManager *manager)
{
    UNUSED_PARAM (manager);
}
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set dispose, property get/set.
 * - Install properties.
 */
static void
resource_manager_class_init (ResourceManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ThreadClass  *thread_class = THREAD_CLASS (klass);

    if (resource_manager_parent_class == NULL)
        resource_manager_parent_class = g_type_class_peek_parent (klass);
    object_class->dispose = resource_manager_dispose;
    object_class->get_property = resource_manager_get_property;
    object_class->set_property = resource_manager_set_property;
    thread_class->thread_run     = resource_manager_thread;
    thread_class->thread_unblock = resource_manager_unblock;

    obj_properties [PROP_QUEUE_IN] =
        g_param_spec_object ("queue-in",
                             "input queue",
                             "Input queue for messages.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SINK] =
        g_param_spec_object ("sink",
                             "Sink",
                             "Reference to a Sink object that we pass messages to.",
                             G_TYPE_OBJECT,
                             G_PARAM_READWRITE);
    obj_properties [PROP_TPM2] =
        g_param_spec_object ("tpm2",
                             "Tpm2 object",
                             "Object used to communicate with TPM2",
                             TYPE_TPM2,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties [PROP_SESSION_LIST] =
        g_param_spec_object ("session-list",
                             "SessionList object",
                             "Data structure to hold session tracking data",
                             TYPE_SESSION_LIST,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Boilerplate code to register functions with the SourceInterface.
 */
static void
resource_manager_source_interface_init (gpointer g_iface)
{
    SourceInterface *source = (SourceInterface*)g_iface;
    source->add_sink = resource_manager_add_sink;
}
/**
 * Boilerplate code to register function with the SinkInterface.
 */
static void
resource_manager_sink_interface_init (gpointer g_iface)
{
    SinkInterface *sink = (SinkInterface*)g_iface;
    sink->enqueue = resource_manager_enqueue;
}
/*
 * A callback function implementing the PruneFunc type for use with the
 * session_list_prune_abaonded function.
 */
gboolean
flush_session_callback (SessionEntry *entry,
                        gpointer data)
{
    return flush_session (RESOURCE_MANAGER (data), entry);
}
/*
 * This structure is used to pass required data into the
 * connection_close_session_callback function.
 */
typedef struct {
    Connection *connection;
    ResourceManager *resource_manager;
} connection_close_data_t;
/*
 * This is a callback function invoked foreach SessionEntry in the SessionList
 * when a client Connection is closed. As it iterates over each SessionEntry,
 * it identifies sessions associated with the connection being closed and,
 * performs some task depending on its state.
 *
 * If session is in state SESSION_ENTRY_SAVED_CLIENT:
 * - take a reference to the SessionEntry
 * - remove SessionEntry from session list
 * - change state to SESSION_ENTRY_SAVED_CLIENT_CLOSED
 * - "prune" other abandoned sessions
 * - add SessionEntry to queue of abandoned sessions
 * If session is in state SESSION_ENTRY_SAVED_RM:
 * - flush session from TPM
 * - remove SessionEntry from session list
 * If session is in any other state
 * - panic
 */
void
connection_close_session_callback (gpointer data,
                                   gpointer user_data)
{
    SessionEntry *session_entry = SESSION_ENTRY (data);
    SessionEntryStateEnum session_state = session_entry_get_state (session_entry);
    connection_close_data_t *callback_data = (connection_close_data_t*)user_data;
    Connection *connection = callback_data->connection;
    ResourceManager *resource_manager = callback_data->resource_manager;
    TPM2_HANDLE handle;
    TSS2_RC rc;

    g_debug ("%s", __func__);
    if (session_entry->connection != connection) {
        g_debug ("%s: connection mismatch", __func__);
        return;
    }
    handle = session_entry_get_handle (session_entry);
    g_debug ("%s: SessionEntry is in state %s", __func__,
             session_entry_state_to_str (session_state));
    switch (session_state) {
    case SESSION_ENTRY_SAVED_CLIENT:
        g_debug ("%s: abandoning.", __func__);
        session_list_abandon_handle (resource_manager->session_list,
                                     connection,
                                     handle);
        session_list_prune_abandoned (resource_manager->session_list,
                                      flush_session_callback,
                                      resource_manager);
        break;
    case SESSION_ENTRY_SAVED_RM:
        g_debug ("%s: flushing.", __func__);
        rc = tpm2_context_flush (resource_manager->tpm2,
                                          handle);
        if (rc != TSS2_RC_SUCCESS) {
            g_warning ("%s: failed to flush context", __func__);
        }
        session_list_remove (resource_manager->session_list,
                             session_entry);
        break;
    default:
        /* This is a situation that should never happen */
        g_error ("%s: Connection closed with session in unexpected state: %s",
                 __func__, session_entry_state_to_str (session_state));
        break;
    }
}
/*
 * This function is invoked when a connection is removed from the
 * ConnectionManager. This is if how we know a connection has been closed.
 * When a connection is removed, we need to remove all associated sessions
 * from the TPM.
 */
void
resource_manager_remove_connection (ResourceManager *resource_manager,
                                    Connection *connection)
{
    connection_close_data_t connection_close_data = {
        .connection = connection,
        .resource_manager = resource_manager,
    };

    g_info ("%s: flushing session contexts", __func__);
    session_list_foreach (resource_manager->session_list,
                          connection_close_session_callback,
                          &connection_close_data);
    g_debug ("%s: done", __func__);
}
/**
 * Create new ResourceManager object.
 */
ResourceManager*
resource_manager_new (Tpm2    *tpm2,
                      SessionList     *session_list)
{
    if (tpm2 == NULL)
        g_error ("resource_manager_new passed NULL Tpm2");
    MessageQueue *queue = message_queue_new ();
    return RESOURCE_MANAGER (g_object_new (TYPE_RESOURCE_MANAGER,
                                           "queue-in",        queue,
                                           "tpm2", tpm2,
                                           "session-list",    session_list,
                                           NULL));
}
