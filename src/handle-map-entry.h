#ifndef HANDLE_MAP_ENTRY_H
#define HANDLE_MAP_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <sapi/tpm20.h>

G_BEGIN_DECLS

typedef struct _HandleMapEntryClass {
    GObjectClass      parent;
} HandleMapEntryClass;

typedef struct _HandleMapEntry {
    GObject           parent_instance;
    TPM_HANDLE        phandle;
    TPM_HANDLE        vhandle;
    TPMS_CONTEXT      context;
} HandleMapEntry;

#define TYPE_HANDLE_MAP_ENTRY              (handle_map_entry_get_type   ())
#define HANDLE_MAP_ENTRY(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_HANDLE_MAP_ENTRY, HandleMapEntry))
#define HANDLE_MAP_ENTRY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_HANDLE_MAP_ENTRY, HandleMapEntryClass))
#define IS_HANDLE_MAP_ENTRY(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_HANDLE_MAP_ENTRY))
#define IS_HANDLE_MAP_ENTRY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_HANDLE_MAP_ENTRY))
#define HANDLE_MAP_ENTRY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_HANDLE_MAP_ENTRY, HandleMapEntryClass))

GType            handle_map_entry_get_type      (void);
HandleMapEntry*  handle_map_entry_new           (TPM_HANDLE         phandle,
                                                 TPM_HANDLE         vhandle);
TPM_HANDLE       handle_map_entry_get_phandle   (HandleMapEntry    *entry);
TPM_HANDLE       handle_map_entry_get_vhandle   (HandleMapEntry    *entry);
TPMS_CONTEXT*    handle_map_entry_get_context   (HandleMapEntry    *entry);

G_END_DECLS
#endif /* HANDLE_MAP_ENTRY_H */
