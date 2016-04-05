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

#include "connection-manager.h"

static void
connection_manager_create_pipe_pair_test (void **state)
{
    int ret, fds[2];
    ssize_t rw_ret;
    char buf[128] = { 0 };
    const char *test_str = "test";

    ret = create_pipe_pair (&fds[0], &fds[1], O_CLOEXEC);
    if (ret == -1)
        g_error ("create_pipe_pair failed: %s", strerror (errno));
    rw_ret = write (fds[1], test_str, strlen(test_str));
    if (rw_ret == -1)
        g_error ("error writing to fds[1]: %s", strerror (errno));
    rw_ret = read (fds[0], buf, 128);
    if (rw_ret == -1)
        g_error ("error reading from fds[0]: %s", strerror (errno));
    assert_true (strncmp(buf, test_str, strlen (test_str)) == 0);
}

/* Simulate running the echo thread by creating all of the data it expects,
 * writing a message to it on the right pipe, invoking it directly on the
 * main thread and then readin gout the data that it echo'd back to us.
 */
static void
connection_manager_echo_thread_test (void **state)
{
    echo_data_t echo_data = { 0, };
    int ret, client_fds[2];
    char buf[128] = { 0, };
    const char *test_str = "test";
    ssize_t rw_ret = 0;

    ret = create_pipe_pairs (echo_data.fds, client_fds, O_CLOEXEC);
    if (ret == -1)
        g_error ("connection_manager_echo_thread_test failed to make pipe "
                 "pair: %s", strerror (errno));
    /* write test data string "test" to the send pipe */
    rw_ret = write (client_fds[1], test_str, strlen (test_str));
    if (rw_ret == -1)
        g_error ("write failed: %s", strerror(errno));
    echo_start (&echo_data);
    /* read test string back */
    rw_ret = read (client_fds[0], buf, 128);
    if (rw_ret == -1)
        g_error ("read failed: %s", strerror(errno));
    assert_true(strncmp(buf, test_str, strlen (test_str)) == 0);
}

int
main(int argc, char* argv[])
{
    const UnitTest tests[] = {
        unit_test(connection_manager_create_pipe_pair_test),
        unit_test(connection_manager_echo_thread_test),
    };
    return run_tests(tests);
}
