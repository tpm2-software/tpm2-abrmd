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
#include <sys/select.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "connection-manager.h"
#include "sink-interface.h"
#include "source-interface.h"
#include "command-attrs.h"
#include "command-source.h"
#include "tpm2-command.h"

typedef struct source_test_data {
    ConnectionManager *manager;
    CommandAttrs   *command_attrs;
    CommandSource *source;
    Connection *connection;
    gboolean match;
} source_test_data_t;


Connection*
__wrap_connection_manager_lookup_fd (ConnectionManager *manager,
                                  gint            fd_in)
{
    g_debug ("__wrap_connection_manager_lookup_fd");
    return CONNECTION (mock ());
}

Tpm2Command*
__wrap_tpm2_command_new_from_fd (Connection *connection,
                                 gint         fd,
                                 TPMA_CC      attributes)
{
    g_debug ("__wrap_tpm2_command_from_fd");
    return TPM2_COMMAND (mock ());
}
void
__wrap_sink_enqueue (Sink     *sink,
                     GObject  *obj)
{
    Tpm2Command **command;

    g_debug ("__wrap_sink_enqueue");
    command = (Tpm2Command**)mock ();

    *command = TPM2_COMMAND (obj);
}
/* command_source_allocate_test begin
 * Test to allcoate and destroy a CommandSource.
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

static void
command_source_allocate_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = connection_manager_new (TPM_HT_TRANSIENT);

    *state = data;
}

static void
command_source_allocate_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    g_object_unref (data->source);
    g_object_unref (data->manager);
    free (data);
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
    ret = thread_cancel (THREAD (data->source));
    assert_int_equal (ret, 0);
    ret = thread_join (THREAD (data->source));
    assert_int_equal (ret, 0);
}

static void
command_source_start_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = connection_manager_new (TPM_HT_TRANSIENT);
    if (data->manager == NULL)
        g_error ("failed to allocate new connection_manager");
    data->command_attrs = command_attrs_new ();
    data->source = command_source_new (data->manager,
                                       data->command_attrs);
    if (data->source == NULL)
        g_error ("failed to allocate new command_source");

    *state = data;
}

static void
command_source_start_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    g_object_unref (data->source);
    g_object_unref (data->manager);
    g_object_unref (data->command_attrs);
    free (data);
}
/* command_source_start_test end */

/* command_source_connection_insert_test begin
 * In this test we create a connection source and all that that entails. We then
 * create a new connection and insert it into the connection manager. We then signal
 * to the source that there's a new connection in the manager by sending data to
 * it over the send end of the wakeup pipe "wakeup_send_fd". We then check the
 * connection_fdset in the source structure to be sure the receive end of the
 * connection pipe is set. This is how we know that the source is now watching
 * for data from the new connection.
 */
static void
command_source_wakeup_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = connection_manager_new (MAX_CONNECTIONS_DEFAULT);
    data->command_attrs = command_attrs_new ();
    data->source  = command_source_new (data->manager,
                                        data->command_attrs);
    *state = data;
}

static void
command_source_connection_insert_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    CommandSource *source = data->source;
    HandleMap     *handle_map;
    Connection *connection;
    gint ret, receive_fd, send_fd;

    /* */
    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    connection = connection_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    assert_false (FD_ISSET (connection->receive_fd, &source->connection_fdset));
    ret = thread_start(THREAD (source));
    assert_int_equal (ret, 0);
    connection_manager_insert (data->manager, connection);
    sleep (1);
    assert_true (FD_ISSET (connection->receive_fd, &source->connection_fdset));
    connection_manager_remove (data->manager, connection);
    thread_cancel (THREAD (source));
    thread_join (THREAD (source));
}
/* command_source_sesion_insert_test end */
/* command_source_connection_test start */
static void
command_source_connection_setup (void **state)
{
    source_test_data_t *data;

    data = calloc (1, sizeof (source_test_data_t));
    data->manager = connection_manager_new (TPM_HT_TRANSIENT);
    data->command_attrs = command_attrs_new ();
    data->source = command_source_new (data->manager,
                                       data->command_attrs);

    *state = data;
}
static void
command_source_connection_teardown (void **state)
{
    source_test_data_t *data = (source_test_data_t*)*state;

    thread_cancel (THREAD (data->source));
    thread_join (THREAD (data->source));

    g_object_unref (data->source);
    g_object_unref (data->manager);
    free (data);
}
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
command_source_connection_responder_success_test (void **state)
{
    struct source_test_data *data = (struct source_test_data*)*state;
    HandleMap   *handle_map;
    Connection *connection;
    Tpm2Command *command, *command_out;
    gint fds[2] = { 0, };
    guint8 *buffer;
    guint8 data_in [] = { 0x80, 0x01, 0x0,  0x0,  0x0,  0x16,
                          0x0,  0x0,  0x01, 0x7a, 0x0,  0x0,
                          0x0,  0x06, 0x0,  0x0,  0x01, 0x0,
                          0x0,  0x0,  0x0,  0x7f, 0x0a };
    gboolean result = FALSE;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    connection = connection_new (&fds[0], &fds[1], 0, handle_map);
    g_object_unref (handle_map);
    /**
     * We must dynamically allocate the buffer for the Tpm2Command since
     * it takes ownership of the data buffer and frees it as part of it's
     * instance finalize method.
     */
    buffer = calloc (1, sizeof (data_in));
    memcpy (buffer, data_in, sizeof (data_in));
    command = tpm2_command_new (connection, buffer, (TPMA_CC){ 0, });
    /* prime wraps */
    will_return (__wrap_connection_manager_lookup_fd, connection);
    will_return (__wrap_tpm2_command_new_from_fd, command);
    will_return (__wrap_sink_enqueue, &command_out);
    result = process_client_fd (data->source, 0, NULL);
    assert_true (result);

    assert_int_equal (command_out, command);
}
/* command_source_connection_test end */
int
main (int argc,
      char* argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (command_source_allocate_test,
                                  command_source_allocate_setup,
                                  command_source_allocate_teardown),
        unit_test_setup_teardown (command_source_start_test,
                                  command_source_start_setup,
                                  command_source_start_teardown),
        unit_test_setup_teardown (command_source_connection_insert_test,
                                  command_source_wakeup_setup,
                                  command_source_start_teardown),
        unit_test_setup_teardown (command_source_connection_responder_success_test,
                                  command_source_connection_setup,
                                  command_source_connection_teardown),
    };
    return run_tests (tests);
}
