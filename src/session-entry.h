/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
#ifndef SESSION_ENTRY_H
#define SESSION_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <tss2/tss2_tpm2_types.h>

#include "connection.h"
#include "session-entry-state-enum.h"

G_BEGIN_DECLS

#define SIZE_BUF_MAX sizeof (TPMS_CONTEXT)

typedef struct size_buf {
    size_t size;
    uint8_t buf [SIZE_BUF_MAX];
} size_buf_t;

typedef struct _SessionEntryClass {
    GObjectClass      parent;
} SessionEntryClass;

typedef struct _SessionEntry {
    GObject                parent_instance;
    Connection            *connection;
    SessionEntryStateEnum  state;
    TPM2_HANDLE            handle;
    size_buf_t             context;
    size_buf_t             context_client;
} SessionEntry;

#define TYPE_SESSION_ENTRY              (session_entry_get_type   ())
#define SESSION_ENTRY(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_SESSION_ENTRY, SessionEntry))
#define SESSION_ENTRY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_SESSION_ENTRY, SessionEntryClass))
#define IS_SESSION_ENTRY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_SESSION_ENTRY))
#define IS_SESSION_ENTRY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_SESSION_ENTRY))
#define SESSION_ENTRY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_SESSION_ENTRY, SessionEntryClass))

GType            session_entry_get_type        (void);
SessionEntry*    session_entry_new             (Connection        *connection,
                                                TPM2_HANDLE         handle);
size_buf_t*      session_entry_get_context_client (SessionEntry *entry);
Connection*      session_entry_get_connection  (SessionEntry      *entry);
TPM2_HANDLE       session_entry_get_handle      (SessionEntry      *entry);
size_buf_t*      session_entry_get_context     (SessionEntry      *entry);
void             session_entry_set_context     (SessionEntry      *entry,
                                                uint8_t           *buf,
                                                size_t             size);
SessionEntryStateEnum session_entry_get_state  (SessionEntry      *entry);
void             session_entry_set_connection  (SessionEntry      *entry,
                                                Connection        *connection);
void session_entry_clear_connection (SessionEntry *entry);
void             session_entry_set_state       (SessionEntry      *entry,
                                                SessionEntryStateEnum state);
void             session_entry_prettyprint     (SessionEntry      *entry);
gint session_entry_compare (gconstpointer a,
                            gconstpointer b);
gint session_entry_compare_on_connection (gconstpointer a,
                                          gconstpointer b);
gint session_entry_compare_on_handle (gconstpointer a,
                                      gconstpointer b);
gint session_entry_compare_on_context_client (SessionEntry *entry,
                                              uint8_t *buf,
                                              size_t size);
void session_entry_abandon (SessionEntry *entry);

G_END_DECLS
#endif /* SESSION_ENTRY_H */
