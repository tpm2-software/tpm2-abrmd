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
#include <glib.h>
#include <unistd.h>

#include "tcti-tabrmd.h"

void
send_recv (TSS2_TCTI_CONTEXT *tcti_context)
{
    /* send / receive */
    TSS2_RC ret;
    char *xmit_str = "test";
    g_debug ("transmitting string: %s", xmit_str);
    ret = tss2_tcti_transmit (tcti_context, strlen (xmit_str), (uint8_t*)xmit_str);
    if (ret != TSS2_RC_SUCCESS)
        g_debug ("tss2_tcti_transmit failed");
    char recv_str[10] = {0};
    size_t recv_size = 10;
    ret = tss2_tcti_receive (tcti_context, &recv_size, (uint8_t*)recv_str, TSS2_TCTI_TIMEOUT_BLOCK);
    if (ret != TSS2_RC_SUCCESS)
        g_debug ("tss2_tcti_receive failed");
    g_debug ("received string: %s", recv_str);
}

void
send_recv_bytes (TSS2_TCTI_CONTEXT *tcti_context,
                 size_t             count)
{
    /* send / receive */
    TSS2_RC ret;
    uint8_t xmit_buf [1024], recv_buf [1024];
    size_t recv_size = 1024, recv_total = 0;
    int i;

    g_debug ("transmitting %zd bytes in 1024 chunks", count);
    for (i = count / 1024; i > 0; --i) {
        ret = tss2_tcti_transmit (tcti_context, 1024, xmit_buf);
        if (ret != TSS2_RC_SUCCESS)
            g_debug ("tss2_tcti_transmit failed");
    }
    if (count % 1024 > 0) {
        ret = tss2_tcti_transmit (tcti_context, count % 1024, xmit_buf);
        if (ret != TSS2_RC_SUCCESS)
            g_error ("tss2_tcti_transmit failed");
    }
    g_debug ("receiving %zd bytes in 1024 chunks", count);
    do {
        ret = tss2_tcti_receive (tcti_context, &recv_size, recv_buf, TSS2_TCTI_TIMEOUT_BLOCK);
        if (ret != TSS2_RC_SUCCESS)
            g_error ("tss2_tcti_receive failed");
        recv_total += recv_size;
    } while (recv_total < count);
    g_debug ("received %zd byte response", recv_total);
}

int
main (void)
{
    TSS2_TCTI_CONTEXT *tcti_context = NULL;
    TSS2_RC ret;
    size_t context_size;

    ret = tss2_tcti_tabrmd_init (NULL, &context_size);
    if (ret != TSS2_RC_SUCCESS)
        g_error ("failed to get size of tcti context");
    g_debug ("tcti size is: %zd", context_size);
    tcti_context = calloc (1, context_size);
    if (tcti_context == NULL)
        g_error ("failed to allocate memory for tcti context");
    g_debug ("context structure allocated successfully");
    ret = tss2_tcti_tabrmd_init (tcti_context, NULL);
    if (ret != TSS2_RC_SUCCESS)
        g_error ("failed to initialize tcti context");
    /* Wait for initialization thread in the TCTI to setup a connection to the
     * tabrmd. The right thing to do is for the TCTI to cause callers to block
     * on a mutex till the setup thread is done.
     */
    g_debug ("initialized");
    g_debug ("send_recv_bytes 1023");
    send_recv_bytes (tcti_context, 1023);
    g_debug ("send_recv_bytes 1024");
    send_recv_bytes (tcti_context, 1024);
    g_debug ("send_recv_bytes 1025");
    send_recv_bytes (tcti_context, 1025);
    g_debug ("send-recv_bytes 1026");
    send_recv_bytes (tcti_context, 1026);
    g_debug ("send-recv_bytes 2047");
    send_recv_bytes (tcti_context, 2047);
    g_debug ("send-recv_bytes 2048");
    send_recv_bytes (tcti_context, 2048);
    g_debug ("send-recv_bytes 2049");
    send_recv_bytes (tcti_context, 2049);

    send_recv (tcti_context);
    send_recv_bytes (tcti_context, 1024);
    tss2_tcti_cancel (tcti_context);
    tss2_tcti_set_locality (tcti_context, 1);
    send_recv (tcti_context);
    send_recv_bytes (tcti_context, 1024);
    tss2_tcti_cancel (tcti_context);
    tss2_tcti_set_locality (tcti_context, 1);
    tss2_tcti_finalize (tcti_context);
}
