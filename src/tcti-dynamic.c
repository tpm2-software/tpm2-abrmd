/*
 * Copyright (c) 2018, Intel Corporation
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

#include <dlfcn.h>
#include <inttypes.h>

#include "util.h"
#include "tabrmd.h"
#include "tcti-dynamic.h"
#include "tcti-util.h"

G_DEFINE_TYPE (TctiDynamic, tcti_dynamic, TYPE_TCTI);

enum {
    PROP_0,
    PROP_FILE_NAME,
    PROP_CONF_STR,
    N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
tcti_dynamic_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    TctiDynamic *self = TCTI_DYNAMIC (object);

    switch (property_id) {
    case PROP_FILE_NAME:
        self->file_name = g_value_dup_string (value);
        g_debug ("%s: TctiDynamic 0x%" PRIxPTR " set filename: %s",
                 __func__, (uintptr_t)self, self->file_name);
        break;
    case PROP_CONF_STR:
        self->conf_str = g_value_dup_string (value);
        g_debug ("%s: TctiDynamic 0x%" PRIxPTR " PROP_CONF_STR set to: %s",
                 __func__, (uintptr_t)self, self->conf_str);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tcti_dynamic_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    TctiDynamic *self = TCTI_DYNAMIC (object);

    g_debug ("%s: TctiDynamic 0x%" PRIxPTR, __func__, (uintptr_t)self);
    switch (property_id) {
    case PROP_FILE_NAME:
        g_value_set_string (value, self->file_name);
        break;
    case PROP_CONF_STR:
        g_value_set_string (value, self->conf_str);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/*
 * Override the parent finalize method so we can free the data associated with
 * the TctiDynamic instance.
 */
static void
tcti_dynamic_finalize (GObject *obj)
{
    Tcti          *tcti        = TCTI (obj);
    TctiDynamic    *tcti_dynamic = TCTI_DYNAMIC (obj);

    if (tcti->tcti_context) {
        Tss2_Tcti_Finalize (tcti->tcti_context);
    }
    g_clear_pointer (&tcti->tcti_context, g_free);
#if !defined (DISABLE_DLCLOSE)
    g_clear_pointer (&tcti_dynamic->tcti_dl_handle, dlclose);
#endif
    g_clear_pointer (&tcti_dynamic->file_name, g_free);
    g_clear_pointer (&tcti_dynamic->conf_str, g_free);
    G_OBJECT_CLASS (tcti_dynamic_parent_class)->finalize (obj);
}
static void
tcti_dynamic_init (TctiDynamic *tcti)
{
    UNUSED_PARAM(tcti);
    /* noop */
}
/*
 * When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_dynamic_class_init (TctiDynamicClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    TctiClass    *tcti_class   = TCTI_CLASS (klass);

    if (tcti_dynamic_parent_class == NULL)
        tcti_dynamic_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_dynamic_finalize;
    object_class->get_property = tcti_dynamic_get_property;
    object_class->set_property = tcti_dynamic_set_property;

    tcti_class->initialize = (TctiInitFunc)tcti_dynamic_initialize;

    obj_properties[PROP_FILE_NAME] =
        g_param_spec_string ("file-name",
                             "TCTI library file",
                             "Library file containing TCTI implementation.",
                             "libtss2-tcti-device.so",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_CONF_STR] =
        g_param_spec_string ("conf-str",
                             "TCTI config string",
                             "TCTI initialization configuration string.",
                             "empty string",
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}
/*
 * Instantiate a new TctiDynamic object.
 */
TctiDynamic*
tcti_dynamic_new (const gchar *file_name,
                  const gchar *conf_str)
{
    return TCTI_DYNAMIC (g_object_new (TYPE_TCTI_DYNAMIC,
                                       "file-name", file_name,
                                       "conf-str", conf_str,
                                       NULL));
}

/*
 * Initialize an instance of a TSS2_TCTI_CONTEXT for the device TCTI.
 */
TSS2_RC
tcti_dynamic_initialize (TctiDynamic *self)
{
    Tcti          *tcti     = TCTI (self);
    TSS2_RC        rc       = TSS2_RC_SUCCESS;

    /*
     * This should be done through the GInitable interface.
     */
    g_debug ("%s: TctiDynamic 0x%" PRIxPTR, __func__, (uintptr_t)self);
    rc = tcti_util_discover_info (self->file_name,
                                  &self->tcti_info,
                                  &self->tcti_dl_handle);
    if (rc != TSS2_RC_SUCCESS) {
        return rc;
    }
    return tcti_util_dynamic_init (self->tcti_info,
                                   self->conf_str,
                                   &tcti->tcti_context);
}
