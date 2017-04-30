/*
 * Copyright (c) 2017, Intel Corporation
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
#include <errno.h>
#include <inttypes.h>

#include "connection.h"
#include "control-message.h"
#include "logging.h"
#include "message-queue.h"
#include "resource-manager.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "tabrmd.h"
#include "tpm2-header.h"
#include "tpm2-command.h"
#include "tpm2-response.h"
#include "util.h"

#define RM_RC(rc) TSS2_RESMGR_ERROR_LEVEL + rc

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
    PROP_ACCESS_BROKER,
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
    TPM_HANDLE    phandle = 0;
    TPMS_CONTEXT *context;
    TSS2_RC       rc = TSS2_RC_SUCCESS;

    context = handle_map_entry_get_context (entry);
    rc = access_broker_context_load (resmgr->access_broker, context, &phandle);
    g_debug ("phandle: 0x%" PRIx32, phandle);
    if (rc == TSS2_RC_SUCCESS) {
        handle_map_entry_set_phandle (entry, phandle);
        tpm2_command_set_handle (command, phandle, handle_number);
    } else {
        g_warning ("Failed to load context: 0x%" PRIx32, rc);
    }
    return rc;
}
/*
 * This function operates on the provided command. It iterates over each
 * handle in the commands handle area. For each relevant handle it loads
 * the related context and fixes up the handles in the command.
 */
TSS2_RC
resource_manager_load_contexts (ResourceManager *resmgr,
                                Tpm2Command     *command,
                                GSList         **entry_slist)
{
    HandleMap    *map;
    HandleMapEntry *entry;
    Connection  *connection;
    TSS2_RC       rc = TSS2_RC_SUCCESS;
    TPM_HANDLE    handles[3] = { 0, };
    guint8 i, handle_count;;

    g_debug ("resource_manager_load_contexts");
    if (!resmgr || !command) {
        g_warning ("resource_manager_load_contexts received NULL parameter.");
        return RM_RC (TSS2_BASE_RC_GENERAL_FAILURE);
    }
    connection = tpm2_command_get_connection (command);
    handle_count = tpm2_command_get_handle_count (command);
    tpm2_command_get_handles (command, handles, handle_count);
    g_debug ("loading contexts for %" PRId8 " handles", handle_count);
    for (i = 0; i < handle_count; ++i) {
        switch (handles [i] >> HR_SHIFT) {
        case TPM_HT_TRANSIENT:
            g_debug ("processing TPM_HT_TRANSIENT: 0x%" PRIx32, handles [i]);
            map = connection_get_trans_map (connection);
            g_debug ("handle 0x%" PRIx32 " is virtual TPM_HT_TRANSIENT, "
                     "loading", handles [i]);
            entry = handle_map_vlookup (map, handles [i]);
            if (entry) {
                g_debug ("mapped virtual handle 0x%" PRIx32 " to entry 0x%"
                         PRIxPTR, handles [i], (uintptr_t)entry);
            } else {
                g_warning ("No HandleMapEntry for vhandle: 0x%" PRIx32,
                           handles [i]);
                continue;
            }
            g_object_unref (map);
            rc = resource_manager_virt_to_phys (resmgr, command, entry, i);
            if (rc != TSS2_RC_SUCCESS)
                break;
            *entry_slist = g_slist_prepend (*entry_slist, entry);
            break;
        default:
            break;
        }
    }
    g_debug ("resource_manager_load_contexts end");
    g_object_unref (connection);

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
    TPM_HANDLE      phandle;
    TSS2_RC         rc = TSS2_RC_SUCCESS;

    g_debug ("resource_manager_flushsave_context for entry: 0x%" PRIxPTR,
             (uintptr_t)entry);
    if (resmgr == NULL || entry == NULL)
        g_error ("resource_manager_flushsave_context passed NULL parameter");
    phandle = handle_map_entry_get_phandle (entry);
    g_debug ("resource_manager_save_context phandle: 0x%" PRIx32, phandle);
    switch (phandle >> HR_SHIFT) {
    case TPM_HT_TRANSIENT:
        g_debug ("handle is transient, saving context");
        context = handle_map_entry_get_context (entry);
        rc = access_broker_context_saveflush (resmgr->access_broker,
                                              phandle,
                                              context);
        if (rc == TSS2_RC_SUCCESS) {
            handle_map_entry_set_phandle (entry, 0);
        } else {
            g_warning ("access_broker_context_save failed for handle: 0x%"
                       PRIx32 " rc: 0x%" PRIx32, phandle, rc);
        }
        break;
    default:
        break;
    }
}
/*
 * Each Tpm2Response object can have at most one handle in it.
 * This function assumes that the handle in the parameter Tpm2Response
 * object is a physical handle. It creates a new virtual handle and
 * allocates a new HandleMapEntry to map the virtual handle to a
 * TPMS_CONTEXT structure when processing future commands associated
 * with the same connection. This HandleMapEntry is inserted into the
 * handle map for the connection. It is also returned to the caller who
 * must decrement the reference count when they are done with it.
 */
HandleMapEntry*
resource_manager_virtualize_handle (ResourceManager *resmgr,
                                    Tpm2Response    *response)
{
    TPM_HANDLE      phandle, vhandle = 0;
    Connection    *connection;
    HandleMap      *handle_map;
    HandleMapEntry *entry = NULL;

    phandle = tpm2_response_get_handle (response);
    g_debug ("resource_manager_virtualize_handle 0x%" PRIx32, phandle);
    switch (phandle >> HR_SHIFT) {
    case TPM_HT_TRANSIENT:
        g_debug ("handle is transient, virtualizing");
        connection = tpm2_response_get_connection (response);
        handle_map = connection_get_trans_map (connection);
        vhandle = handle_map_next_vhandle (handle_map);
        if (vhandle == 0)
            g_error ("vhandle rolled over!");
        g_debug ("now has vhandle:0x%" PRIx32, vhandle);
        entry = handle_map_entry_new (phandle, vhandle);
        g_debug ("handle map entry: 0x%" PRIxPTR, (uintptr_t)entry);
        handle_map_insert (handle_map, vhandle, entry);
        tpm2_response_set_handle (response, vhandle);
        g_object_unref (connection);
        g_object_unref (handle_map);
        break;
    default:
        g_debug ("handle isn't transient, not virtualizing");
        break;
    }

    return entry;
}
static void
dump_command (Tpm2Command *command)
{
    g_assert (command != NULL);
    g_debug ("Tpm2Command: 0x%" PRIxPTR, (uintptr_t)command);
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
    g_debug ("Tpm2Response: 0x%" PRIxPTR, (uintptr_t)response);
    g_debug_bytes (tpm2_response_get_buffer (response),
                   tpm2_response_get_size (response),
                   16,
                   4);
    g_debug_tpma_cc (tpm2_response_get_attributes (response));
}
Tpm2Response*
resource_manager_flush_context (ResourceManager *resmgr,
                                Tpm2Command     *command)
{
    Connection     *connection;
    HandleMap      *map;
    HandleMapEntry *entry;
    Tpm2Response   *response;
    TPM_HANDLE      handle;
    TPM_HT          handle_type;
    TSS2_RC         rc;

    if (tpm2_command_get_code (command) != TPM_CC_FlushContext) {
        g_warning ("resource_manager_flush_context with wrong command");
        return TSS2_RC_SUCCESS;
    }
    handle = tpm2_command_get_flush_handle (command);
    g_debug ("resource_manager_flush_context handle: 0x%" PRIx32, handle);
    handle_type = handle >> HR_SHIFT;
    switch (handle_type) {
    case TPM_HT_TRANSIENT:
        g_debug ("handle is TPM_HT_TRANSIENT, virtualizing");
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
            rc = RM_RC (TPM_RC_HANDLE + TPM_RC_P + TPM_RC_1);
        }
        g_object_unref (map);
        response = tpm2_response_new_rc (connection, rc);
        g_object_unref (connection);
        break;
    case TPM_HT_POLICY_SESSION:
        g_debug ("handle is TPM_HT_POLICY_SESSION");
        /* fallthrough */
    default:
        g_debug ("handle is for unmanaged object, sending command to TPM");
        response = access_broker_send_command (resmgr->access_broker,
                                               command,
                                               &rc);
        break;
    }

    return response;
}
/*
 * Determine whether the command may return a transient handle. If so
 * be sure we have room in the handle map for it.
 */
gboolean
resource_manager_is_over_object_quota (ResourceManager *resmgr,
                                       Tpm2Command     *command)
{
    HandleMap   *handle_map;
    Connection *connection;
    gboolean     ret = FALSE;

    switch (tpm2_command_get_code (command)) {
    /* These commands load transient objects. */
    case TPM_CC_CreatePrimary:
    case TPM_CC_Load:
    case TPM_CC_LoadExternal:
        connection = tpm2_command_get_connection (command);
        handle_map = connection_get_trans_map (connection);
        if (handle_map_is_full (handle_map)) {
            ret = TRUE;
        }
        g_object_unref (connection);
        g_object_unref (handle_map);
    }

    return ret;
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
    TPM_HANDLE       handle      = handle_map_entry_get_vhandle (entry);
    TPM_HT           handle_type = 0;

    handle_type = handle >> HR_SHIFT;
    g_debug ("remove_entry_from_handle_map");
    switch (handle_type) {
    case TPM_HT_TRANSIENT:
        g_debug ("entry 0x%" PRIxPTR " is transient, removing from map",
                 (uintptr_t)entry);
        handle_map_remove (map, handle);
        break;
    default:
        g_debug ("entry 0x%" PRIxPTR " not transient, leaving entry alone",
                 (uintptr_t)entry);
        break;
    }
}
/*
 * This function handles the required post-processing on the HandleMapEntry
 * objects in the GSList that represent objects loaded into the TPM as part of
 * executing a command.
 */
void
post_process_entry_list (ResourceManager  *resmgr,
                         GSList          **entry_slist,
                         Connection       *connection,
                         TPMA_CC           command_attrs)
{
    /* if flushed bit is clear we need to flush & save contexts */
    if (!command_attrs.flushed) {
        g_debug ("flushsave_context for %" PRIu32 " entries",
                 g_slist_length (*entry_slist));
        g_slist_foreach (*entry_slist,
                        resource_manager_flushsave_context,
                        resmgr);
    } else {
        /* if flushed bit is set the entry has been flushed, remove it */
        g_debug ("TPMA_CC flushed bit set");
        g_slist_foreach (*entry_slist,
                         remove_entry_from_handle_map,
                         connection);
    }
    g_slist_free_full (*entry_slist, g_object_unref);
}
/*
 * This structure is used to keep state while iterating over a list of
 * TPM_HANDLES.
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
    TPMI_YES_NO           more_data;
    TPM_HANDLE            start_handle;
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
    TPM_HANDLE                vhandle  = (uintptr_t)entry;
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
        state->more_data = YES;
        return;
    }
    cap_data->data.handles.handle [cap_data->data.handles.count] = vhandle;
    ++cap_data->data.handles.count;
}
/*
 * This is a GCompareFunc used to sort a list of TPM_HANDLES.
 */
int
handle_compare (gconstpointer a,
                gconstpointer b)
{
    TPM_HANDLE handle_a = (uintptr_t)a;
    TPM_HANDLE handle_b = (uintptr_t)b;

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
 *   YES when more handles are present
 *   NO when there are no more handles
 */
TPMI_YES_NO
get_cap_handles (HandleMap            *map,
                 TPM_HANDLE            prop,
                 UINT32                count,
                 TPMS_CAPABILITY_DATA *cap_data)
{
    GList *vhandle_list;
    vhandle_iterator_state_t state = {
        .cap_data     = cap_data,
        .max_count    = count,
        .more_data    = NO,
        .start_handle = prop,
    };

    cap_data->capability = TPM_CAP_HANDLES;
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
 * specifically taylored and should not be used elsewhere.
 */
#define YES_NO_OFFSET TPM_HEADER_SIZE
#define YES_NO_SET(buffer, value) \
    (*(TPMI_YES_NO*)(buffer + YES_NO_OFFSET) = value)
#define CAP_OFFSET (TPM_HEADER_SIZE + sizeof (TPMI_YES_NO))
#define CAP_SET(buffer, value) \
    (*(TPM_CAP*)(buffer + CAP_OFFSET) = htobe32 (value))
#define HANDLE_COUNT_OFFSET (CAP_OFFSET + sizeof (TPM_CAP))
#define HANDLE_COUNT_SET(buffer, value) \
    (*(UINT32*)(buffer + HANDLE_COUNT_OFFSET) = htobe32 (value))
#define HANDLE_OFFSET (HANDLE_COUNT_OFFSET + sizeof (UINT32))
#define HANDLE_INDEX(i) (sizeof (TPM_HANDLE) * i)
#define HANDLE_SET(buffer, i, value) \
    (*(TPM_HANDLE*)(buffer + HANDLE_OFFSET + HANDLE_INDEX (i)) = \
        htobe32 (value))
#define CAP_RESP_SIZE(value) \
    (TPM_HEADER_SIZE + \
     sizeof (TPMI_YES_NO) + \
     sizeof (value->capability) + \
     sizeof (value->data.handles.count) + \
     (value->data.handles.count * \
      sizeof (value->data.handles.handle [0])))
/*
 * This function is used to build a response buffer that contains the provided
 * TPMS_CAPABILITY_DATA and TPMI_YES_NO. These are the two response parameters
 * to the TPM2_GetCapability function.
 * The 'cap_data' parameter *must* have the TPMU_CAPABILITY union populated
 * with the TPM_CAP_HANDLES selector.
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
    set_response_tag (buf, TPM_ST_NO_SESSIONS);
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
 * This function takes a Tpm2Command and the associated connection object
 * as parameters. The Tpm2Command *must* be a GetCapability command. If it's
 * a GetCapability command that we "virtualize" then we'll build a
 * Tpm2Response object and return it. If not we return NULL.
 */
Tpm2Response*
get_cap_handles_response (Tpm2Command *command,
                          Connection *connection)
{
    TPM_CAP  cap         = tpm2_command_get_cap (command);
    UINT32   prop        = tpm2_command_get_prop (command);
    UINT32   prop_count  = tpm2_command_get_prop_count (command);
    TPM_HT   handle_type = prop >> HR_SHIFT;
    HandleMap *map;
    TPMS_CAPABILITY_DATA cap_data = { .capability = cap };
    TPMI_YES_NO more_data = NO;
    uint8_t *resp_buf;
    Tpm2Response *response = NULL;

    g_debug ("processing TPM_CC_GetCapability with cap: 0x%" PRIx32
             " prop: 0x%" PRIx32 " prop_count: 0x%" PRIx32,
             cap, prop, prop_count);
    if (cap == TPM_CAP_HANDLES && handle_type == TPM_HT_TRANSIENT) {
        g_debug ("TPM_CAP_HANDLES && TPM_HT_TRANSIENT");
        map = connection_get_trans_map (connection);
        more_data = get_cap_handles (map,  prop, prop_count, &cap_data);
        g_object_unref (map);
        resp_buf = build_cap_handles_response (&cap_data, more_data);
        response = tpm2_response_new (connection,
                                      resp_buf,
                                      tpm2_command_get_attributes (command));
    }

    return response;
}
/*
 * If the provided command is something that the ResourceManager "virtualizes"
 * then this function will do so and return a Tpm2Response object that will be
 * returned to the same connection. If the command isn't something that we
 * virtualize then we just return NULL.
 */
Tpm2Response*
virtualize_command (ResourceManager *resmgr,
                    Tpm2Command     *command)
{
    Connection   *connection = NULL;
    Tpm2Response *response   = NULL;

    switch (tpm2_command_get_code (command)) {
    case TPM_CC_FlushContext:
        g_debug ("processing TPM_CC_FlushContext");
        response = resource_manager_flush_context (resmgr, command);
        break;
    case TPM_CC_GetCapability:
        g_debug ("processing TPM2_CC_GetCapability");
        response = get_cap_handles_response (command, connection);
        break;
    default:
        break;
    }

    return response;
}
/**
 * This function is invoked in response to the receipt of a Tpm2Command.
 * This is the place where we send the command buffer out to the TPM
 * through the AccessBroker which will eventually get it to the TPM for us.
 * The AccessBroker will send us back a Tpm2Response that we send back to
 * the client by way of our Sink object. The flow is roughly:
 * - Receive the Tpm2Command as a parameter
 * - Load all virtualized objects required by the command.
 * - Send the Tpm2Command out through the AccessBroker.
 * - Receive the response from the AccessBroker.
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
    HandleMapEntry *entry;
    Tpm2Response   *response;
    TSS2_RC         rc = TSS2_RC_SUCCESS;
    GSList         *entry_slist = NULL;
    TPMA_CC         command_attrs = tpm2_command_get_attributes (command);

    g_debug ("resource_manager_process_tpm2_command: resmgr: 0x%" PRIxPTR
             ", cmd: 0x%" PRIxPTR, (uintptr_t)resmgr, (uintptr_t)command);
    dump_command (command);
    connection = tpm2_command_get_connection (command);
    /* If connection has reached quota limit, send error response */
    if (resource_manager_is_over_object_quota (resmgr, command)) {
        response = tpm2_response_new_rc (connection,
                                         TSS2_RESMGR_RC_OBJECT_MEMORY);
        goto send_response;
    }
    /* If command is virtualized by the ResourceManager, get the response */
    response = virtualize_command (resmgr, command);
    if (response != NULL) {
        goto send_response;
    }
    /* Load contexts and switch virtual to physical handles in command */
    if (tpm2_command_get_handle_count (command) > 0) {
        resource_manager_load_contexts (resmgr, command, &entry_slist);
    }
    response = access_broker_send_command (resmgr->access_broker,
                                           command,
                                           &rc);
    if (response == NULL) {
        g_warning ("access_broker_send_command returned error: 0x%x", rc);
        response = tpm2_response_new_rc (connection, rc);
    }
    dump_response (response);
    /* transform virtualized handles in Tpm2Response if necessary */
    if (tpm2_response_has_handle (response)) {
        entry = resource_manager_virtualize_handle (resmgr, response);
        if (entry != NULL) {
            entry_slist = g_slist_prepend (entry_slist, entry);
        }
    }
send_response:
    /* send response to next processing stage */
    sink_enqueue (resmgr->sink, G_OBJECT (response));
    g_object_unref (response);
    /* save contexts that were previously loaded by 'load_contexts */
    post_process_entry_list (resmgr, &entry_slist, connection, command_attrs);
    g_object_unref (connection);
    return;
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

    g_debug ("resource_manager_thread start");
    while (TRUE) {
        obj = message_queue_dequeue (resmgr->in_queue);
        g_debug ("resource_manager_thread: message_queue_dequeue got obj: "
                 "0x%" PRIxPTR, (uintptr_t)obj);
        if (obj == NULL) {
            g_debug ("resource_manager_thread: dequeued a null object");
            break;
        }
        if (IS_TPM2_COMMAND (obj)) {
            resource_manager_process_tpm2_command (resmgr, TPM2_COMMAND (obj));
            g_object_unref (obj);
        } else if (IS_CONTROL_MESSAGE (obj)) {
            ControlCode code =
                control_message_get_code (CONTROL_MESSAGE (obj));
            /* we must unref the message before processing the ControlCode
             * since the function may cause the thread to exit.
             */
            g_object_unref (obj);
            process_control_code (code);
        }
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
    g_debug ("resource_manager_cancel: enqueuing ControlMessage: 0x%" PRIxPTR,
             (uintptr_t)msg);
    message_queue_enqueue (resmgr->in_queue, G_OBJECT (msg));
    g_object_unref (msg);
}
/**
 * Implement the 'enqueue' function from the Sink interface. This is how
 * new messages / commands get into the AccessBroker.
 */
void
resource_manager_enqueue (Sink        *sink,
                          GObject     *obj)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (sink);

    g_debug ("resource_manager_enqueue: ResourceManager: 0x%" PRIxPTR " obj: "
             "0x%" PRIxPTR, (uintptr_t)resmgr, (uintptr_t)obj);
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

    g_debug ("resource_manager_add_sink: ResourceManager: 0x%" PRIxPTR
             ", Sink: 0x%" PRIxPTR, (uintptr_t)resmgr, (uintptr_t)sink);
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

    g_debug ("resource_manager_set_property: 0x%" PRIxPTR,
             (uintptr_t)resmgr);
    switch (property_id) {
    case PROP_QUEUE_IN:
        resmgr->in_queue = g_value_get_object (value);
        g_debug ("  in_queue: 0x%" PRIxPTR, (uintptr_t)resmgr->in_queue);
        break;
    case PROP_SINK:
        if (resmgr->sink != NULL) {
            g_warning ("  sink already set");
            break;
        }
        resmgr->sink = SINK (g_value_get_object (value));
        g_object_ref (resmgr->sink);
        g_debug ("  sink: 0x%" PRIxPTR, (uintptr_t)resmgr->sink);
        break;
    case PROP_ACCESS_BROKER:
        if (resmgr->access_broker != NULL) {
            g_warning ("  access_broker already set");
            break;
        }
        resmgr->access_broker = g_value_get_object (value);
        g_object_ref (resmgr->access_broker);
        g_debug ("  access_broker: 0x%" PRIxPTR, (uintptr_t)resmgr->access_broker);
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

    g_debug ("resource_manager_get_property: 0x%" PRIxPTR, (uintptr_t)resmgr);
    switch (property_id) {
    case PROP_QUEUE_IN:
        g_value_set_object (value, resmgr->in_queue);
        break;
    case PROP_SINK:
        g_value_set_object (value, resmgr->sink);
        break;
    case PROP_ACCESS_BROKER:
        g_value_set_object (value, resmgr->access_broker);
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
resource_manager_finalize (GObject *obj)
{
    ResourceManager *resmgr = RESOURCE_MANAGER (obj);
    Thread *thread = THREAD (obj);

    g_debug ("resource_manager_finalize: 0x%" PRIxPTR, (uintptr_t)resmgr);
    if (resmgr == NULL)
        g_error ("resource_manager_finalize passed NULL ResourceManager pointer");
    if (thread->thread_id != 0)
        g_error ("resource_manager_finalize called with thread running, "
                 "cancel thread first");
    g_object_unref (resmgr->in_queue);
    if (resmgr->sink) {
        g_object_unref (resmgr->sink);
        resmgr->sink = NULL;
    }
    if (resmgr->access_broker) {
        g_object_unref (resmgr->access_broker);
        resmgr->access_broker = NULL;
    }
    if (resource_manager_parent_class)
        G_OBJECT_CLASS (resource_manager_parent_class)->finalize (obj);
}
static void
resource_manager_init (ResourceManager *manager)
{ /* noop */ }
/**
 * GObject class initialization function. This function boils down to:
 * - Setting up the parent class.
 * - Set finalize, property get/set.
 * - Install properties.
 */
static void
resource_manager_class_init (ResourceManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ThreadClass  *thread_class = THREAD_CLASS (klass);

    if (resource_manager_parent_class == NULL)
        resource_manager_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize     = resource_manager_finalize;
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
    obj_properties [PROP_ACCESS_BROKER] =
        g_param_spec_object ("access-broker",
                             "AccessBroker object",
                             "TPM Access Broker for communication with TPM",
                             TYPE_ACCESS_BROKER,
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
/**
 * Create new ResourceManager object.
 */
ResourceManager*
resource_manager_new (AccessBroker    *broker)
{
    if (broker == NULL)
        g_error ("resource_manager_new passed NULL AccessBroker");
    MessageQueue *queue = message_queue_new ("ResourceManager input queue");
    return RESOURCE_MANAGER (g_object_new (TYPE_RESOURCE_MANAGER,
                                           "queue-in",        queue,
                                           "access-broker",   broker,
                                           NULL));
}
