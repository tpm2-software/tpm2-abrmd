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
#ifndef HANDLE_MAP_ENTRY_H
#define HANDLE_MAP_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <tss2/tpm20.h>

G_BEGIN_DECLS

typedef struct _HandleMapEntryClass {
    GObjectClass      parent;
} HandleMapEntryClass;

typedef struct _HandleMapEntry {
    GObject           parent_instance;
    TPM2_HANDLE        phandle;
    TPM2_HANDLE        vhandle;
    TPMS_CONTEXT      context;
} HandleMapEntry;

#define TYPE_HANDLE_MAP_ENTRY              (handle_map_entry_get_type   ())
#define HANDLE_MAP_ENTRY(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_HANDLE_MAP_ENTRY, HandleMapEntry))
#define HANDLE_MAP_ENTRY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_HANDLE_MAP_ENTRY, HandleMapEntryClass))
#define IS_HANDLE_MAP_ENTRY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_HANDLE_MAP_ENTRY))
#define IS_HANDLE_MAP_ENTRY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_HANDLE_MAP_ENTRY))
#define HANDLE_MAP_ENTRY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_HANDLE_MAP_ENTRY, HandleMapEntryClass))

GType            handle_map_entry_get_type      (void);
HandleMapEntry*  handle_map_entry_new           (TPM2_HANDLE         phandle,
                                                 TPM2_HANDLE         vhandle);
TPM2_HANDLE       handle_map_entry_get_phandle   (HandleMapEntry    *entry);
TPM2_HANDLE       handle_map_entry_get_vhandle   (HandleMapEntry    *entry);
TPMS_CONTEXT*    handle_map_entry_get_context   (HandleMapEntry    *entry);
void             handle_map_entry_set_phandle   (HandleMapEntry    *entry,
                                                 TPM2_HANDLE         phandle);

G_END_DECLS
#endif /* HANDLE_MAP_ENTRY_H */
