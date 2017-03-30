#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"
#include "tpm2-header.h"

#define READ_SIZE  UTIL_BUF_SIZE
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
/*
 * This is a mock function for the read system call. It expects the mock
 * parameter queue to be populated with two things:
 * 1) the value of errno to be set (only set if return value is != count
 *    parameter)
 * 2) the source buffer to be coppied into the provided destination
 * 3) the return value
 */
ssize_t
__wrap_read (int    fd,
             void  *buf,
             size_t count)
{
    int errno_tmp = (int)mock ();
    void *src_buf = (void*)mock ();
    ssize_t ret   = (ssize_t)mock ();

    if (ret == count) {
        memcpy (buf, src_buf, count);
    } else {
        errno = errno_tmp;
    }

    return ret;
}
/*
 * Structure to hold data for the tpm_header_from_fd_* tests.
 */
typedef struct {
    uint8_t header_in [TPM_HEADER_SIZE];
    uint8_t header_out [TPM_HEADER_SIZE];
    uint8_t header_zero [TPM_HEADER_SIZE];
    size_t size;
} header_from_fd_data_t;
/*
 * Setup a header_from_fd_data_t structure for use in the tpm_header_from_fd*
 * tests below. Specifically we populate the header_in array with a valid TPM2
 * header. The size field is initialized to 0xa which is 10 in the size field.
 * The size member is also set to TPM_HEADER_SIZE. All else is initialized to
 * zero.
 */
static void
tpm_header_from_fd_setup (void **state)
{
    header_from_fd_data_t *data = calloc (1, sizeof (header_from_fd_data_t));
    uint8_t header_in [] = { 0x80, 0x02,
                             0x00, 0x00, 0x00, 0x0a,
                             0x00, 0x00, 0x00, 0x00 };

    memcpy (data->header_in, header_in, TPM_HEADER_SIZE);
    data->size = TPM_HEADER_SIZE;

    *state = data;
}
/*
 * Teardown function paired to the above _setup.
 */
static void
tpm_header_from_fd_teardown (void **state)
{
    if (*state != NULL)
        free (*state);
}
/*
 * This test covers the common case for the tpm_header_from_fd function.
 * In this case the attempt to read TPM_HEADER_SIZE bytes from the input fd
 * completes successfully and the header written to the response buffer.
 * The function should return 0 indicating success and the header_in and
 * header_out should be identical.
 */
static void
tpm_header_from_fd_success_test (void **state)
{
    header_from_fd_data_t *data = *state;
    int ret;

    will_return (__wrap_read, 0);
    will_return (__wrap_read, data->header_in);
    will_return (__wrap_read, data->size);

    ret = tpm_header_from_fd (0, data->header_out, data->size);
    assert_int_equal (ret, 0);
    assert_memory_equal (data->header_in, data->header_out, data->size);
}
/*
 * This test covers a typical error condition for the read system call.
 * The errno is set to EAGAIN, and the return value will be -1.
 * In this case the tpm_header_from_fd function should return the errno
 * and the header_out buffer should be unchanged (all 0's).
 */
static void
tpm_header_from_fd_errno_test (void **state)
{
    header_from_fd_data_t *data = *state;
    int ret = 0;
    
    will_return (__wrap_read, EAGAIN);
    will_return (__wrap_read, data->header_in);
    will_return (__wrap_read, -1);

    ret = tpm_header_from_fd (0, data->header_out, data->size);
    assert_int_equal (ret, EAGAIN);
    assert_memory_equal (data->header_out, data->header_zero, data->size);
}
/*
 * This test covers an error case where reading from the fd results in 0
 * bytes being read. This happens when a file reaches EOF or the other end
 * of an IPC mechanism has been closed. In this case the return value should
 * be -1 and the header_out buffer should remain unchanged.
 */
static void
tpm_header_from_fd_eof_test (void **state)
{
    header_from_fd_data_t *data = *state;
    int ret = 0;
    
    will_return (__wrap_read, 0);
    will_return (__wrap_read, data->header_in);
    will_return (__wrap_read, 0);

    ret = tpm_header_from_fd (0, data->header_out, data->size);
    assert_int_equal (ret, -1);
    assert_memory_equal (data->header_out, data->header_zero, data->size);
}
/*
 * This test covers an error case where reading from the fd results in fewer
 * bytes returned than requested (aka a "short read"). In this case the
 * function should return -1.
 */
static void
tpm_header_from_fd_short_test (void **state)
{
    header_from_fd_data_t *data = *state;
    int ret = 0;
    
    will_return (__wrap_read, 0);
    will_return (__wrap_read, data->header_in);
    will_return (__wrap_read, 3);

    ret = tpm_header_from_fd (0, data->header_out, data->size);
    assert_int_equal (ret, -1);
}
/*
 * This test covers an error case where the size of the provided buffer
 * is insufficient to hold a tpm header. In this case -2 is returned and the
 * output buffer should remain unchanged (all 0's).
 */
static void
tpm_header_from_fd_size_lt_header_test (void **state)
{
    header_from_fd_data_t *data = *state;
    int ret = 0;

    ret = tpm_header_from_fd (0, data->header_out, 3);
    assert_int_equal (ret, -2);
    assert_memory_equal (data->header_out, data->header_zero, data->size);
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
        unit_test_setup_teardown (tpm_header_from_fd_success_test,
                                  tpm_header_from_fd_setup,
                                  tpm_header_from_fd_teardown),
        unit_test_setup_teardown (tpm_header_from_fd_errno_test,
                                  tpm_header_from_fd_setup,
                                  tpm_header_from_fd_teardown),
        unit_test_setup_teardown (tpm_header_from_fd_eof_test,
                                  tpm_header_from_fd_setup,
                                  tpm_header_from_fd_teardown),
        unit_test_setup_teardown (tpm_header_from_fd_short_test,
                                  tpm_header_from_fd_setup,
                                  tpm_header_from_fd_teardown),
        unit_test_setup_teardown (tpm_header_from_fd_size_lt_header_test,
                                  tpm_header_from_fd_setup,
                                  tpm_header_from_fd_teardown),
    };
    return run_tests (tests);
}
