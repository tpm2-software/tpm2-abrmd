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

gint
main (gint    argc,
      gchar  *argv[])
{
    const UnitTest tests[] = {
        unit_test (tpm2_command_new_unref_test),
        unit_test (tpm2_command_new_unref_with_buffer_test),
        unit_test (tpm2_command_new_unref_with_session_test),
        unit_test (tpm2_command_type_check_test),
    };
    return run_tests (tests);
}
