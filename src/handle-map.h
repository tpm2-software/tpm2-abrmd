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
#ifndef HANDLE_MAP_H
#define HANDLE_MAP_H

#include <glib.h>
#include <glib-object.h>
#include <pthread.h>
#include <sapi/tpm20.h>

#include "handle-map-entry.h"

G_BEGIN_DECLS

#define MAX_ENTRIES_DEFAULT 27
#define MAX_ENTRIES_MAX     100

typedef struct _HandleMapClass {
    GObjectClass      parent;
} HandleMapClass;

typedef struct _HandleMap {
    GObject             parent_instance;
    pthread_mutex_t     mutex;
    TPM2_HT              handle_type;
    TPM2_HANDLE          handle_count;
    GHashTable         *vhandle_to_entry_table;
    guint               max_entries;
} HandleMap;

#define TYPE_HANDLE_MAP              (handle_map_get_type   ())
#define HANDLE_MAP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_HANDLE_MAP, HandleMap))
#define HANDLE_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_HANDLE_MAP, HandleMapClass))
#define IS_HANDLE_MAP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_HANDLE_MAP))
#define IS_HANDLE_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_HANDLE_MAP))
#define HANDLE_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_HANDLE_MAP, HandleMapClass))

GType            handle_map_get_type    (void);
HandleMap*       handle_map_new         (TPM2_HT          handle_type,
                                         guint           max_entries);
gboolean         handle_map_insert      (HandleMap      *map,
                                         TPM2_HANDLE      vhandle,
                                         HandleMapEntry *entry);
gint             handle_map_remove      (HandleMap     *map,
                                         TPM2_HANDLE     vremove);
HandleMapEntry*  handle_map_plookup     (HandleMap     *map,
                                         TPM2_HANDLE     phandle);
HandleMapEntry*  handle_map_vlookup     (HandleMap     *map,
                                         TPM2_HANDLE     vhandle);
guint            handle_map_size        (HandleMap     *map);
TPM2_HANDLE       handle_map_next_vhandle (HandleMap    *map);
void             handle_map_foreach      (HandleMap    *map,
                                          GHFunc        callback,
                                          gpointer      user_data);
gboolean         handle_map_is_full      (HandleMap *map);
GList*           handle_map_get_keys     (HandleMap    *map);

G_END_DECLS
#endif /* HANDLE_MAP_H */
