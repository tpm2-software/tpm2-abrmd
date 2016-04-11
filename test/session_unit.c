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

#include "session.h"

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
    session_t *session = NULL;

    session = session_new ();
    assert_non_null (session);
    session_free (session);
}

static void
session_setup (void **state)
{
    session_t *session = NULL;

    session = session_new ();
    *state = session;
}

static void
session_teardown (void **state)
{
    session_t *session = (session_t*)*state;

    session_free (session);
}

static void
session_key_test (void **state)
{
    session_t *session = (session_t*)*state;
    int *key = NULL;

    key = (int*)session_key (session);
    assert_int_equal (session->receive_fd, *key);
}

static void
session_equal_test (void **state)
{
    session_t *session = (session_t*)*state;
    const gint *key = session_key (session);
    assert_true (session_equal (key, session_key (session)));
}

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test (session_allocate_test),
        unit_test (session_create_pipe_pair_test),
        unit_test (session_create_pipe_pairs_test),
        unit_test_setup_teardown (session_key_test,
                                  session_setup,
                                  session_teardown),
        unit_test_setup_teardown (session_equal_test,
                                  session_setup,
                                  session_teardown),
    };
    return run_tests(tests);
}
