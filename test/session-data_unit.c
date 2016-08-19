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

#include "session-data.h"

typedef struct session_test_data {
    SessionData *session;
    gint receive_fd;
    gint send_fd;
} session_test_data_t;

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
session_create_pipe_pair_test (void **state)
{
    int ret, fds[2];
    const char *test_str = "test";
    size_t length = strlen (test_str);

    ret = create_pipe_pair (&fds[0], &fds[1], O_CLOEXEC);
    if (ret == -1)
        g_error ("create_pipe_pair failed: %s", strerror (errno));
    assert_int_equal (write_read (fds[1], fds[0], test_str, length), length);
}

static void
session_create_pipe_pairs_test (void **state)
{
    int client_fds[2], server_fds[2], ret;
    const char *test_str = "test";
    size_t length = strlen (test_str);

    assert_int_equal (create_pipe_pairs (client_fds, server_fds, O_CLOEXEC), 0);
    ret = write_read (client_fds[1], server_fds[0], test_str, length);
    assert_int_equal (ret, length);
    ret = write_read (server_fds[1], client_fds[0], test_str, length);
    assert_int_equal (ret, length);
}

static void
session_allocate_test (void **state)
{
    SessionData *session = NULL;
    gint receive_fd, send_fd;

    session = session_data_new (&receive_fd, &send_fd, 0);
    assert_non_null (session);
    assert_true (receive_fd >= 0);
    assert_true (send_fd >= 0);
    g_object_unref (session);
}

static void
session_setup (void **state)
{
    session_test_data_t *data = NULL;

    data = calloc (1, sizeof (session_test_data_t));
    assert_non_null (data);
    data->session = session_data_new (&data->receive_fd, &data->send_fd, 0);
    assert_non_null (data->session);
    *state = data;
}

static void
session_teardown (void **state)
{
    session_test_data_t *data = (session_test_data_t*)*state;

    g_object_unref (data->session);
    close (data->receive_fd);
    close (data->send_fd);
    free (data);
}

static void
session_key_fd_test (void **state)
{
    session_test_data_t *data = (session_test_data_t*)*state;
    SessionData *session = SESSION_DATA (data->session);
    int *key = NULL;

    key = (int*)session_data_key_fd (session);
    assert_int_equal (session->receive_fd, *key);
}

static void
session_key_id_test (void **state)
{
    session_test_data_t *data = (session_test_data_t*)*state;
    SessionData *session = SESSION_DATA (data->session);
    guint64 *key = NULL;

    key = (guint64*)session_data_key_id (session);
    assert_int_equal (session->id, *key);
}

static void
session_equal_fd_test (void **state)
{
    session_test_data_t *data = (session_test_data_t*)*state;
    const gint *key = session_data_key_fd (data->session);
    assert_true (session_data_equal_fd (key, session_data_key_fd (data->session)));
}

static void
session_equal_id_test (void **state)
{
    session_test_data_t *data = (session_test_data_t*)*state;
    const guint64 *key = session_data_key_id (data->session);
    assert_true (session_data_equal_id (key, session_data_key_id (data->session)));
}

/* session_client_to_server_test begin
 * This test creates a session and communicates with it as though the pipes
 * that are created as part of session setup.
 */
static void
session_client_to_server_test (void ** state)
{
    session_test_data_t *data = (session_test_data_t*)*state;
    gint ret = 0;

    ret = write_read (data->session->send_fd, data->receive_fd, "test", strlen ("test"));
    if (ret == -1)
        g_print ("write_read failed: %d\n", ret);
    assert_int_equal (ret, strlen ("test"));
}
/* session_client_to_server_test end */
/* session_server_to_client_test begin
 * Do the same in the reverse direction.
 */
static void
session_server_to_client_test (void **state)
{
    session_test_data_t *data = (session_test_data_t*)*state;
    gchar buf[256] = { 0 };
    gint ret = 0;

    ret = write_read (data->send_fd, data->session->receive_fd, "test", strlen ("test"));
    if (ret == -1)
        g_print ("write_read failed: %d\n", ret);
    assert_int_equal (ret, strlen ("test"));
}
/* session_server_to_client_test end */

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test (session_allocate_test),
        unit_test (session_create_pipe_pair_test),
        unit_test (session_create_pipe_pairs_test),
        unit_test_setup_teardown (session_key_fd_test,
                                  session_setup,
                                  session_teardown),
        unit_test_setup_teardown (session_key_id_test,
                                  session_setup,
                                  session_teardown),
        unit_test_setup_teardown (session_equal_fd_test,
                                  session_setup,
                                  session_teardown),
        unit_test_setup_teardown (session_equal_id_test,
                                  session_setup,
                                  session_teardown),
        unit_test_setup_teardown (session_client_to_server_test,
                                  session_setup,
                                  session_teardown),
        unit_test_setup_teardown (session_server_to_client_test,
                                  session_setup,
                                  session_teardown),
    };
    return run_tests(tests);
}
