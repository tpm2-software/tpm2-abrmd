/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>

#include <errno.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
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
    GIOStream  *client_iostream;
} connection_test_data_t;
/*
 * Data goes in the iostream_in, and out the iostream_out.
 */
static int
write_read (GIOStream  *iostream_in,
            GIOStream  *iostream_out,
            const char *buf,
            ssize_t      length)
{
    char out_buf[256] = { 0 };
    ssize_t ret;
    GInputStream *istream;
    GOutputStream *ostream;

    ostream = g_io_stream_get_output_stream (iostream_in);
    ret = g_output_stream_write (ostream, buf, length, NULL, NULL);
    if (ret != length)
        g_error ("error writing to fds[1]: %s", strerror (errno));
    istream = g_io_stream_get_input_stream (iostream_out);
    ret = g_input_stream_read (istream, out_buf, length, NULL, NULL);
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
    GIOStream *iostream;
    UNUSED_PARAM(state);

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
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
    GIOStream *iostream;
    GSocket *socket;

    data = calloc (1, sizeof (connection_test_data_t));
    assert_non_null (data);
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    data->connection = connection_new (iostream, 0, handle_map);
    g_object_unref (iostream);
    socket = g_socket_new_from_fd (client_fd, NULL);
    data->client_iostream =
        G_IO_STREAM (g_socket_connection_factory_create_connection (socket));
    g_object_unref (socket);
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
    g_object_unref (data->client_iostream);
    free (data);
    return 0;
}

static void
connection_key_socket_test (void **state)
{
    connection_test_data_t *data = (connection_test_data_t*)*state;
    Connection *connection = CONNECTION (data->connection);
    gpointer *key = NULL;

    key = connection_key_istream (connection);
    assert_ptr_equal (g_io_stream_get_input_stream (connection->iostream),
                                                    key);
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

    ret = write_read (data->connection->iostream, data->client_iostream, "test", strlen ("test"));
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

    ret = write_read (data->client_iostream,
                      data->connection->iostream,
                      "test",
                      strlen ("test"));
    if (ret == -1)
        g_print ("write_read failed: %d\n", ret);
    assert_int_equal (ret, strlen ("test"));
}
/* connection_server_to_client_test end */

int
main(void)
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
