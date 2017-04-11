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
#ifndef TABD_TCTI_DEVICE_H
#define TABD_TCTI_DEVICE_H

#include <sapi/tpm20.h>
#include <tcti/tcti_device.h>
#include <glib-object.h>

#include "tcti.h"

G_BEGIN_DECLS

#define TCTI_DEVICE_DEFAULT_FILE "/dev/tpm0"

typedef struct _TctiDeviceClass {
   TctiClass           parent;
} TctiDeviceClass;

typedef struct _TctiDevice
{
    Tcti               parent_instance;
    gchar             *filename;
} TctiDevice;

#define TYPE_TCTI_DEVICE             (tcti_device_get_type       ())
#define TCTI_DEVICE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TCTI_DEVICE, TctiDevice))
#define TCTI_DEVICE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST    ((klass), TYPE_TCTI_DEVICE, TctiDeviceClass))
#define IS_TCTI_DEVICE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TCTI_DEVICE))
#define IS_TCTI_DEVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE    ((klass), TYPE_TCTI_DEVICE))
#define TCTI_DEVICE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS  ((obj),   TYPE_TCTI_DEVICE, TctiDeviceClass))

GType                tcti_device_get_type       (void);
TctiDevice*          tcti_device_new            (gchar const      *filename);
TSS2_RC              tcti_device_initialize     (TctiDevice       *tcti);

G_END_DECLS
#endif /* TABD_TCTI_DEVICE_H */
