/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef SESSION_ENTRY_STATE_ENUM_H
#define SESSION_ENTRY_STATE_ENUM_H

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * These define the possible states that a SessionEntry object may be in. The
 * states are defined by some previous action performed over the connection
 * that created the session / SessionEntry object.
 * - LOADED: The session is currently loaded in the TPM2 device. All
 *   SessionEntry objects are in this state when they are first created.
 * - SESSION_ENTRY_SAVED_RM:  A SessionEntry transitions to this state when
 *   the session is saved by the daemon.
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
    SESSION_ENTRY_LOADED,
    SESSION_ENTRY_SAVED_RM,
    SESSION_ENTRY_SAVED_CLIENT,
    SESSION_ENTRY_SAVED_CLIENT_CLOSED,
} SessionEntryStateEnum;

#define TYPE_SESSION_ENTRY_STATE_ENUM   (session_entry_state_enum_get_type ())
GType   session_entry_state_enum_get_type  (void);
char*   session_entry_state_to_str (SessionEntryStateEnum state);

G_END_DECLS
#endif /* SESSION_ENTRY_STATE_ENUM_H */
