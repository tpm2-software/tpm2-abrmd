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
#include "session-entry-state-enum.h"

char*
session_entry_state_to_str (SessionEntryStateEnum state)
{
    switch (state) {
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
