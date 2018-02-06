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
#ifndef TABD_TCTI_DYNAMIC_H
#define TABD_TCTI_DYNAMIC_H

#include <sapi/tpm20.h>
#include <glib-object.h>

#include "tcti.h"

G_BEGIN_DECLS

#define TCTI_DYNAMIC_DEFAULT_FILE_NAME "tcti-device.so"
#define TCTI_DYNAMIC_DEFAULT_CONF_STR  "/dev/tpm0"

typedef struct _TctiDynamicClass {
   TctiClass           parent;
} TctiDynamicClass;

typedef struct _TctiDynamic
{
    Tcti               parent_instance;
    gchar             *file_name;
    gchar             *conf_str;
    const TSS2_TCTI_INFO *tcti_info;
} TctiDynamic;

#define TYPE_TCTI_DYNAMIC             (tcti_dynamic_get_type       ())
#define TCTI_DYNAMIC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_DYNAMIC, TctiDynamic))
#define TCTI_DYNAMIC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_DYNAMIC, TctiDynamicClass))
#define IS_TCTI_DYNAMIC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_DYNAMIC))
#define IS_TCTI_DYNAMIC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_DYNAMIC))
#define TCTI_DYNAMIC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_DYNAMIC, TctiDynamicClass))

GType                tcti_dynamic_get_type       (void);
TctiDynamic*         tcti_dynamic_new            (gchar const      *file_name,
                                                  gchar const      *conf_str);
TSS2_RC              tcti_dynamic_discover_info  (TctiDynamic      *tcti);
TSS2_RC              tcti_dynamic_initialize     (TctiDynamic      *tcti);

G_END_DECLS
#endif /* TABD_TCTI_DYNAMIC_H */
