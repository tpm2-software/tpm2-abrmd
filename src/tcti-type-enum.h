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
#ifndef TABD_TCTI_TYPE_ENUM_H
#define TABD_TCTI_TYPE_ENUM_H

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * Set the default TCTI. In order of preference: libtcti-device (if enabled),
 * otherwise fall back to libtcti-socket. If neither are available throw an
 * error. This should be a configure time error though.
 */
#ifdef HAVE_TCTI_DEVICE
#define TCTI_TYPE_DEFAULT TCTI_TYPE_DEVICE
#elif HAVE_TCTI_SOCKET
#define TCTI_TYPE_DEFAULT TCTI_TYPE_SOCKET
#else
#error at least one TCTI must be enabled
#endif

typedef enum TCTI_TYPE {
    TCTI_TYPE_NONE,
#ifdef HAVE_TCTI_DEVICE
    TCTI_TYPE_DEVICE,
#endif
#ifdef HAVE_TCTI_SOCKET
    TCTI_TYPE_SOCKET,
#endif
    TCTI_TYPE_DYNAMIC,
} TctiTypeEnum;

#define TYPE_TCTI_TYPE_ENUM      (tcti_type_enum_get_type ())
GType   tcti_type_enum_get_type  (void);

G_END_DECLS
#endif /* TABD_TCTI_TYPE_ENUM_H */
