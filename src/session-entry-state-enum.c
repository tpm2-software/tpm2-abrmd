/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include "session-entry-state-enum.h"

char*
session_entry_state_to_str (SessionEntryStateEnum state)
{
    switch (state) {
    case SESSION_ENTRY_LOADED:
        return "loaded";
    case SESSION_ENTRY_SAVED_CLIENT:
        return "saved-client";
    case SESSION_ENTRY_SAVED_CLIENT_CLOSED:
        return "saved_client-closed";
    case SESSION_ENTRY_SAVED_RM:
        return "saved-rm";
    default:
        return NULL;
    }
}

GType
session_entry_state_enum_get_type (void)
{
    static volatile gsize g_define_type_id__volatile = 0;

    if (g_once_init_enter (&g_define_type_id__volatile)) {
        static const GEnumValue my_enum_values[] = {
            {
                SESSION_ENTRY_LOADED,
                "SessionEntry loaded in TPM",
                "Loaded",
            },
            {
                SESSION_ENTRY_SAVED_RM,
                "SessionEntry populated with latest context saved by RM",
                "SavedRM"
            },
            {
                SESSION_ENTRY_SAVED_CLIENT,
                "SessionEntry for context saved by the client",
                "SavedClient"
            },
            {
                SESSION_ENTRY_SAVED_CLIENT_CLOSED,
                "SesssionEntry for context saved by client over a connection "
                "that has been closed",
                "SavedClientClosed",
            },
            { 0, NULL, NULL }
        };

        GType session_entry_state_enum_type =
            g_enum_register_static ("SessionEntryStateEnum", my_enum_values);
        g_once_init_leave (&g_define_type_id__volatile,
                           session_entry_state_enum_type);
    }

    return g_define_type_id__volatile;
}
