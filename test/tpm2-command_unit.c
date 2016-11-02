#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "tpm2-command.h"

static void
tpm2_command_new_unref_test (void **state)
{
    Tpm2Command     *cmd     = NULL;
    guint8          *buffer  = NULL;
    guint32          size    = 0;
    SessionData     *session = NULL;

    cmd = tpm2_command_new (session, size, buffer);
    assert_int_equal (session, cmd->session);
    assert_int_equal (buffer,  cmd->buffer);
    assert_int_equal (size,    cmd->size);

    g_object_unref (cmd);
}

static void
tpm2_command_new_unref_with_buffer_test (void **state)
{
    Tpm2Command     *cmd     = NULL;
    guint8          *buffer  = NULL;
    guint32          size    = 0;
    SessionData     *session = NULL;

    size = 10;
    buffer = calloc (1, size);

    cmd = tpm2_command_new (session, size, buffer);
    assert_int_equal (session, cmd->session);
    assert_int_equal (buffer,  cmd->buffer);
    assert_int_equal (size,    cmd->size);

    g_object_unref (cmd);
}

static void
tpm2_command_new_unref_with_session_test (void **state)
{
    Tpm2Command *cmd     = NULL;
    guint8      *buffer  = NULL;
    guint32      size    = 0;
    gint         fds[2]  = { 0, };
    SessionData *session = NULL;

    session = session_data_new (&fds[0], &fds[1], 0);
    cmd = tpm2_command_new (session, size, buffer);
    assert_int_equal (session, cmd->session);
    assert_int_equal (buffer,  cmd->buffer);
    assert_int_equal (size,    cmd->size);

    g_object_unref (cmd);
}

static void
tpm2_command_type_check_test (void **state)
{
    GObject         *obj     = NULL;
    Tpm2Command     *cmd     = NULL;
    guint8          *buffer  = NULL;
    size_t           size    = 0;
    SessionData     *session = NULL;

    cmd = tpm2_command_new (session, size, buffer);
    assert_true (IS_TPM2_COMMAND (cmd));
    assert_true (G_IS_OBJECT (cmd));
    obj = G_OBJECT (cmd);
    assert_true (G_IS_OBJECT (obj));
    assert_true (IS_TPM2_COMMAND (obj));
    assert_int_equal (cmd, obj);

    g_object_unref (cmd);
}

static void
tpm2_command_get_buffer_test (void **state)
{
    Tpm2Command     *cmd     = NULL;
    guint8          *buffer  = NULL, *buffer_ret = NULL;
    guint32          size    = 10;
    SessionData     *session = NULL;

    buffer = calloc (1, size);

    cmd = tpm2_command_new (session, size, buffer);
    buffer_ret = tpm2_command_get_buffer (cmd);
    assert_int_equal (buffer,  buffer_ret);

    g_object_unref (cmd);
}

static void
tpm2_command_get_tag_test (void **state)
{
    Tpm2Command         *cmd       = NULL;
    guint32              size      = 2;
    SessionData         *session   = NULL;
    guint8              *buffer    = NULL;
    TPMI_ST_COMMAND_TAG  tag_ret;

    buffer = calloc (1, size);
    /* this is tpm_st_sessions in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;

    cmd = tpm2_command_new (session, size, buffer);
    tag_ret = tpm2_command_get_tag (cmd);
    assert_int_equal (tag_ret, TPM_ST_SESSIONS);

    g_object_unref (cmd);
}

static void
tpm2_command_get_size_test (void **state)
{
    Tpm2Command         *cmd       = NULL;
    guint32              size      = 6, size_ret = 0;
    SessionData         *session   = NULL;
    /* this is TPM_ST_SESSIONS + a size of 0x6 in network byte order */
    guint8              *buffer    = NULL;

    buffer = calloc (1, size);
    /* this is tpm_st_sessions in network byte order */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x06;

    cmd = tpm2_command_new (session, size, buffer);
    size_ret = tpm2_command_get_size (cmd);
    assert_int_equal (size_ret, size);

    g_object_unref (cmd);
}

static void
tpm2_command_get_code_test (void **state)
{
    Tpm2Command    *cmd       = NULL;
    guint32         size      = 10;
    SessionData    *session   = NULL;
    guint8         *buffer    = NULL;
    TPM_CC         command_code;

    buffer = calloc (1, size);
    /**
     * This is TPM_ST_SESSIONS + a size of 0x0a + the command code for
     * GetCapability in network byte order
     */
    buffer[0] = 0x80;
    buffer[1] = 0x02;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x06;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x01;
    buffer[9] = 0x7a;

    cmd = tpm2_command_new (session, size, buffer);
    command_code = tpm2_command_get_code (cmd);
    assert_int_equal (command_code, TPM_CC_GetCapability);

    g_object_unref (cmd);

}

gint
main (gint    argc,
      gchar  *argv[])
{
    const UnitTest tests[] = {
        unit_test (tpm2_command_new_unref_test),
        unit_test (tpm2_command_new_unref_with_buffer_test),
        unit_test (tpm2_command_new_unref_with_session_test),
        unit_test (tpm2_command_type_check_test),
        unit_test (tpm2_command_get_buffer_test),
        unit_test (tpm2_command_get_tag_test),
        unit_test (tpm2_command_get_size_test),
        unit_test (tpm2_command_get_code_test),
    };
    return run_tests (tests);
}
