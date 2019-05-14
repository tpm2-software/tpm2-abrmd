/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef RESOURCE_MANAGER_SESSION_H
#define RESOURCE_MANAGER_SESSION_H

#include <glib.h>
#include <inttypes.h>

#include <tss2/tss2_tpm2_types.h>

#include "resource-manager.h"
#include "session-entry.h"

Tpm2Response*
load_session (ResourceManager *resmgr,
              SessionEntry *entry);
gboolean
flush_session (ResourceManager *resmgr,
               SessionEntry *entry);
Tpm2Response*
save_session (ResourceManager *resmgr,
              SessionEntry *entry);
gboolean
regap_session (ResourceManager *resmgr,
               SessionEntry *entry);
#endif
