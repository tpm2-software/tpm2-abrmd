#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"

#define READ_SIZE  UTIL_BUF_SIZE
#define WRITE_SIZE 10

void*
__wrap_realloc (void   *buf,
                size_t  count)
{
    errno = (int) mock ();
    return (void*) mock ();
}

void
fail_on_first (void **state)
{
    ssize_t read;
    size_t  total_read;
    guint8  *buf;

    will_return (__wrap_realloc, ENOMEM);
    will_return (__wrap_realloc, NULL);
    read = read_till_block (0, &buf, &total_read);
    assert_int_equal (read, -1);
}

/* Cause a failure on the second call to realloc. This requires that we have
 * one successful round so we must write data to the fd so that the call to
 * read succeeds.
 */
void
fail_on_second (void **state)
{
    ssize_t read;
    size_t total_read;
    guint8 *buf, *priv_buf;
    gint fds[2] = {0}, ret = 0;

    ret = pipe (fds);
    assert_int_equal (ret, 0);
    priv_buf = calloc (1, UTIL_BUF_SIZE);
    will_return (__wrap_realloc, 0);
    will_return (__wrap_realloc, priv_buf);
    will_return (__wrap_realloc, ENOMEM);
    will_return (__wrap_realloc, NULL);
    write (fds[1], priv_buf, UTIL_BUF_SIZE);
    read = read_till_block (fds[0], &buf, &total_read);
    assert_int_equal (read, -1);
}

gint
main (gint    argc,
      gchar  *argv[])
{
    const UnitTest tests[] = {
        unit_test (fail_on_first),
        unit_test (fail_on_second),
    };
    return run_tests (tests);
}
