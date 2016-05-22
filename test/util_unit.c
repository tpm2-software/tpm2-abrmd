#include <stdlib.h>
#include <stdio.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"

#define WRITE_SIZE 10

ssize_t
__wrap_write (gint         fd,
              const void  *buf,
              size_t       count)
{
    return (ssize_t) mock ();
}

void
write_in_one (void **state)
{
    ssize_t written;

    will_return (__wrap_write, WRITE_SIZE);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, WRITE_SIZE);
}

void
write_in_two (void **state)
{
    ssize_t written;

    will_return (__wrap_write, 5);
    will_return (__wrap_write, 5);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, WRITE_SIZE);
}

void
write_in_three (void **state)
{
    ssize_t written;

    will_return (__wrap_write, 3);
    will_return (__wrap_write, 3);
    will_return (__wrap_write, 4);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, WRITE_SIZE);
}

void
write_error (void **state)
{
    ssize_t written;

    will_return (__wrap_write, -1);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, -1);
}

void
write_zero (void **state)
{
    ssize_t written;

    will_return (__wrap_write, 0);
    written = write_all (0, NULL, WRITE_SIZE);
    assert_int_equal (written, 0);
}

gint
main (gint    argc,
      gchar  *argv[])
{
    const UnitTest tests[] = {
        unit_test (write_in_one),
        unit_test (write_in_two),
        unit_test (write_in_three),
        unit_test (write_error),
        unit_test (write_zero),
    };
    return run_tests (tests);
}
