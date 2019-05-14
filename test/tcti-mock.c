/* SPDX-License-Identifier: BSD-2-Clause */
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include <tss2/tss2_tpm2_types.h>
#include <tss2/tss2_tcti.h>

#include "tcti-mock.h"
#include "util.h"

/*
 * This function assumes that the unit test invoking this function (or
 * causing it to be invoked) has added the following types in the
 * specified order:
 * - TSS2_RC - response code
 */
TSS2_RC
tcti_mock_transmit (TSS2_TCTI_CONTEXT *context,
                    size_t size,
                    uint8_t const *command)
{
    TCTI_MOCK_CONTEXT *tcti_mock = (TCTI_MOCK_CONTEXT*)context;
    TSS2_RC rc = mock_type (TSS2_RC);

    UNUSED_PARAM (size);
    UNUSED_PARAM (command);

    if (rc != TSS2_RC_SUCCESS) {
        return rc;
    }
    if (context == NULL) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    if (TSS2_TCTI_VERSION (context) < 1) {
        return TSS2_TCTI_RC_ABI_MISMATCH;
    }
    if (tcti_mock->state != SEND) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }
    tcti_mock->state = RECEIVE;

    return rc;
}
/*
 * This function assumes that the unit test invoking this function (or
 * causing it to be invoked) has added the following types in the
 * specified order:
 * - uint8_t* - pointer to response buffer
 * - size_t - size of buffer
 * - TSS2_RC - response code
 */
TSS2_RC
tcti_mock_receive (TSS2_TCTI_CONTEXT *context,
                   size_t *size,
                   uint8_t *response,
                   int32_t timeout)
{
    TCTI_MOCK_CONTEXT *tcti_mock = (TCTI_MOCK_CONTEXT*)context;
    uint8_t *buf;
    size_t buf_size;
    TSS2_RC rc;

    UNUSED_PARAM (timeout);

    buf = mock_type (uint8_t*);
    buf_size = mock_type (size_t);
    rc = mock_type (TSS2_RC);

    if (rc != TSS2_RC_SUCCESS) {
        return rc;
    }
    if (context == NULL) {
        return TSS2_TCTI_RC_BAD_CONTEXT;
    }
    if (TSS2_TCTI_VERSION (context) < 1) {
        return TSS2_TCTI_RC_ABI_MISMATCH;
    }
    if (tcti_mock->state != RECEIVE) {
        return TSS2_TCTI_RC_BAD_SEQUENCE;
    }

    if (size == NULL) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }
    if (response == NULL) {
        *size = sizeof (TPM2_MAX_COMMAND_SIZE);
        return TSS2_RC_SUCCESS;
    }
    if (*size > TPM2_MAX_COMMAND_SIZE) {
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;
    }
    if (buf != NULL) {
        memcpy (response, buf, buf_size);
        *size = buf_size;
    }
    tcti_mock->state = SEND;

    return rc;
}
TSS2_RC
Tss2_Tcti_Mock_Init (TSS2_TCTI_CONTEXT *context,
                     size_t *size,
                     const char *conf)
{
    TCTI_MOCK_CONTEXT *tcti_mock = (TCTI_MOCK_CONTEXT*)context;

    UNUSED_PARAM (conf);

    if (context == NULL && size != NULL) {
        *size = sizeof (TCTI_MOCK_CONTEXT);
        return TSS2_RC_SUCCESS;
    }
    if (size == NULL) {
        return TSS2_TCTI_RC_BAD_VALUE;
    }

    memset (context, 0, sizeof (TCTI_MOCK_CONTEXT));
    TSS2_TCTI_MAGIC (tcti_mock) = 0x1;
    TSS2_TCTI_VERSION (tcti_mock) = 2;
    TSS2_TCTI_TRANSMIT (tcti_mock) = tcti_mock_transmit;
    TSS2_TCTI_RECEIVE (tcti_mock) = tcti_mock_receive;
    tcti_mock->state = SEND;

    return TSS2_RC_SUCCESS;
}
/*
 * non-standard init function to automate the TCTI init protocol.
 */
TSS2_TCTI_CONTEXT*
tcti_mock_init_full (void)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *context;
    size_t size = 0;

    rc = Tss2_Tcti_Mock_Init (NULL, &size, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("first Tss2_Tcti_Mock_Init failed with RC 0x%" PRIx32, rc);
        return NULL;
    }

    context = malloc (size);
    rc = Tss2_Tcti_Mock_Init (context, &size, NULL);
    if (rc != TSS2_RC_SUCCESS) {
        g_critical ("second Tss2_Tcti_Mock_Init failed with RC 0x%" PRIx32, rc);
        return NULL;
    }

    return context;
}
