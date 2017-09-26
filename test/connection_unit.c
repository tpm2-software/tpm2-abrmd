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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "connection.h"
#include "util.h"

typedef struct connection_test_data {
    Connection *connection;
    GSocket    *client_socket;
} connection_test_data_t;

static int
write_read (GSocket    *socket_in,
            GSocket    *socket_out,
            const char *buf,
            size_t      length)
{
    char out_buf[256] = { 0 };
    ssize_t ret;

    ret = g_socket_send (socket_in, buf, length, NULL, NULL);
    if (ret != length)
        g_error ("error writing to fds[1]: %s", strerror (errno));
    ret = g_socket_receive (socket_out, out_buf, length, NULL, NULL);
    if (ret != length)
        g_error ("error reading from fds[0]: %s", strerror (errno));

    return ret;
}

static void
connection_allocate_test (void **state)
{
    HandleMap   *handle_map = NULL;
    Connection *connection = NULL;
    gint client_fd;
    GSocket *server_socket;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    server_socket = create_socket_connection (&client_fd);
    connection = connection_new (server_socket, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (server_socket);
    assert_non_null (connection);
    assert_true (client_fd >= 0);
    g_object_unref (connection);
}

static int
connection_setup (void **state)
{
    connection_test_data_t *data = NULL;
    HandleMap *handle_map = NULL;
    int client_fd;
    GSocket *server_socket;

    data = calloc (1, sizeof (connection_test_data_t));
    assert_non_null (data);
    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    server_socket = create_socket_connection (&client_fd);
    data->connection = connection_new (server_socket, 0, handle_map);
    data->client_socket = g_socket_new_from_fd (client_fd, NULL);
    assert_non_null (data->connection);
    g_object_unref (handle_map);
    g_object_unref (server_socket);
    *state = data;
    return 0;
}

static int
connection_teardown (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;

    g_object_unref (data->connection);
    g_object_unref (data->client_socket);
    free (data);
    return 0;
}

static void
connection_key_socket_test (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    Connection *connection = CONNECTION (data->connection);
    gpointer *key = NULL;

    key = connection_key_socket (connection);
    assert_ptr_equal (connection->socket, key);
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

/* connection_client_to_server_test begin
 * This test creates a connection and communicates with it as though the pipes
 * that are created as part of connection setup.
 */
static void
connection_client_to_server_test (void ** state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    gint ret = 0;

    ret = write_read (data->connection->socket, data->client_socket, "test", strlen ("test"));
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

    ret = write_read (data->client_socket,
                      data->connection->socket,
                      "test",
                      strlen ("test"));
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
        cmocka_unit_test_setup_teardown (connection_key_socket_test,
                                         connection_setup,
                                         connection_teardown),
        cmocka_unit_test_setup_teardown (connection_key_id_test,
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
