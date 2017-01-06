#include <glib.h>
#include <inttypes.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tpm2-command.h"

#define HANDLE_FIRST  0x80000000
#define HANDLE_SECOND 0x80000001

typedef struct {
    Tpm2Command *command;
    guint8      *buffer;
    SessionData *session;
} test_data_t;
/**
 * This is the minimum work required to instantiate a Tpm2Command. It needs
 * a data buffer to hold the command and a SessionData object. We also
 * allocate a structure to hold these things so that we can free them in
 * the teardown.
 */
static void
tpm2_command_setup_base (void **state)
{
    test_data_t *data   = NULL;
    gint         fds[2] = { 0, };

    data = calloc (1, sizeof (test_data_t));
    /* allocate a buffer large enough to hold a TPM2 header and 3 handles */
    data->buffer = calloc (1, TPM_RESPONSE_HEADER_SIZE + sizeof (TPM_HANDLE) * 3);
    data->session = session_data_new (&fds[0], &fds[1], 0);
    *state = data;
}
static void
tpm2_command_setup (void **state)
{
    test_data_t *data   = NULL;
    TPMA_CC  attributes = {
        .val = 0x0,
    };

    tpm2_command_setup_base (state);
    data = (test_data_t*)*state;
    data->command = tpm2_command_new (data->session,
                                      data->buffer,
                                      attributes);
    *state = data;
}
static void
tpm2_command_setup_two_handles (void **state)
{
    test_data_t *data = NULL;
    TPMA_CC  attributes = {
       .val = 2 << 25,
    };

    tpm2_command_setup_base (state);
    data = (test_data_t*)*state;
    data->command = tpm2_command_new (data->session,
                                      data->buffer,
                                      attributes);
    /*
     * This sets the two handles to 0x80000000 and 0x80000001, assuming the
     * buffer was initialized to all 0's
     */
    data->buffer [10] = 0x80;
    data->buffer [14] = 0x80;
    data->buffer [17] = 0x01;
}
/**
 * Tear down all of the data from the setup function. We don't have to
 * free the data buffer (data->buffer) since the Tpm2Command frees it as
 * part of its finalize function.
 */
static void
tpm2_command_teardown (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    g_object_unref (data->session);
    g_object_unref (data->command);
    free (data);
}
/**
 * This is a test for memory management / reference counting. The setup
 * function does exactly that so when we get the Tpm2Command object we just
 * check to be sure it's a GObject and then we unref it. This test will
 * probably only fail when run under valgrind if the reference counting is
 * off.
 */
static void
tpm2_command_type_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_true (G_IS_OBJECT (data->command));
    assert_true (IS_TPM2_COMMAND (data->command));
}

static void
tpm2_command_get_session_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (data->session, tpm2_command_get_session (data->command));
}

static void
tpm2_command_get_buffer_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;

    assert_int_equal (data->buffer, tpm2_command_get_buffer (data->command));
}

static void
tpm2_command_get_tag_test (void **state)
{
    test_data_t         *data   = (test_data_t*)*state;
    guint8              *buffer = tpm2_command_get_buffer (data->command);
    TPMI_ST_COMMAND_TAG  tag_ret;

    /* this is tpm_st_sessions in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;

    tag_ret = tpm2_command_get_tag (data->command);
    assert_int_equal (tag_ret, TPM_ST_SESSIONS);
}

static void
tpm2_command_get_size_test (void **state)
{
    test_data_t *data     = (test_data_t*)*state;
    guint8      *buffer   = tpm2_command_get_buffer (data->command);
    guint32      size_ret = 0;

    /* this is tpm_st_sessions in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x06;

    size_ret = tpm2_command_get_size (data->command);
    assert_int_equal (0x6, size_ret);
}

static void
tpm2_command_get_code_test (void **state)
{
    test_data_t *data     = (test_data_t*)*state;
    guint8      *buffer   = tpm2_command_get_buffer (data->command);
    TPM_CC       command_code;

    /**
     * This is TPM_ST_SESSIONS + a size of 0x0a + the command code for
     * GetCapability in network byte order
     */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x0a;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x01;
    buffer[9] = 0x7a;

    command_code = tpm2_command_get_code (data->command);
    assert_int_equal (command_code, TPM_CC_GetCapability);
}

static void
tpm2_command_get_two_handle_count_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    guint8 command_handles;

    command_handles = tpm2_command_get_handle_count (data->command);
    assert_int_equal (command_handles, 2);
}

static void
tpm2_command_get_handles_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    TPM_HANDLE handles [3] = { 0, };
    gboolean ret;

    ret = tpm2_command_get_handles (data->command, handles, 3);
    assert_true (ret == TRUE);
    assert_int_equal (handles [0], HANDLE_FIRST);
    assert_int_equal (handles [1], HANDLE_SECOND);
}
static void
tpm2_command_set_handles_test (void **state)
{
    test_data_t *data = (test_data_t*)*state;
    gboolean ret;
    TPM_HANDLE handles_in [2] = {
        TPM_HT_TRANSIENT + 0x1,
        TPM_HT_TRANSIENT + 0x2,
    };
    TPM_HANDLE handles_out [2] = { 0, };

    ret = tpm2_command_set_handles (data->command, handles_in, 2);
    assert_true (ret == TRUE);
    ret = tpm2_command_get_handles (data->command, handles_out, 2);
    assert_true (ret == TRUE);
    assert_memory_equal (handles_in, handles_out, 2 * sizeof (TPM_HANDLE));
}
gint
main (gint    argc,
      gchar  *argv[])
{
    const UnitTest tests[] = {
        unit_test_setup_teardown (tpm2_command_type_test,
                                  tpm2_command_setup,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_get_session_test,
                                  tpm2_command_setup,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_get_buffer_test,
                                  tpm2_command_setup,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_get_tag_test,
                                  tpm2_command_setup,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_get_size_test,
                                  tpm2_command_setup,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_get_code_test,
                                  tpm2_command_setup,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_get_two_handle_count_test,
                                  tpm2_command_setup_two_handles,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_get_handles_test,
                                  tpm2_command_setup_two_handles,
                                  tpm2_command_teardown),
        unit_test_setup_teardown (tpm2_command_set_handles_test,
                                  tpm2_command_setup_two_handles,
                                  tpm2_command_teardown),
    };
    return run_tests (tests);
}
