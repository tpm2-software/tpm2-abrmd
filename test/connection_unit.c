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

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "connection.h"

typedef struct connection_test_data {
    Connection *connection;
    gint client_fd;
} connection_test_data_t;

static int
write_read (int write_fd, int read_fd, const char* buf, size_t length)
{
    char out_buf[256] = { 0 };
    ssize_t ret;

    ret = write (write_fd, buf, length);
    if (ret != length)
        g_error ("error writing to fds[1]: %s", strerror (errno));
    ret = read (read_fd, out_buf, length);
    if (ret != length)
        g_error ("error reading from fds[0]: %s", strerror (errno));

    return ret;
}

static void
connection_create_pipe_pair_test (void **state)
{
    int ret, client_fd, server_fd;
    const char *test_str = "test";
    size_t length = strlen (test_str);

    ret = create_fd_pair (&client_fd, &server_fd, O_CLOEXEC);
    if (ret == -1)
        g_error ("create_pipe_pair failed: %s", strerror (errno));
    assert_int_equal (write_read (client_fd, server_fd, test_str, length), length);
}

static void
connection_create_pipe_pairs_test (void **state)
{
    int client_fd, server_fd, ret;
    const char *test_str = "test";
    size_t length = strlen (test_str);

    assert_int_equal (create_fd_pair (&client_fd, &server_fd, O_CLOEXEC), 0);
    ret = write_read (client_fd, server_fd, test_str, length);
    assert_int_equal (ret, length);
    ret = write_read (server_fd, client_fd, test_str, length);
    assert_int_equal (ret, length);
}

static void
connection_allocate_test (void **state)
{
    HandleMap   *handle_map = NULL;
    Connection *connection = NULL;
    gint client_fd;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    connection = connection_new (&client_fd, 0, handle_map);
    assert_non_null (connection);
    assert_true (client_fd >= 0);
    g_object_unref (handle_map);
    g_object_unref (connection);
}

static int
connection_setup (void **state)
{
    connection_test_data_t *data = NULL;
    HandleMap *handle_map = NULL;

    data = calloc (1, sizeof (connection_test_data_t));
    assert_non_null (data);
    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    data->connection = connection_new (&data->client_fd, 0, handle_map);
    assert_non_null (data->connection);
    g_object_unref (handle_map);
    *state = data;
    return 0;
}

static int
connection_teardown (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;

    g_object_unref (data->connection);
    close (data->client_fd);
    free (data);
    return 0;
}

static void
connection_key_fd_test (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    Connection *connection = CONNECTION (data->connection);
    int *key = NULL;

    key = (int*)connection_key_fd (connection);
    assert_int_equal (connection->fd, *key);
}

static void
connection_key_id_test (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    Connection *connection = CONNECTION (data->connection);
    guint64 *key = NULL;

    key = (guint64*)connection_key_id (connection);
    assert_int_equal (connection->id, *key);
}

static void
connection_equal_fd_test (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    const gint *key = connection_key_fd (data->connection);
    assert_true (connection_equal_fd (key, connection_key_fd (data->connection)));
}

static void
connection_equal_id_test (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    const guint64 *key = connection_key_id (data->connection);
    assert_true (connection_equal_id (key, connection_key_id (data->connection)));
}

/* connection_client_to_server_test begin
 * This test creates a connection and communicates with it as though the pipes
 * that are created as part of connection setup.
 */
static void
connection_client_to_server_test (void ** state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    gint ret = 0;

    ret = write_read (data->connection->fd, data->client_fd, "test", strlen ("test"));
    if (ret == -1)
        g_print ("write_read failed: %d\n", ret);
    assert_int_equal (ret, strlen ("test"));
}
/* connection_client_to_server_test end */
/* connection_server_to_client_test begin
 * Do the same in the reverse direction.
 */
static void
connection_server_to_client_test (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    gint ret = 0;

    ret = write_read (data->client_fd, data->connection->fd, "test", strlen ("test"));
    if (ret == -1)
        g_print ("write_read failed: %d\n", ret);
    assert_int_equal (ret, strlen ("test"));
}
/* connection_server_to_client_test end */

int
main(int argc, char* argv[])
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (connection_allocate_test),
        cmocka_unit_test (connection_create_pipe_pair_test),
        cmocka_unit_test (connection_create_pipe_pairs_test),
        cmocka_unit_test_setup_teardown (connection_key_fd_test,
                                         connection_setup,
                                         connection_teardown),
        cmocka_unit_test_setup_teardown (connection_key_id_test,
                                         connection_setup,
                                         connection_teardown),
        cmocka_unit_test_setup_teardown (connection_equal_fd_test,
                                         connection_setup,
                                         connection_teardown),
        cmocka_unit_test_setup_teardown (connection_equal_id_test,
                                         connection_setup,
                                         connection_teardown),
        cmocka_unit_test_setup_teardown (connection_client_to_server_test,
                                         connection_setup,
                                         connection_teardown),
        cmocka_unit_test_setup_teardown (connection_server_to_client_test,
                                         connection_setup,
                                         connection_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
