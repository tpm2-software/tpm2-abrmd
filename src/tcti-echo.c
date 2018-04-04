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
#include <errno.h>
#include <string.h>

#include "util.h"
#include "tcti-echo.h"
#include "tss2-tcti-echo.h"

G_DEFINE_TYPE (TctiEcho, tcti_echo, TYPE_TCTI);

enum {
    PROP_0,
    PROP_SIZE,
    N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
tcti_echo_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    TctiEcho *self = TCTI_ECHO (object);

    g_debug ("tcti_echo_set_property");
    switch (property_id) {
    case PROP_SIZE:
        self->size = g_value_get_uint (value);
        g_debug ("  PROP_SIZE: 0x%x", self->size);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
static void
tcti_echo_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
    TctiEcho *self = TCTI_ECHO (object);

    g_debug ("tcti_echo_get_property");
    switch (property_id) {
    case PROP_SIZE:
        g_debug ("  PROP_SIZE: 0x%x", self->size);
        g_value_set_uint (value, self->size);
        break;
    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
/* override the parent finalize method so we can free the data associated with
 * the TctiEcho instance.
 * NOTE: breaking naming convention due to conflict with TCTI finalize function
 */
static void
tcti_echo_finalize (GObject *obj)
{
    Tcti *tcti = TCTI (obj);

    if (tcti->tcti_context != NULL) {
        Tss2_Tcti_Finalize (tcti->tcti_context);
    }
    g_clear_pointer (&tcti->tcti_context, g_free);
    G_OBJECT_CLASS (tcti_echo_parent_class)->finalize (obj);
}
static void
tcti_echo_init (TctiEcho *tcti)
{
    UNUSED_PARAM(tcti);
    /* noop */
}
/* When the class is initialized we set the pointer to our finalize function.
 */
static void
tcti_echo_class_init (TctiEchoClass *klass)
{
    GObjectClass  *object_class = G_OBJECT_CLASS  (klass);
    TctiClass *base_class   = TCTI_CLASS (klass);

    if (tcti_echo_parent_class == NULL)
        tcti_echo_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize     = tcti_echo_finalize;
    object_class->get_property = tcti_echo_get_property;
    object_class->set_property = tcti_echo_set_property;

    base_class->initialize     = (TctiInitFunc)tcti_echo_initialize;

    obj_properties[PROP_SIZE] =
        g_param_spec_uint ("size",
                           "buffer size",
                           "Size for allocated internal buffer",
                           TSS2_TCTI_ECHO_MIN_BUF,
                           TSS2_TCTI_ECHO_MAX_BUF,
                           TSS2_TCTI_ECHO_MAX_BUF,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

TctiEcho*
tcti_echo_new (guint size)
{
    g_debug ("tcti_echo_new with parameter size: 0x%x", size);
    return TCTI_ECHO (g_object_new (TYPE_TCTI_ECHO,
                                    "size", size,
                                    NULL));
}
TSS2_RC
tcti_echo_initialize (TctiEcho *tcti_echo)
{
    Tcti          *tcti  = TCTI (tcti_echo);
    TSS2_RC        rc    = TSS2_RC_SUCCESS;
    size_t         ctx_size;

    if (tcti->tcti_context)
        goto out;
    rc = tss2_tcti_echo_init (NULL, &ctx_size, tcti_echo->size);
    if (rc != TSS2_RC_SUCCESS)
        goto out;
    g_debug ("allocating tcti_context: 0x%zu", ctx_size);
    tcti->tcti_context = g_malloc0 (ctx_size);
    rc = tss2_tcti_echo_init (tcti->tcti_context, &ctx_size, tcti_echo->size);
    if (rc != TSS2_RC_SUCCESS) {
        g_warning ("failed to initialize echo TCTI context: 0x%x", rc);
        g_free (tcti->tcti_context);
        tcti->tcti_context = NULL;
    }
out:
    return rc;
}
