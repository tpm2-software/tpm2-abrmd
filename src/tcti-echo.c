#include <errno.h>
#include <string.h>

#include <glib.h>

#include "tcti-echo.h"
#include "tcti-echo-priv.h"

#define TCTI_ECHO_MAX_BUF 16384

TSS2_RC
tcti_echo_transmit (TSS2_TCTI_CONTEXT *tcti_context,
                    size_t             size,
                    uint8_t           *command)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (echo_context == NULL)
        return TSS2_TCTI_RC_BAD_REFERENCE;
    if (echo_context->state != CAN_TRANSMIT)
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    if (size > echo_context->buf_size || size == 0)
        return TSS2_TCTI_RC_BAD_VALUE;

    memcpy (echo_context->buf, command, size);
    echo_context->data_size = size;
    echo_context->state = CAN_RECEIVE;
    return TSS2_RC_SUCCESS;
}

TSS2_RC
tcti_echo_receive (TSS2_TCTI_CONTEXT *tcti_context,
                   size_t            *size,
                   uint8_t           *response,
                   int32_t            timeout)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (echo_context == NULL || size == NULL)
        return TSS2_TCTI_RC_BAD_REFERENCE;
    if (echo_context->state != CAN_RECEIVE)
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    if (*size < echo_context->data_size)
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    memcpy (response, echo_context->buf, echo_context->data_size);
    *size = echo_context->data_size;
    echo_context->state = CAN_TRANSMIT;

    return TSS2_RC_SUCCESS;
}

void
tcti_echo_finalize (TSS2_TCTI_CONTEXT *tcti_context)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (tcti_context == NULL)
        g_error ("tcti_echo_finalize passed NULL TCTI context");
    if (echo_context->buf)
        free (echo_context->buf);
}

TSS2_RC
tcti_echo_cancel (TSS2_TCTI_CONTEXT *tcti_context)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tcti_echo_get_poll_handles (TSS2_TCTI_CONTEXT     *tcti_context,
                            TSS2_TCTI_POLL_HANDLE *handles,
                            size_t                *num_handles)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tcti_echo_set_locality (TSS2_TCTI_CONTEXT *tcti_context,
                        uint8_t            locality)
{
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

TSS2_RC
tcti_echo_init (TSS2_TCTI_CONTEXT *tcti_context,
                size_t            *ctx_size,
                size_t             buf_size)
{
    TCTI_ECHO_CONTEXT *echo_context = (TCTI_ECHO_CONTEXT*)tcti_context;

    if (tcti_context == NULL && ctx_size == NULL)
        return TSS2_TCTI_RC_BAD_VALUE;
    if (tcti_context == NULL && ctx_size != NULL) {
        *ctx_size = sizeof (TCTI_ECHO_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    if (buf_size > TCTI_ECHO_MAX_BUF) {
        g_warning ("tcti_echo_init cannot allocate data buffer larger than 0x%x bytes",
                   TCTI_ECHO_MAX_BUF);
        return TSS2_TCTI_RC_BAD_VALUE;
    } else if (buf_size == 0) {
        buf_size = TCTI_ECHO_MAX_BUF;
    }

    TSS2_TCTI_MAGIC (tcti_context)            = TCTI_ECHO_MAGIC;
    TSS2_TCTI_VERSION (tcti_context)          = 1;
    TSS2_TCTI_TRANSMIT (tcti_context)         = tcti_echo_transmit;
    TSS2_TCTI_RECEIVE (tcti_context)          = tcti_echo_receive;
    TSS2_TCTI_FINALIZE (tcti_context)         = tcti_echo_finalize;
    TSS2_TCTI_CANCEL (tcti_context)           = tcti_echo_cancel;
    TSS2_TCTI_GET_POLL_HANDLES (tcti_context) = tcti_echo_get_poll_handles;
    TSS2_TCTI_SET_LOCALITY (tcti_context)     = tcti_echo_set_locality;

    echo_context->buf = (uint8_t*)calloc (1, buf_size);
    if (echo_context->buf == NULL)
        g_error ("tcti_echo_init buffer allocation failed: %s", strerror (errno));
    echo_context->buf_size = buf_size;
    echo_context->data_size = 0;
    echo_context->state = CAN_TRANSMIT;

    return TSS2_RC_SUCCESS;
}
