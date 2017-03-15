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

#include "connection.h"
#include "connection-manager.h"

static void
connection_manager_allocate_test (void **state)
{
    ConnectionManager *manager = NULL;

    manager = connection_manager_new (MAX_CONNECTIONS_DEFAULT);
    assert_non_null (manager);
    g_object_unref (manager);
}

static void
connection_manager_setup (void **state)
{
    ConnectionManager *manager = NULL;

    manager = connection_manager_new (MAX_CONNECTIONS_DEFAULT);
    assert_non_null (manager);
    *state = manager;
}

static void
connection_manager_teardown (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);

    g_object_unref (manager);
}

static void
connection_manager_insert_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, receive_fd, send_fd;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    connection = connection_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret = connection_manager_insert (manager, connection);
    assert_int_equal (ret, 0);
}

static void
connection_manager_lookup_fd_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL, *connection_lookup = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, receive_fd, send_fd;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    connection = connection_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret = connection_manager_insert (manager, connection);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
    connection_lookup = connection_manager_lookup_fd (manager, *(int*)connection_key_fd (connection));
    assert_int_equal (connection, connection_lookup);
    g_object_unref (connection_lookup);
}

static void
connection_manager_lookup_id_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL, *connection_lookup = NULL;
    HandleMap   *handle_map = NULL;
    gint ret, receive_fd, send_fd;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    connection = connection_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret = connection_manager_insert (manager, connection);
    assert_int_equal (ret, TSS2_RC_SUCCESS);
    connection_lookup = connection_manager_lookup_id (manager, *(int*)connection_key_id (connection));
    assert_int_equal (connection, connection_lookup);
}

static void
connection_manager_remove_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *connection = NULL;
    HandleMap   *handle_map = NULL;
    gint ret_int, receive_fd, send_fd;
    gboolean ret_bool;

    handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    connection = connection_new (&receive_fd, &send_fd, 5, handle_map);
    g_object_unref (handle_map);
    ret_int = connection_manager_insert (manager, connection);
    assert_int_equal (ret_int, 0);
    ret_bool = connection_manager_remove (manager, connection);
    assert_true (ret_bool);
}

static void
connection_manager_set_fds_test (void **state)
{
    ConnectionManager *manager = CONNECTION_MANAGER (*state);
    Connection *first_connection = NULL, *second_connection = NULL;
    HandleMap   *first_handle_map = NULL, *second_handle_map = NULL;
    gint ret_int, receive_fd0, send_fd0, receive_fd1, send_fd1;
    fd_set manager_fds = { 0 };

    first_handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    first_connection = connection_new (&receive_fd0, &send_fd0, 5, first_handle_map);
    g_object_unref (first_handle_map);
    ret_int = connection_manager_insert (manager, first_connection);
    assert_int_equal (ret_int, 0);
    second_handle_map = handle_map_new (TPM_HT_TRANSIENT, MAX_ENTRIES_DEFAULT);
    second_connection = connection_new (&receive_fd1, &send_fd1, 5, second_handle_map);
    g_object_unref (second_handle_map);
    ret_int = connection_manager_insert (manager, second_connection);
    assert_int_equal (ret_int, 0);
    connection_manager_set_fds (manager, &manager_fds);
    for (ret_int = 0; ret_int < FD_SETSIZE; ++ret_int) {
        if (ret_int == first_connection->receive_fd) {
            assert_true (FD_ISSET(first_connection->receive_fd, &manager_fds) != 0);
        } else if (ret_int == second_connection->receive_fd) {
            assert_true (FD_ISSET (second_connection->receive_fd, &manager_fds) != 0);
        } else {
            assert_true (FD_ISSET (ret_int, &manager_fds) == 0);
        }
    }
    g_object_unref (G_OBJECT (first_connection));
    g_object_unref (G_OBJECT (second_connection));
}

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test (connection_manager_allocate_test),
        unit_test_setup_teardown (connection_manager_insert_test,
                                  connection_manager_setup,
                                  connection_manager_teardown),
        unit_test_setup_teardown (connection_manager_lookup_fd_test,
                                  connection_manager_setup,
                                  connection_manager_teardown),
        unit_test_setup_teardown (connection_manager_lookup_id_test,
                                  connection_manager_setup,
                                  connection_manager_teardown),
        unit_test_setup_teardown (connection_manager_remove_test,
                                  connection_manager_setup,
                                  connection_manager_teardown),
        unit_test_setup_teardown (connection_manager_set_fds_test,
                                  connection_manager_setup,
                                  connection_manager_teardown),
    };
    return run_tests(tests);
}
