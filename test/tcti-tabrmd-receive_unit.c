/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Intel Corporation
 */
#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#if defined(__FreeBSD__)
#include <sys/poll.h>
#else
#include <poll.h>
#endif

#include <setjmp.h>
#include <cmocka.h>

#include <tss2/tss2_tpm2_types.h>

#include "tcti-tabrmd-priv.h"
#include "mock-funcs.h"

/*
 * This tests the tcti_tabrmd_poll function, ensuring that it returns the
 * expected response code for the POLIN event.
 */
static void
tcti_tabrmd_poll_fd_ready_pollin (void **state)
{
    UNUSED_PARAM (state);
    int ret;

    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);

    ret = tcti_tabrmd_poll (TEST_FD, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (ret, 0);
}
/*
 * This tests the tcti_tabrmd_poll function, ensuring that it returns the
 * expected response code for the POLLPRI event.
 */
static void
tcti_tabrmd_poll_fd_ready_pollpri (void **state)
{
    UNUSED_PARAM (state);
    int ret;

    will_return (__wrap_poll, POLLPRI);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);

    ret = tcti_tabrmd_poll (TEST_FD, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (ret, 0);
}
/*
 * This tests the tcti_tabrmd_poll function, ensuring that it returns the
 * expected response code for the POLLRDHUP event.
 */

#if defined(__FreeBSD__)
#ifndef POLLRDHUP
#define POLLRDHUP 0x0
#endif
#endif
static void
tcti_tabrmd_poll_fd_ready_pollrdhup (void **state)
{
    UNUSED_PARAM (state);
    int ret;

    will_return (__wrap_poll, POLLRDHUP);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);

    ret = tcti_tabrmd_poll (TEST_FD, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (ret, 0);
}
/*
 * This tests the tcti_tabrmd_poll function, ensuring that it returns the
 * expected response code when a timeout occurs.
 */
static void
tcti_tabrmd_poll_timeout (void **state)
{
    UNUSED_PARAM (state);
    int ret;

    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 0);

    ret = tcti_tabrmd_poll (TEST_FD, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (ret, -1);
}
/*
 * This tests the tcti_tabrmd_poll function, ensuring that it returns the
 * expected response when an error occurs.
 */
static void
tcti_tabrmd_poll_error (void **state)
{
    UNUSED_PARAM (state);
    int ret;

    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, EINVAL);
    will_return (__wrap_poll, -1);

    ret = tcti_tabrmd_poll (TEST_FD, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (ret, EINVAL);
}
/*
 * This is the setup function used to create a skeletal / minimal context
 * for testing purposes.
 */
static int
tcti_tabrmd_setup (void **state)
{
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx;
    TSS2_TCTI_CONTEXT_COMMON_V1 *common_ctx;

    tcti_ctx = calloc (1, sizeof (TSS2_TCTI_TABRMD_CONTEXT));
    tcti_ctx->state = TABRMD_STATE_RECEIVE;
    tcti_ctx->sock_connect = TEST_CONNECTION;
    tcti_ctx->index = 0;
    tcti_ctx->state = TABRMD_STATE_TRANSMIT;
    common_ctx = (TSS2_TCTI_CONTEXT_COMMON_V1*)tcti_ctx;
    common_ctx->magic = TSS2_TCTI_TABRMD_MAGIC;
    common_ctx->version = TSS2_TCTI_TABRMD_VERSION;

    *state = tcti_ctx;
    return 0;
}
/*
 * This is the setup function used to create a skeletal / minimal context
 * for testing the 'receive' function from the TCTI.
 */
static int
tcti_tabrmd_receive_setup (void **state)
{
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx;

    tcti_tabrmd_setup (state);
    tcti_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)*state;
    tcti_ctx->state = TABRMD_STATE_RECEIVE;

    *state = tcti_ctx;
    return 0;
}
/*
 * This is a teardown function to deallocate / cleanup all resources
 * associated with these tests.
 */
static int
tcti_tabrmd_teardown (void **state)
{
    free (*state);
    return 0;
}
/*
 * This test ensures that a call to tcti_tabrmd_read that causes poll to
 * timeout will return the appropriate RC.
 */
static void
tcti_tabrmd_read_poll_timeout (void **state)
{
    TSS2_RC rc;
    uint8_t resp [TPM2_MAX_RESPONSE_SIZE] = { 0, };
    size_t resp_size = sizeof (resp);
    uint32_t timeout = TSS2_TCTI_TIMEOUT_BLOCK;
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)*state;

    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);

    /* prime mock stack for poll, will return 0 indicating timeout */
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 0);

    rc = tcti_tabrmd_read (tcti_ctx, resp, resp_size, timeout);
    assert_int_equal (rc, TSS2_TCTI_RC_TRY_AGAIN);
}
/*
 * This test ensures that a call to tcti_tabrmd_read that causes poll to
 * fail / return an error that it will return the appropriate RC.
 */
static void
tcti_tabrmd_read_poll_fail (void **state)
{
    TSS2_RC rc;
    uint8_t resp [TPM2_MAX_RESPONSE_SIZE] = { 0, };
    size_t resp_size = sizeof (resp);
    uint32_t timeout = TSS2_TCTI_TIMEOUT_BLOCK;
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)*state;

    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);

    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, EINVAL);
    will_return (__wrap_poll, -1);

    rc = tcti_tabrmd_read (tcti_ctx, resp, resp_size, timeout);
    assert_int_equal (rc, TSS2_TCTI_RC_GENERAL_FAILURE);
}
/*
 * This test ensures that a call to tcti_tabrmd_read that causes
 * g_input_stream_read to return EOF will return the appropriate RC.
 */
static void
tcti_tabrmd_read_eof (void **state)
{
    int ret;
    uint8_t resp [TPM2_MAX_RESPONSE_SIZE] = { 0, };
    size_t resp_size = sizeof (resp);
    uint32_t timeout = TSS2_TCTI_TIMEOUT_BLOCK;
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)*state;

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);

    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);

    /* mock stack required to extract GIStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    /* cause g_input_stream_read to return 0 indicating EOF */
    will_return (__wrap_g_input_stream_read, 0);

    ret = tcti_tabrmd_read (tcti_ctx, resp, resp_size, timeout);
    assert_int_equal (ret, TSS2_TCTI_RC_NO_CONNECTION);
}
/*
 * This test ensures that a call to tcti_tabrmd_read that causes
 * g_input_stream_read to indicate that it would block, returns the
 * appropriate RC.
 */
static void
tcti_tabrmd_read_block_error (void **state)
{
    int ret;
    uint8_t resp [TPM2_MAX_RESPONSE_SIZE] = { 0, };
    size_t resp_size = sizeof (resp);
    uint32_t timeout = TSS2_TCTI_TIMEOUT_BLOCK;
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)*state;
    GError* error;

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);

    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);

    /* mock stack required to extract GIStream & read data */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    error = g_error_new (1, G_IO_ERROR_WOULD_BLOCK, __func__);
    will_return (__wrap_g_input_stream_read, -1);
    will_return (__wrap_g_input_stream_read, error);

    ret = tcti_tabrmd_read (tcti_ctx, resp, resp_size, timeout);
    assert_int_equal (ret, TSS2_TCTI_RC_TRY_AGAIN);
}
/*
 * This test forces the call to 'g_input_stream_read' to read fewer bytes
 * than requested by the caller (the 'tcti_tabrmd_read' in this case). This
 * is a "short read" and should return an RC telling the caller to retry.
 */
static void
tcti_tabrmd_read_short (void **state)
{
    int ret;
    uint8_t resp [TPM2_MAX_RESPONSE_SIZE] = { 0, };
    size_t resp_size = sizeof (resp), read_size = resp_size / 2;
    uint32_t timeout = TSS2_TCTI_TIMEOUT_BLOCK;
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)*state;
    uint8_t buf [sizeof (resp)] = { 0, };

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);

    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);

    /* mock stack required to extract GIStream & read data */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, read_size);
    will_return (__wrap_g_input_stream_read, buf);

    ret = tcti_tabrmd_read (tcti_ctx, resp, resp_size, timeout);
    assert_int_equal (ret, TSS2_TCTI_RC_TRY_AGAIN);
}
static void
tcti_tabrmd_read_success (void **state)
{
    int ret;
    uint8_t resp [TPM2_MAX_RESPONSE_SIZE] = { 0, };
    uint8_t buf [TPM2_MAX_RESPONSE_SIZE] = { 0, };
    size_t resp_size = sizeof (resp);
    uint32_t timeout = TSS2_TCTI_TIMEOUT_BLOCK;
    TSS2_TCTI_TABRMD_CONTEXT *tcti_ctx = (TSS2_TCTI_TABRMD_CONTEXT*)*state;

    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);

    /* mock stack required to extract GIStream & read data */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, resp_size);
    will_return (__wrap_g_input_stream_read, buf);

    ret = tcti_tabrmd_read (tcti_ctx, resp, resp_size, timeout);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
}
static void
tcti_tabrmd_receive_null_context (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);

    rc = tss2_tcti_tabrmd_receive (NULL, NULL, NULL, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_REFERENCE);
}
/*
 * This test ensures that when passed a non-zero value in the 'size' param
 * and a NULL response buffer the receive function will return the
 * appropriate RC (this seems overly restrictive).
 */
static void
tcti_tabrmd_receive_null_response (void **state)
{
    TSS2_RC rc;
    UNUSED_PARAM(state);
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)1;
    size_t size = 1;

    rc = tss2_tcti_tabrmd_receive (ctx, &size, NULL, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/*
 * Test that required checks are done on the MAGIC value in the context
 * structure and that the appropriate RC is returned.
 */
static void
tcti_tabrmd_receive_bad_magic (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    TSS2_TCTI_CONTEXT_COMMON_V1 *common_ctx = (TSS2_TCTI_CONTEXT_COMMON_V1*)*state;
    uint8_t buf[1] = { 0, };
    size_t size = sizeof (buf);

    common_ctx->magic = 0;
    rc = tss2_tcti_tabrmd_receive (ctx, &size, buf, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
}
/* Same for the version field. */
static void
tcti_tabrmd_receive_bad_version (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    TSS2_TCTI_CONTEXT_COMMON_V1 *common_ctx = (TSS2_TCTI_CONTEXT_COMMON_V1*)*state;
    uint8_t buf[1] = { 0, };
    size_t size = sizeof (buf);

    common_ctx->version = 0;
    rc = tss2_tcti_tabrmd_receive (ctx, &size, buf, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_CONTEXT);
}
/*
 * Test to be sure the 'receive' function is returning the right RC when
 * the context is in the wrong state.
 */
static void
tcti_tabrmd_receive_bad_state (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t buf[1] = { 0, };
    size_t size = sizeof (buf);

    rc = tss2_tcti_tabrmd_receive (ctx, &size, buf, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_SEQUENCE);
}
/*
 * Test that the 'receive' function returns the right RC when passed a
 * invalid timeout value.
 */
static void
tcti_tabrmd_receive_bad_timeout (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t buf[1] = { 0, };
    size_t size = sizeof (buf);

    rc = tss2_tcti_tabrmd_receive (ctx, &size, buf, -2);
    assert_int_equal (rc, TSS2_TCTI_RC_BAD_VALUE);
}
/*
 * Test that the 'receive' funciton returns the rght RC when passed a
 * buffer that is too small.
 */
static void
tcti_tabrmd_receive_size_lt_header (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t buf[1] = { 0, };
    size_t size = sizeof (buf);

    rc = tss2_tcti_tabrmd_receive (ctx, &size, buf, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_INSUFFICIENT_BUFFER);
}
/*
 * Test that the 'receive' function handles errors returned by 'poll'
 * correctly.
 */
static void
tcti_tabrmd_receive_header_fail (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    size_t size = 0;

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);

    /* prime mock stack for poll, will return 0 indicating timeout */
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, EINVAL);
    will_return (__wrap_poll, -1);

    rc = tss2_tcti_tabrmd_receive (ctx, &size, NULL, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_GENERAL_FAILURE);
}
/*
 * This test causes the header to be read successfully, but the size field
 * indicates a size < TPM_HEADER_SIZE which makes it impossible for us to
 * get the rest of the buffer.
 */
static void
tcti_tabrmd_receive_header_lt_expected (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t buf [TPM_HEADER_SIZE] = {
        0x80, 0x02,
        0x00, 0x00, 0x00, 0x01, /* invalid size < TPM_HEADER_SIZE */
        0x00, 0x00, 0x00, 0x00,
    };
    size_t size = 0;

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);
    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to get 10 bytes back from the GInputStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, TPM_HEADER_SIZE);
    will_return (__wrap_g_input_stream_read, buf);

    rc = tss2_tcti_tabrmd_receive (ctx, &size, NULL, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_MALFORMED_RESPONSE);
}
/*
 * This test covers a use case where the caller provides a NULL buffer to
 * hold the response and a zero'd size parameter. This signals to the
 * receive function that the caller wants to know how much space the
 * response will require.
 */
static void
tcti_tabrmd_receive_get_size (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t buf [TPM_HEADER_SIZE] = {
        0x80, 0x02,
        0x00, 0x00, 0x00, 0x0a, /* TPM_HEADER_SIZE -> resp is just header */
        0x00, 0x00, 0x00, 0x00,
    };
    size_t size = 0;

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);
    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to get 10 bytes back from the GInputStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, TPM_HEADER_SIZE);
    will_return (__wrap_g_input_stream_read, buf);

    rc = tss2_tcti_tabrmd_receive (ctx, &size, NULL, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (size, TPM_HEADER_SIZE);
}
/*
 * This test ensures that a response from the TPM that is exactly 10
 * bytes can be 'receive'd in a single call.
 */
static void
tcti_tabrmd_receive_header_only (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t resp [TPM_HEADER_SIZE] = { 0, };
    size_t resp_size = sizeof (resp);
    uint8_t buf [TPM_HEADER_SIZE] = {
        0x80, 0x02,
        0x00, 0x00, 0x00, 0x0a, /* TPM_HEADER_SIZE -> resp is just header */
        0x00, 0x00, 0x00, 0x00,
    };

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);
    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to get 10 bytes back from the GInputStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, TPM_HEADER_SIZE);
    will_return (__wrap_g_input_stream_read, buf);

    rc = tss2_tcti_tabrmd_receive (ctx, &resp_size, resp, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_memory_equal (buf, resp, resp_size);
}
/*
 * Receive response that is only a header in two reads. The first read will
 * be a "short" read and we should get a response indicating that we need
 * to try again.
 */
#define FIRST_READ_SIZE 3
#define SECOND_READ_SIZE 7
static void
tcti_tabrmd_receive_header_only_retry (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t resp [TPM_HEADER_SIZE] = { 0, };
    size_t resp_size = sizeof (resp);
    uint8_t buf [TPM_HEADER_SIZE] = {
        0x80, 0x02,
        0x00, 0x00, 0x00, 0x0a, /* TPM_HEADER_SIZE -> resp is just header */
        0x00, 0x00, 0x00, 0x00,
    };

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);
    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to get first bytes back from the GInputStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, FIRST_READ_SIZE);
    will_return (__wrap_g_input_stream_read, buf);

    rc = tss2_tcti_tabrmd_receive (ctx, &resp_size, resp, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_TCTI_RC_TRY_AGAIN);

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);
    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to get first bytes back from the GInputStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, SECOND_READ_SIZE);
    will_return (__wrap_g_input_stream_read, &buf[FIRST_READ_SIZE]);

    rc = tss2_tcti_tabrmd_receive (ctx, &resp_size, resp, TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_memory_equal (buf, resp, resp_size);
}
/*
 * This test implements a calling pattern referred to as "partial reads"
 * in the system API implementation from tpm2-tss. It is also prescribed
 * by the TCTI spec.
 */
static void
tcti_tabrmd_receive_partial_reads (void **state)
{
    TSS2_RC rc;
    TSS2_TCTI_CONTEXT *ctx = (TSS2_TCTI_CONTEXT*)*state;
    uint8_t buf [] = {
        0x80, 0x02,
        0x00, 0x00, 0x00, 0x0e,
        0xde, 0xad, 0xbe, 0xef,
        0xca, 0xfe, 0xd0, 0x0d,
    };
    uint8_t resp [sizeof (buf)] = { 0, };
    size_t resp_size = 0;

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);
    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to get 10 bytes back from the GInputStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, TPM_HEADER_SIZE);
    will_return (__wrap_g_input_stream_read, buf);

    rc = tss2_tcti_tabrmd_receive (ctx,
                                   &resp_size,
                                   NULL,
                                   TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_int_equal (resp_size, sizeof (buf));

    /* mock stack required to extract the fd from the GSocketConnection */
    will_return (__wrap_g_socket_connection_get_socket, TEST_SOCKET);
    will_return (__wrap_g_socket_get_fd, TEST_FD);
    /* prime mock stack for poll to indicate data is ready */
    will_return (__wrap_poll, POLLIN);
    will_return (__wrap_poll, 0);
    will_return (__wrap_poll, 1);
    /* mock stack required to get 10 bytes back from the GInputStream */
    will_return (__wrap_g_io_stream_get_input_stream, TEST_CONNECTION);
    will_return (__wrap_g_input_stream_read, 4);
    will_return (__wrap_g_input_stream_read, &buf[TPM_HEADER_SIZE]);

    rc = tss2_tcti_tabrmd_receive (ctx,
                                   &resp_size,
                                   resp,
                                   TSS2_TCTI_TIMEOUT_BLOCK);
    assert_int_equal (rc, TSS2_RC_SUCCESS);
    assert_memory_equal (buf, resp, sizeof (buf));
}

int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (tcti_tabrmd_poll_fd_ready_pollin),
        cmocka_unit_test (tcti_tabrmd_poll_fd_ready_pollpri),
        cmocka_unit_test (tcti_tabrmd_poll_fd_ready_pollrdhup),
        cmocka_unit_test (tcti_tabrmd_poll_timeout),
        cmocka_unit_test (tcti_tabrmd_poll_error),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_read_poll_timeout,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_read_poll_fail,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_read_eof,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_read_block_error,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_read_short,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_read_success,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test (tcti_tabrmd_receive_null_context),
        cmocka_unit_test (tcti_tabrmd_receive_null_response),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_bad_magic,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_bad_version,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_bad_state,
                                         tcti_tabrmd_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_bad_timeout,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_size_lt_header,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_header_fail,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_header_lt_expected,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_get_size,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_header_only,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_header_only_retry,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
        cmocka_unit_test_setup_teardown (tcti_tabrmd_receive_partial_reads,
                                         tcti_tabrmd_receive_setup,
                                         tcti_tabrmd_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
