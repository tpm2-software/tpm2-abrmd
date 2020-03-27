/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "connection.h"
#include "connection-manager.h"
#include "tabrmd-defaults.h"
#include "util.h"

static void
connection_manager_allocate_test (void **state)
{
    ConnectionManager *manager = NULL;
    UNUSED_PARAM(state);

    manager = connection_manager_new (TABRMD_CONNECTIONS_MAX_DEFAULT);
    assert_non_null (manager);
    g_object_unref (manager);
}

static int
connection_manager_setup (void **state)
{
    ConnectionManager *manager = NULL;

    manager = connection_manager_new (TABRMD_CONNECTIONS_MAX_DEFAULT);
    assert_non_null (manager);
    *state = manager;
    return 0;
}

static int
connection_manager_teardown (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);

    g_object_unref (manager);
    return 0;
}

static void
connection_manager_insert_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, client_fd;
    GIOStream *iostream;

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 5, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    ret = connection_manager_insert (manager, connection);
    assert_int_equal (ret, 0);
}

static void
connection_manager_lookup_fd_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL, *connection_lookup = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, client_fd;
    GIOStream *iostream;

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 5, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    ret = connection_manager_insert (manager, connection);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
    connection_lookup = connection_manager_lookup_istream (manager,
                                                           connection_key_istream (connection));
    assert_int_equal (connection, connection_lookup);
    g_object_unref (connection_lookup);
}

static void
connection_manager_lookup_id_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL, *connection_lookup = NULL;
    HandleMap   *handle_map = NULL;
    GIOStream *iostream;
    gint ret, client_fd;

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 5, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    ret = connection_manager_insert (manager, connection);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
    connection_lookup = connection_manager_lookup_id (manager, *(gint64*)connection_key_id (connection));
    assert_int_equal (connection, connection_lookup);
}

static void
connection_manager_remove_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL;
    GIOStream *iostream;
    HandleMap   *handle_map = NULL;
    gint ret_int, client_fd;
    gboolean ret_bool;

    handle_map = handle_map_new (TPM2_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    iostream = create_connection_iostream (&client_fd);
    connection = connection_new (iostream, 5, handle_map);
    g_object_unref (handle_map);
    g_object_unref (iostream);
    ret_int = connection_manager_insert (manager, connection);
    assert_int_equal (ret_int, 0);
    ret_bool = connection_manager_remove (manager, connection);
    assert_true (ret_bool);
}

int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test (connection_manager_allocate_test),
        cmocka_unit_test_setup_teardown (connection_manager_insert_test,
                                         connection_manager_setup,
                                         connection_manager_teardown),
        cmocka_unit_test_setup_teardown (connection_manager_lookup_fd_test,
                                         connection_manager_setup,
                                         connection_manager_teardown),
        cmocka_unit_test_setup_teardown (connection_manager_lookup_id_test,
                                         connection_manager_setup,
                                         connection_manager_teardown),
        cmocka_unit_test_setup_teardown (connection_manager_remove_test,
                                         connection_manager_setup,
                                         connection_manager_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
