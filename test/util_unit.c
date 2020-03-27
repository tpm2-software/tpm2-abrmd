/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"
#include "tpm2-header.h"

#define MAX_BUF 4096
#define READ_SIZE  UTIL_BUF_SIZE
#define WRITE_SIZE 10

#define UTIL_UNIT_ERROR util_unit_error_quark ()

GQuark
util_unit_error_quark (void)
{
    return g_quark_from_static_string ("util-unit-error-quark");
}

ssize_t
__wrap_g_output_stream_write (GOutputStream *ostream,
                              const void    *buf,
                              size_t         count,
                              GCancellable  *cancellable,
                              GError       **error)
{
    GError *error_tmp = mock_type (GError*);
    UNUSED_PARAM(ostream);
    UNUSED_PARAM(buf);
    UNUSED_PARAM(count);
    UNUSED_PARAM(cancellable);

    if (error_tmp != NULL && error != NULL) {
        *error = error_tmp;
    }
    return mock_type (ssize_t);
}

void
write_in_one (void **state)
{
    ssize_t written;
    UNUSED_PARAM(state);

    will_return (__wrap_g_output_stream_write, NULL);
    will_return (__wrap_g_output_stream_write, WRITE_SIZE);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, WRITE_SIZE);
}

void
write_in_two (void **state)
{
    ssize_t written;
    UNUSED_PARAM(state);

    will_return (__wrap_g_output_stream_write, NULL);
    will_return (__wrap_g_output_stream_write, 5);
    will_return (__wrap_g_output_stream_write, NULL);
    will_return (__wrap_g_output_stream_write, 5);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, WRITE_SIZE);
}

void
write_in_three (void **state)
{
    ssize_t written;
    UNUSED_PARAM(state);

    will_return (__wrap_g_output_stream_write, NULL);
    will_return (__wrap_g_output_stream_write, 3);
    will_return (__wrap_g_output_stream_write, NULL);
    will_return (__wrap_g_output_stream_write, 3);
    will_return (__wrap_g_output_stream_write, NULL);
    will_return (__wrap_g_output_stream_write, 4);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, WRITE_SIZE);
}

void
write_error (void **state)
{
    ssize_t written;
    GError *error;
    UNUSED_PARAM(state);

    /* this is free'd by the 'write_all' function */
    error = g_error_new (UTIL_UNIT_ERROR,
                         G_IO_ERROR_WOULD_BLOCK,
                         "g-io-error-would-block");
    will_return (__wrap_g_output_stream_write, error);
    will_return (__wrap_g_output_stream_write, -1);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, -1);
}

void
write_zero (void **state)
{
    ssize_t written;
    UNUSED_PARAM(state);

    will_return (__wrap_g_output_stream_write, NULL);
    will_return (__wrap_g_output_stream_write, 0);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, 0);
}
/*
 * mock 'read' function: This function expects 3 things to be on the mock
 * queue:
 *   input buffer
 *   offset into input buffer where read starts
 *   GError
 *   return value
 */
ssize_t
__wrap_g_input_stream_read (GInputStream  *istream,
                            gint          *buf,
                            size_t         count,
                            GCancellable  *cancellable,
                            GError       **error)
{
    uint8_t *buf_in    = mock_type (uint8_t*);
    size_t   buf_index = mock_type (size_t);
    GError  *error_in  = mock_type (GError*);
    ssize_t  ret       = mock_type (ssize_t);
    UNUSED_PARAM(istream);
    UNUSED_PARAM(cancellable);

    g_debug ("%s", __func__);
    if (error_in != NULL && error != NULL) {
        *error = error_in;
    }
    /* be careful comparing signed to unsigned values */
    if (ret > 0) {
        assert_true (ret <= (ssize_t)count);
        memcpy (buf, &buf_in [buf_index], ret);
    }

    return ret;
}
/* global static input array used by read_data* tests */
static uint8_t buf_in [MAX_BUF] = {
    /* header */
    0x80, 0x02, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00,
    0x00, 0x00,
    /* body */
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef
};
/*
 * Data structure to hold data for read tests.
 */
typedef struct {
    GIOStream *iostream;
    size_t  index;
    uint8_t buf_out [MAX_BUF];
    size_t  buf_size;
} data_t;

static int
read_data_setup (void **state)
{
    data_t *data;

    data = calloc (1, sizeof (data_t));
    data->buf_size = 26;
    data->iostream = (GIOStream*)1;

    *state = data;
    return 0;
}

static int
read_data_teardown (void **state)
{
    data_t *data = *state;

    if (data != NULL) {
        free (data);
    }
    return 0;
}
/*
 */
static void
create_socket_pair_success_test (void **state)
{
    int ret, client_fd, server_fd, flags = 0;
    UNUSED_PARAM(state);

#if !defined(__FreeBSD__)
    flags = O_CLOEXEC;
#endif

    ret = create_socket_pair (&client_fd, &server_fd, flags);
    if (ret == -1)
        g_error ("create_pipe_pair failed: %s", strerror (errno));
    close (client_fd);
}

/*
 * Simple call to read wrapper function. Returns exactly what we ask for.
 * We check to be sure return value is 0, the index variable is updated
 * properly and that the output data buffer is the same as the input.
 */
static void
read_data_success_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    /* prime the wrap queue for a successful read */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->index);
    will_return (__wrap_g_input_stream_read, NULL);
    will_return (__wrap_g_input_stream_read, data->buf_size);

    ret = read_data (NULL, &data->index, data->buf_out, data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf_in, data->buf_size);
}

/*
 * This tests a simple error case where read returns -1 and errno is set to
 * EIO. In this case the index should remain unchanged (0).
 */
static void
read_data_error_test (void **state)
{
    data_t *data = *state;
    int ret = 0;
    GError *error;

    error = g_error_new (UTIL_UNIT_ERROR,
                         G_IO_ERROR_WOULD_BLOCK,
                         "g-io-error-would-block");
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->index);
    will_return (__wrap_g_input_stream_read, error);
    will_return (__wrap_g_input_stream_read, -1);

    ret = read_data (NULL, &data->index, data->buf_out, data->buf_size);
    assert_int_equal (ret, G_IO_ERROR_WOULD_BLOCK);
    assert_int_equal (data->index, 0);
}
/*
 * This test covers the 'short read'. A single call to 'read_data' results in
 * a the first read returning half the data (not an error), followed by
 * the second half obtained through a second 'read' call.
 */
static void
read_data_short_success_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    /* prime the wrap queue for a short read (half the requested size) */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->index);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, data->buf_size / 2);
    /* do it again for the second half of the read */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->buf_size / 2);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, data->buf_size / 2);

    ret = read_data (NULL, &data->index, data->buf_out, data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf_in, data->buf_size);
}
/*
 * This test covers a short read followed by a failing 'read'. NOTE: half of
 * the buffer is filled due to the first read succeeding.
 */
static void
read_data_short_err_test (void **state)
{
    data_t *data = *state;
    int ret = 0;
    GError *error;

    /*
     * Prime the wrap queue for another short read, again half the requested
     * size.
     */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->index);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, data->buf_size / 2);
    /* */
    error = g_error_new (UTIL_UNIT_ERROR,
                         G_IO_ERROR_WOULD_BLOCK,
                         "g-io-error-would-block");
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->buf_size / 2);
    will_return (__wrap_g_input_stream_read, error);
    will_return (__wrap_g_input_stream_read, -1);
    /* read the second half of the buffer, the index maintains the state */
    ret = read_data (NULL, &data->index, data->buf_out, data->buf_size);
    assert_int_equal (ret, G_IO_ERROR_WOULD_BLOCK);
    assert_int_equal (data->index, data->buf_size / 2);
    assert_memory_equal (data->buf_out, buf_in, data->buf_size / 2);
}
/*
 * This test covers a single call returning EOF. This is signaled to the
 * through the return value which is -1.
 */
static void
read_data_eof_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->index);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);

    ret = read_data (NULL, &data->index, data->buf_out, data->buf_size);
    assert_int_equal (ret, -1);
    assert_int_equal (data->index, 0);
}
/*
 * This test covers the common case when reading a tpm command / response
 * buffer. read_tpm_buffer first reads the header (10 bytes), extracts the
 * size field from the header, and then reads this many bytes into the
 * provided buffer.
 */
static void
read_tpm_buf_success_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    /* prime read to successfully produce the header */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 10);
    /* prime read to successfully produce the rest of the buffer */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 10);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, data->buf_size - 10);

    ret = read_tpm_buffer (NULL,
                           &data->index,
                           data->buf_out,
                           data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf_in, data->buf_size);
}
/*
 * This tests the code path where a header is successfully read and the size
 * field in the header is the size of the header.
 */
static void
read_tpm_buf_header_only_success_test (void **state)
{
    data_t *data = *state;
    int ret = 0;
    uint8_t buf [10] = {
        0x80, 0x02, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00
    };

    /* prime read to successfully produce the header */
    data->buf_size = 10;
    will_return (__wrap_g_input_stream_read, buf);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 10);

    ret = read_tpm_buffer (NULL,
                           &data->index,
                           data->buf_out,
                           data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf, data->buf_size);
}
/*
 * Test the condition where the header read has a size larger than the
 * provided buffer.
 */
static void
read_tpm_buf_lt_header_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    ret = read_tpm_buffer (NULL, &data->index, data->buf_out, 8);
    assert_int_equal (ret, EPROTO);
    assert_int_equal (data->index, 0);
}

static void
read_tpm_buf_short_header_test (void **state)
{
    data_t *data = *state;
    int ret = 0;
    GError *error;

    /*
     * Prime read to successfully produce 4 bytes. This is a short read to
     * exercise the error handling path in the function under test.
     */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 4);

    error = g_error_new (UTIL_UNIT_ERROR,
                         G_IO_ERROR_WOULD_BLOCK,
                         "g-io-error-would-block");
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 4);
    will_return (__wrap_g_input_stream_read, error);
    will_return (__wrap_g_input_stream_read, -1);

    ret = read_tpm_buffer (NULL,
                           &data->index,
                           data->buf_out,
                           data->buf_size);
    assert_int_equal (ret, G_IO_ERROR_WOULD_BLOCK);
    assert_int_equal (data->index, 4);
    assert_memory_equal (data->buf_out, buf_in, data->index);
}
/*
 * Test the condition where the header read has a size larger than the
 * provided buffer.
 */
static void
read_tpm_buf_lt_body_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    /*
     * Prime read to successfully produce 10 bytes. This is the number of
     * bytes that the first 'read' syscall is asked to produce (header).
     */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, data->index);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 10);

    ret = read_tpm_buffer (NULL, &data->index, data->buf_out, 11);
    assert_int_equal (ret, EPROTO);
    assert_int_equal (data->index, 10);
    assert_memory_equal (data->buf_out, buf_in, 10);
}
/*
 * Read the header in one go. The second call to 'read' will be an attempt to
 * read the body of the command. We setup the mock stuff such that we get a
 * short read, followed by the rest of the command.
 */
static void
read_tpm_buf_short_body_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    /*
     * Prime read to successfully produce 10 bytes. This is the number of
     * bytes that the first 'read' syscall is asked to produce (header).
     */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 10);
    /* Now cause a short read when getting the body of the buffer. */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 10);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 5);
    /* And then the rest of the buffer */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 15);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, data->buf_size - 15);

    ret = read_tpm_buffer (NULL,
                           &data->index,
                           data->buf_out,
                           data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf_in, data->buf_size);
}
/*
 * The following tests cover cases where the 'read_tpm_buffer' function is
 * called with the tpm buffer is partially populated (from previous calls to
 * same function) with the index value set appropriately.
 */
/*
 * This test sets up a buffer such that the first 4 bytes of the header have
 * already been populated and the index set accordingly. The read_tpm_buffer
 * function is then invoked as though it has bee previously interrupted.
 * This results in the last 6 bytes of the header being read, followed by
 * the rest of the buffer.
 */
static void
read_tpm_buf_populated_header_half_test (void **state)
{
    data_t *data = *state;
    int ret = 0;
    /* buffer already has 4 bytes of data */
    memcpy (data->buf_out, buf_in, 4);

    /* prime read to successfully produce the header */
    data->index = 4;
    /* read the last 6 bytes of the header */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 4);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 6);
    /* read the rest of the body (26 - 10) */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 10);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 16);

    ret = read_tpm_buffer (NULL,
                           &data->index,
                           data->buf_out,
                           data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf_in, data->buf_size);
}
/*
 * This test is a bit weird. We pass a buffer that has already had the header
 * populated and the index incremented to the end of the header. The call to
 * read_tpm_buffer is smart enough to know the header has already been read
 * so it extracts the size from the header. This is however the size of the
 * header and so the function returns before any read operation happens.
 */
static void
read_tpm_buf_populated_header_only_test (void **state)
{
    data_t *data = *state;
    int ret = 0;
    uint8_t buf [10] = {
        0x80, 0x02, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00
    };

    /* prime read to successfully produce the header */
    data->buf_size = 10;
    data->index = 10;
    memcpy (data->buf_out, buf, data->buf_size);

    ret = read_tpm_buffer (NULL,
                           &data->index,
                           data->buf_out,
                           data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf, data->buf_size);
}
/*
 * This test initializes a buffer as though it has already been populated
 * beyond the header (16 bytes) with the index set accordingly. The call to
 * read_tpm_buffer then picks up where it left off filling in the remainder
 * of the buffer.
 */
static void
read_tpm_buf_populated_body_test (void **state)
{
    data_t *data = *state;
    int ret = 0;

    memcpy (data->buf_out, buf_in, 16);

    /* prime read to successfully produce the header */
    data->index = 16;

    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 16);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 10);

    ret = read_tpm_buffer (NULL,
                           &data->index,
                           data->buf_out,
                           data->buf_size);
    assert_int_equal (ret, 0);
    assert_int_equal (data->index, data->buf_size);
    assert_memory_equal (data->buf_out, buf_in, data->buf_size);
}

static void
read_tpm_buf_alloc_success_test (void **state)
{
    data_t *data = *state;
    uint8_t *buf;
    size_t   buf_size;

    /* prime read to successfully produce the header */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 10);
    /* prime read to successfully produce the rest of the buffer */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 10);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, data->buf_size - 10);

    buf = read_tpm_buffer_alloc ((GInputStream*)1, &buf_size);
    assert_non_null (buf);
    assert_int_equal (buf_size, data->buf_size);
    assert_memory_equal (buf, buf_in, data->buf_size);
    g_free (buf);
}

static void
read_tpm_buf_alloc_header_only_test (void **state)
{
    data_t *data = *state;
    uint8_t buf [10] = {
        0x80, 0x02, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t *buf_out = NULL;

    /* prime read to successfully produce the header */
    data->buf_size = 10;
    will_return (__wrap_g_input_stream_read, buf);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 10);

    buf_out = read_tpm_buffer_alloc ((GInputStream*)1, &data->buf_size);
    assert_non_null (buf_out);
    assert_int_equal (data->buf_size, TPM_HEADER_SIZE);
    assert_memory_equal (buf_out, buf, data->buf_size);
    g_free (buf_out);
}
static void
read_tpm_buf_alloc_eof_test (void **state)
{
    uint8_t *buf;
    size_t   buf_size;
    UNUSED_PARAM(state);

    /* prime read to successfully produce the header */
    will_return (__wrap_g_input_stream_read, buf_in);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);
    will_return (__wrap_g_input_stream_read, 0);

    buf = read_tpm_buffer_alloc ((GInputStream*)1, &buf_size);
    assert_null (buf);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (write_in_one),
        cmocka_unit_test (write_in_two),
        cmocka_unit_test (write_in_three),
        cmocka_unit_test (write_error),
        cmocka_unit_test (write_zero),
        cmocka_unit_test (create_socket_pair_success_test),
        /* read_data tests */
        cmocka_unit_test_setup_teardown (read_data_success_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_data_short_success_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_data_short_err_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_data_error_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_data_eof_test,
                                         read_data_setup,
                                         read_data_teardown),
        /* read_tpm_buf tests */
        cmocka_unit_test_setup_teardown (read_tpm_buf_success_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_header_only_success_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_short_header_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_lt_header_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_lt_body_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_short_body_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_populated_header_half_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_populated_header_only_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_populated_body_test,
                                         read_data_setup,
                                         read_data_teardown),
        /* read_tpm_buffer_alloc*/
        cmocka_unit_test_setup_teardown (read_tpm_buf_alloc_success_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_alloc_header_only_test,
                                         read_data_setup,
                                         read_data_teardown),
        cmocka_unit_test_setup_teardown (read_tpm_buf_alloc_eof_test,
                                         read_data_setup,
                                         read_data_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
