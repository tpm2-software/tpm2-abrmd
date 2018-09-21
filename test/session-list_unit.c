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
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>


#include <tss2/tss2_tpm2_types.h>

#include "mock-io-stream.h"
#include "connection.h"
#include "session-entry.h"
#include "session-list.h"
#include "util.h"

#define INSERT_TEST_ID 0x1

typedef struct test_data {
    Connection *connection;
    SessionList *session_list;
} test_data_t;

static GIOStream*
test_iostream_new (void)
{
    GInputStream *input;
    GOutputStream *output;
    GIOStream *iostream;
    int fds[2];

    socketpair (AF_LOCAL, SOCK_STREAM, 0, fds);
    input = g_unix_input_stream_new (fds[0], TRUE);
    output = g_unix_output_stream_new (fds[1], TRUE);
    iostream = mock_io_stream_new (input, output);
    g_object_unref (input);
    g_object_unref (output);
    return iostream;
}
/*
 * Allocate a test_data_t structure to hold test data.
 * Create a SessionList object for use in testing.
 * Create a Connection object for use by tests.
 */
static int
session_list_setup (void **state)
{
    GIOStream *iostream = NULL;
    test_data_t *data = NULL;
    HandleMap *handle_map = NULL;

    data = calloc (1, sizeof (test_data_t));
    data->session_list = session_list_new (SESSION_LIST_MAX_ENTRIES_DEFAULT);
    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = test_iostream_new ();
    data->connection = connection_new (iostream, INSERT_TEST_ID, handle_map);
    g_clear_object (&iostream);
    g_clear_object (&handle_map);

    *state = data;
    return 0;
}

static int
session_list_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    if (data) {
        g_clear_object (&data->connection);
        g_clear_object (&data->session_list);
        free (data);
    }

    return 0;
}
/*
 * Simple object type checks.
 */
static void
session_list_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (G_IS_OBJECT (data->session_list));
    assert_true (IS_SESSION_LIST (data->session_list));
}
/* Simple test of SessionList insert function */
#define INSERT_TEST_HANDLE (TPM2_HR_TRANSIENT + 1)
static void
session_list_insert_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    SessionEntry *entry = NULL;

    entry = session_entry_new (data->connection, INSERT_TEST_HANDLE);
    assert_true (session_list_insert (data->session_list, entry));
    g_clear_object (&entry);
}
/*
 * Add 3 SessionEntry objects to a SessionList then verify size is reported
 * correctly.
 */
#define SIZE_TEST_HANDLE_1 (TPM2_HR_TRANSIENT + 1)
#define SIZE_TEST_HANDLE_2 (TPM2_HR_TRANSIENT + 2)
#define SIZE_TEST_HANDLE_3 (TPM2_HR_TRANSIENT + 3)
static void
session_list_size_three_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    SessionEntry *entry = NULL;

    entry = session_entry_new (data->connection, SIZE_TEST_HANDLE_1);
    session_list_insert (data->session_list, entry);
    g_clear_object (&entry);
    entry = session_entry_new (data->connection, SIZE_TEST_HANDLE_2);
    session_list_insert (data->session_list, entry);
    g_clear_object (&entry);
    entry = session_entry_new (data->connection, SIZE_TEST_HANDLE_3);
    session_list_insert (data->session_list, entry);
    g_clear_object (&entry);
    assert_int_equal (3, session_list_size (data->session_list));
}

/*
 * Test removal of a SessionEntry using the handle as an identifier. This
 * relies on the session_list_insert_test to add the SessionEntry required
 * by this test.
 */
static void
session_list_remove_handle_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    gboolean ret = FALSE;

    session_list_insert_test (state);
    ret = session_list_remove_handle (data->session_list, INSERT_TEST_HANDLE);
    assert_true (ret);
    assert_int_equal (session_list_size (data->session_list), 0);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (session_list_type_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_insert_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_size_three_test,
                                         session_list_setup,
                                         session_list_teardown),
        cmocka_unit_test_setup_teardown (session_list_remove_handle_test,
                                         session_list_setup,
                                         session_list_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
