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
#ifndef SESSION_ENTRY_STATE_ENUM_H
#define SESSION_ENTRY_STATE_ENUM_H

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * These define the possible states that a SessionEntry object may be in. The
 * states are defined by some previous action performed over the connection
 * that created the session / SessionEntry object.
 * - SESSION_ENTRY_SAVED_RM: All SessionEntry objects are in this state when
 *   they are first created. SessionEntry objects are created when the
 *   StartAuthSession command comes in over a connection. When the response
 *   comes back from the TPM the ResourceManager extracts the handle and saves
 *   the session.
 * - SESSION_ENTRY_SAVED_CLIENT: A SessionEntry transitions to this state when
 *   the client saves the session context by way of the TPM_CC_ContextSave
 *   command. Once a SessionEntry is in this state it must be explicitly
 *   loaded by a client before it can be used again.
 * - SESSION_ETNRY_SAVED_CLIENT_CLOSED: A SessionEntry transitions to this
 *   state from the SESSION_ENTRY_SAVED_CLIENT state when the connection used
 *   to create and save the session is closed. A session previously saved by
 *   the client will not be flushed by the ResourceManager which allows other
 *   connections to claim the session but only if they're in posession of the
 *   context blob.
 */
typedef enum SESSION_ENTRY_STATE {
    SESSION_ENTRY_SAVED_RM,
    SESSION_ENTRY_SAVED_CLIENT,
    SESSION_ENTRY_SAVED_CLIENT_CLOSED,
} SessionEntryStateEnum;

#define TYPE_SESSION_ENTRY_STATE_ENUM   (session_entry_state_enum_get_type ())
GType   session_entry_state_enum_get_type  (void);
char*   session_entry_state_to_str (SessionEntryStateEnum state);

G_END_DECLS
#endif /* SESSION_ENTRY_STATE_ENUM_H */
