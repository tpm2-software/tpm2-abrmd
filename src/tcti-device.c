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
#include "tcti-device.h"

G_DEFINE_TYPE (TctiDevice, tcti_device, TYPE_TCTI);

enum {
    PROP_0,
    PROP_FILENAME,
    N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
tcti_device_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    TctiDevice *self = TCTI_DEVICE (object);

    switch (property_id) {
    case PROP_FILENAME:
        g_free (self->filename);
        self->filename = g_value_dup_string (value);
        g_debug ("TctiDevice set filename: %s", self->filename);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tcti_device_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    TctiDevice *self = TCTI_DEVICE (object);

    switch (property_id) {
    case PROP_FILENAME:
        g_value_set_string (value, self->filename);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/* override the parent finalize method so we can free the data associated with
 * the TctiDevice instance.
 */
static void
tcti_device_finalize (GObject *obj)
{
    Tcti          *tcti        = TCTI (obj);
    TctiDevice    *tcti_device = TCTI_DEVICE (obj);

    if (tcti->tcti_context) {
        tss2_tcti_finalize (tcti->tcti_context);
    }
    g_clear_pointer (&tcti->tcti_context, g_free);
    g_clear_pointer (&tcti_device->filename, g_free);
    G_OBJECT_CLASS (tcti_device_parent_class)->finalize (obj);
}
static void
tcti_device_init (TctiDevice *tcti)
{ /* noop */ }
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_device_class_init (TctiDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    TctiClass    *tcti_class   = TCTI_CLASS (klass);

    if (tcti_device_parent_class == NULL)
        tcti_device_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_device_finalize;
    object_class->get_property = tcti_device_get_property;
    object_class->set_property = tcti_device_set_property;

    tcti_class->initialize = (TctiInitFunc)tcti_device_initialize;

    obj_properties[PROP_FILENAME] =
        g_param_spec_string ("filename",
                             "Device file",
                             "TPM device file",
                             "/dev/tpm0",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/**
 * Allocate a new TctiDevice object and initialize the 'tcti_context'
 * variable to NULL.
 */
TctiDevice*
tcti_device_new (const gchar *filename)
{
    return TCTI_DEVICE (g_object_new (TYPE_TCTI_DEVICE,
                                      "filename", filename,
                                      NULL));
}
/**
 * Initialize an instance of a TSS2_TCTI_CONTEXT for the device TCTI.
 */
TSS2_RC
tcti_device_initialize (TctiDevice *self)
{
    Tcti          *tcti     = TCTI (self);
    TSS2_RC        rc       = TSS2_RC_SUCCESS;
    size_t         ctx_size;

    TCTI_DEVICE_CONF config = {
        .device_path = self->filename,
        .logCallback = NULL,
        .logData     = NULL,
    };

    if (tcti->tcti_context != NULL)
        goto out;
    rc = InitDeviceTcti (NULL, &ctx_size, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to get size for device TCTI contexxt structure: "
                   "0x%x", rc);
        goto out;
    }
    tcti->tcti_context = g_malloc0 (ctx_size);
    if (tcti->tcti_context == NULL) {
        g_warning ("failed to allocate memory");
        goto out;
    }
    rc = InitDeviceTcti (tcti->tcti_context, &ctx_size, &config);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to initialize device TCTI context: 0x%x", rc);
        g_free (tcti->tcti_context);
        tcti->tcti_context = NULL;
    }
out:
    return rc;
}
