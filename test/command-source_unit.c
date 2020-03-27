/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "connection-manager.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "command-attrs.h"
#include "command-source.h"
#include "tabrmd-defaults.h"
#include "tpm2-command.h"
#include "util.h"

typedef struct source_test_data {
    ConnectionManager *manager;
    CommandAttrs   *command_attrs;
    CommandSource *source;
    Connection *connection;
    gboolean match;
} source_test_data_t;


/* mock function to return TPM command attributes TPMA_CC */
TPMA_CC
__wrap_command_attrs_from_cc (CommandAttrs *attrs,
                              TPM2_CC        command_code)
{
    UNUSED_PARAM(attrs);
    UNUSED_PARAM(command_code);
    return (TPMA_CC)mock_type (UINT32);
}
Connection*
__wrap_connection_manager_lookup_istream (ConnectionManager *manager,
                                          GInputStream      *istream)
{
    UNUSED_PARAM(manager);
    UNUSED_PARAM(istream);
    g_debug ("%s", __func__);
    return CONNECTION (mock_ptr_type (GObject*));
}
gint
__wrap_connection_manager_remove      (ConnectionManager  *manager,
                                       Connection         *connection)
{
    UNUSED_PARAM(manager);
    UNUSED_PARAM(connection);
    return mock_type (int);
}
uint8_t*
__wrap_read_tpm_buffer_alloc (GSocket   *socket,
                              size_t    *buf_size)
{
    uint8_t *buf_src = mock_type (uint8_t*);
    uint8_t *buf_dst = NULL;
    size_t   size = mock_type (size_t);
    UNUSED_PARAM(socket);

    g_debug ("%s", __func__);
    buf_dst = g_malloc0 (size);
    memcpy (buf_dst, buf_src, size);
    *buf_size = size;

    return buf_dst;
}
void
__wrap_sink_enqueue (Sink     *sink,
                     GObject  *obj)
{
    GObject **object;
    UNUSED_PARAM(sink);

    g_debug ("%s", __func__);
    object = mock_ptr_type (GObject**);

    *object = G_OBJECT (obj);
    g_object_ref (*object);
}
/*
 * This wrap function allows us to gain access to the data that will be
 * passed to the source callback registered by the CommandSource object when
 * a new Connection is added. Without this callback it's not possible to get
 * access to the source_data_t structure created by the CommandSource insert
 * function which is required for us to simulate the callback function.
 */
void
__wrap_g_source_set_callback (GSource *source,
                              GSourceFunc func,
                              gpointer data,
                              GDestroyNotify notify)
{
    source_data_t *source_data = (source_data_t*)data;
    source_data_t **source_data_param = mock_type (source_data_t**);
    UNUSED_PARAM(source);
    UNUSED_PARAM(func);
    UNUSED_PARAM(notify);

    *source_data_param = source_data;
}
/* command_source_allocate_test begin
 * Test to allocate and destroy a CommandSource.
 */
static void
command_source_allocate_test (void **state)
{
    source_test_data_t *data = (source_test_data_t *)*state;

    data->command_attrs = command_attrs_new ();
    data->source = command_source_new (data->manager,
                                       data->command_attrs);
    assert_non_null (data->source);
}

static int
command_source_allocate_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = connection_manager_new (TABRMD_CONNECTIONS_MAX_DEFAULT);

    *state = data;
    return 0;
}
/* command_source_allocate end */

/* command_source_start_test
 * This is a basic usage flow test. Can it be started, canceled and joined.
 * We're testing the underlying pthread usage ... mostly.
 */
void
command_source_start_test (void **state)
{
    source_test_data_t *data;
    gint ret;

    data = (source_test_data_t*)*state;
    thread_start(THREAD (data->source));
    sleep (1);
    thread_cancel (THREAD (data->source));
    ret = thread_join (THREAD (data->source));
    assert_int_equal (ret, 0);
}

static int
command_source_start_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = connection_manager_new (TABRMD_CONNECTIONS_MAX_DEFAULT);
    if (data->manager == NULL)
        g_error ("failed to allocate new connection_manager");
    data->command_attrs = command_attrs_new ();
    data->source = command_source_new (data->manager,
                                       data->command_attrs);
    if (data->source == NULL)
        g_error ("failed to allocate new command_source");

    *state = data;
    return 0;
}

static int
command_source_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    g_clear_object (&data->source);
    g_clear_object (&data->manager);
    g_clear_object (&data->command_attrs);
    g_clear_pointer (&data, g_free);
    return 0;
}
/* command_source_start_test end */

/* command_source_connection_insert_test begin
 * In this test we create a connection source and all that that entails. We then
 * create a new connection and insert it into the connection manager. We then signal
 * to the source that there's a new connection in the manager by sending data to
 * it over the send end of the wakeup pipe "wakeup_send_fd". We then check the
 * receive_fdset in the source structure to be sure the receive end of the
 * connection pipe is set. This is how we know that the source is now watching
 * for data from the new connection.
 */
static int
command_source_connection_setup (void **state)
{
    source_test_data_t *data;

    g_debug ("%s", __func__);
    data = calloc (1, sizeof (source_test_data_t));
    data->manager = connection_manager_new (TABRMD_CONNECTIONS_MAX_DEFAULT);
    data->command_attrs = command_attrs_new ();
    data->source = command_source_new (data->manager,
                                       data->command_attrs);

    *state = data;
    return 0;
}
static void
command_source_connection_insert_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    source_data_t *source_data;
    CommandSource *source = data->source;
    GIOStream     *iostream;
    HandleMap     *handle_map;
    Connection *connection;
    gint ret, client_fd;

    g_debug ("%s", __func__);
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 5, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    /* starts the main loop in the CommandSource */
    ret = thread_start(THREAD (source));
    will_return (__wrap_g_source_set_callback, &source_data);
    assert_int_equal (ret, 0);
    /* normally a callback from the connection manager but we fake it here */
    sleep (1);
    command_source_on_new_connection (data->manager, connection, source);
    /* check internal state of the CommandSource*/
    assert_int_equal (g_hash_table_size (source->istream_to_source_data_map),
                      1);
    thread_cancel (THREAD (source));
    thread_join (THREAD (source));
    g_object_unref (connection);
}
/* command_source_session_insert_test end */

/**
 * A test: Test the command_source_connection_responder function. We do this
 * by creating a new Connection object, associating it with a new
 * Tpm2Command object (that we populate with a command body), and then
 * calling the command_source_connection_responder.
 * This function will in turn call the connection_manager_lookup_fd,
 * tpm2_command_new_from_fd, before finally calling the sink_enqueue function.
 * We mock these 3 functions to control the flow through the function under
 * test.
 * The most tricky bit to this is the way the __wrap_sink_enqueue function
 * works. Since this thing has no return value we pass it a reference to a
 * Tpm2Command pointer. It sets this to the Tpm2Command that it receives.
 * We determine success /failure for this test by verifying that the
 * sink_enqueue function receives the same Tpm2Command that we passed to
 * the command under test (command_source_connection_responder).
 */
static void
command_source_on_io_ready_success_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    GIOStream   *iostream;
    HandleMap   *handle_map;
    Connection *connection;
    Tpm2Command *command_out;
    gint client_fd;
    guint8 data_in [] = { 0x80, 0x01, 0x0,  0x0,  0x0,  0x17,
                          0x0,  0x0,  0x01, 0x7a, 0x0,  0x0,
                          0x0,  0x06, 0x0,  0x0,  0x01, 0x0,
                          0x0,  0x0,  0x0,  0x7f, 0x0a };

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
        /* prime wraps */
    will_return (__wrap_connection_manager_lookup_istream, connection);

    /* setup read of tpm buffer */
    will_return (__wrap_read_tpm_buffer_alloc, data_in);
    will_return (__wrap_read_tpm_buffer_alloc, sizeof (data_in));
    /* setup query for command attributes */
    will_return (__wrap_command_attrs_from_cc, 0);

    will_return (__wrap_sink_enqueue, &command_out);

    command_source_on_input_ready (NULL, data->source);

    assert_memory_equal (tpm2_command_get_buffer (command_out),
                         data_in,
                         sizeof (data_in));
    g_object_unref (command_out);
}
/*
 * This tests the CommandSource on_io_ready function for situations where
 * the GSocket associated with a client connection is closed. This causes
 * the attempt to read data from the socket to return an error indicating
 * that the socket was closed. In this case the function should return a
 * value telling GLib to remove the GSource from the main loop. Additionally
 * the data held internally by the CommandSource must be freed.
 */
static void
command_source_on_io_ready_eof_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    source_data_t *source_data;
    GIOStream   *iostream;
    HandleMap   *handle_map;
    Connection *connection;
    ControlMessage *msg;
    gint client_fd, hash_table_size;
    gboolean ret;

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 0, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
        /* prime wraps */
    will_return (__wrap_g_source_set_callback, &source_data);
    will_return (__wrap_connection_manager_lookup_istream, connection);
    will_return (__wrap_read_tpm_buffer_alloc, NULL);
    will_return (__wrap_read_tpm_buffer_alloc, 0);
    will_return (__wrap_sink_enqueue, &msg);
    will_return (__wrap_connection_manager_remove, TRUE);

    command_source_on_new_connection (data->manager, connection, data->source);
    ret = command_source_on_input_ready (g_io_stream_get_input_stream (connection->iostream), source_data);
    assert_int_equal (ret, G_SOURCE_REMOVE);
    hash_table_size = g_hash_table_size (data->source->istream_to_source_data_map);
    assert_int_equal (hash_table_size, 0);
    g_object_unref (msg);
}
/* command_source_connection_test end */
int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (command_source_allocate_test,
                                         command_source_allocate_setup,
                                         command_source_teardown),
        cmocka_unit_test_setup_teardown (command_source_start_test,
                                         command_source_start_setup,
                                         command_source_teardown),
        cmocka_unit_test_setup_teardown (command_source_connection_insert_test,
                                         command_source_connection_setup,
                                         command_source_teardown),
        cmocka_unit_test_setup_teardown (command_source_on_io_ready_success_test,
                                         command_source_connection_setup,
                                         command_source_teardown),
        cmocka_unit_test_setup_teardown (command_source_on_io_ready_eof_test,
                                         command_source_connection_setup,
                                         command_source_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
