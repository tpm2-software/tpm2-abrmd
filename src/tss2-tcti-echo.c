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
#include "tss2-tcti-echo.h"
#include "tss2-tcti-echo-priv.h"

TSS2_RC
tss2_tcti_echo_transmit (TSS2_TCTI_CONTEXT *tcti_context,
                         size_t             size,
                         const uint8_t      *command)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    /* the tss2_tcti_transmit macro checks for a valid context for us */
    if ((command == NULL) ||
        (size > echo_context->buf_size) ||
        (size <= 0))
    {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (echo_context->state != CAN_TRANSMIT)
        return TSS2_TCTI_RC_BAD_SEQUENCE;

    memcpy (echo_context->buf, command, size);
    echo_context->data_size = size;
    echo_context->state = CAN_RECEIVE;
    return TSS2_RC_SUCCESS;
}

TSS2_RC
tss2_tcti_echo_receive (TSS2_TCTI_CONTEXT *tcti_context,
                        size_t            *size,
                        uint8_t           *response,
                        int32_t            timeout)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (echo_context->state != CAN_RECEIVE)
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    if (size == NULL)
        return TSS2_TCTI_RC_BAD_VALUE;
    if (*size < echo_context->data_size) {
        *size = echo_context->data_size;
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    if (*size <= 0 || response == NULL)
        return TSS2_TCTI_RC_BAD_VALUE;
    if (timeout < 0 &&
        timeout != TSS2_TCTI_TIMEOUT_BLOCK &&
        timeout != TSS2_TCTI_TIMEOUT_NONE)
    {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    memcpy (response, echo_context->buf, echo_context->data_size);
    *size = echo_context->data_size;
    echo_context->state = CAN_TRANSMIT;

    return TSS2_RC_SUCCESS;
}

void
tss2_tcti_echo_finalize (TSS2_TCTI_CONTEXT *tcti_context)
{
    UNUSED_PARAM(tcti_context);
    /* noop */
}

TSS2_RC
tss2_tcti_echo_cancel (TSS2_TCTI_CONTEXT *tcti_context)
{
    UNUSED_PARAM(tcti_context);
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tss2_tcti_echo_get_poll_handles (TSS2_TCTI_CONTEXT     *tcti_context,
                                 TSS2_TCTI_POLL_HANDLE *handles,
                                 size_t                *num_handles)
{
    UNUSED_PARAM(tcti_context);
    UNUSED_PARAM(handles);
    UNUSED_PARAM(num_handles);
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tss2_tcti_echo_set_locality (TSS2_TCTI_CONTEXT *tcti_context,
                             uint8_t            locality)
{
    UNUSED_PARAM(tcti_context);
    UNUSED_PARAM(locality);
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}
/**
 * The init function serves two purposes:
 * 1) Firstly if the 'tcti_context' parameter is set to NULL then the
 *    function must return the appropriate size for the context structure.
 *    The assumption is that the caller will allocate this data for us.
 *    For this to be possible the 'ctx_size' parameter must be non NULL
 *    since we use this to return the size to the caller. Also the
 *    'buf_size' parameter must be at least TCTI_ECHO_MIN_BUF.
 * 2) Secondly if the 'tcti_context' parameter is non-NULL the it is
 *    initialized for use. This requires that the 'ctx_size' is set to
 *    the size allocated by the caller. The 'buf_size' parameter is
 *    ignored since we can calculate this size from the 'ctx_size'.
 */
TSS2_RC
tss2_tcti_echo_init (TSS2_TCTI_CONTEXT *tcti_context,
                     size_t            *ctx_size,
                     size_t             buf_size)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    /* Error case for invalid parameters */
    if ((tcti_context == NULL && ctx_size == NULL) ||
        (tcti_context == NULL && buf_size < TSS2_TCTI_ECHO_MIN_BUF) ||
        (tcti_context == NULL && buf_size > TSS2_TCTI_ECHO_MAX_BUF))
    {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    /* First case from description above */
    if (tcti_context == NULL && ctx_size != NULL) {
        *ctx_size = sizeof (TCTI_ECHO_CONTEXT) + buf_size;
        return TSS2_RC_SUCCESS;
    }
    /* the rest covers the second case: initialization */
    TSS2_TCTI_MAGIC (tcti_context)            = TCTI_ECHO_MAGIC;
    TSS2_TCTI_VERSION (tcti_context)          = 1;
    TSS2_TCTI_TRANSMIT (tcti_context)         = tss2_tcti_echo_transmit;
    TSS2_TCTI_RECEIVE (tcti_context)          = tss2_tcti_echo_receive;
    TSS2_TCTI_FINALIZE (tcti_context)         = tss2_tcti_echo_finalize;
    TSS2_TCTI_CANCEL (tcti_context)           = tss2_tcti_echo_cancel;
    TSS2_TCTI_GET_POLL_HANDLES (tcti_context) = tss2_tcti_echo_get_poll_handles;
    TSS2_TCTI_SET_LOCALITY (tcti_context)     = tss2_tcti_echo_set_locality;

    echo_context->buf_size = *ctx_size - sizeof (TCTI_ECHO_CONTEXT);
    echo_context->data_size = 0;
    echo_context->state = CAN_TRANSMIT;
    memset (echo_context->buf, 0, echo_context->buf_size);

    return TSS2_RC_SUCCESS;
}
