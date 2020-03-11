/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>
#include <stdio.h>

#include <setjmp.h>
#include <cmocka.h>

#include <tss2/tss2_tcti.h>

#include "tabrmd-init.h"
#include "tabrmd-options.h"
#include "tcti-mock.h"
#include "tcti.h"
#include "util.h"

#define ID_IPCFRONT (IpcFrontend*)0x10293847
#define ID_GMAIN (GMainLoop*)0x64738291
#define ID_RANDOM (Random*)0x65132784

/*
 * This flag is set to TRUE by the mock function for g_main_loop_quit.
 * This is a hack to simplify state / mem management when we want to
 * test whether or not the gmain loop is killed by code under test.
 */
static gboolean gmain_quit = FALSE;
static int
test_setup (void **state)
{
    UNUSED_PARAM (state);

    gmain_quit = FALSE;

    return 0;
}

void
__wrap_g_main_loop_quit (GMainLoop *loop)
{
    UNUSED_PARAM (loop);
    gmain_quit = mock_type (gboolean);
}

gboolean
__wrap_g_main_loop_is_running (GMainLoop *loop)
{
    UNUSED_PARAM (loop);
    return mock_type (gboolean);
}

static void
on_ipc_frontend_disconnect_test (void **state)
{
    UNUSED_PARAM (state);
    IpcFrontend *ipc_frontend = ID_IPCFRONT;
    gmain_data_t data = { .loop = ID_GMAIN, };

    will_return (__wrap_g_main_loop_is_running, TRUE);
    will_return (__wrap_g_main_loop_quit, TRUE);
    on_ipc_frontend_disconnect (ipc_frontend, &data);
    assert_true (gmain_quit);
}

static gmain_data_t data;

guint
__wrap_g_unix_signal_add (gint signum,
                          GSourceFunc handler,
                          gpointer user_data)
{
    UNUSED_PARAM (signum);
    UNUSED_PARAM (handler);
    UNUSED_PARAM (user_data);

    return mock_type (guint);
}
static int
init_thread_func_setup (void **state)
{
    UNUSED_PARAM (state);
    gmain_data_t data_src = { .options = TABRMD_OPTIONS_INIT_DEFAULT, };

    data_src.options.flush_all = TRUE;
    memcpy (&data, &data_src, sizeof (data));
    g_mutex_init (&data.init_mutex);

    return 0;
}

static void
init_thread_func_signal_add_fail (void **state)
{
    UNUSED_PARAM (state);

    will_return (__wrap_g_unix_signal_add, 0);
    assert_int_equal (init_thread_func (&data), EX_OSERR);
}

int
__wrap_random_seed_from_file (Random *random,
                              const char *fname)
{
    UNUSED_PARAM (random);
    UNUSED_PARAM (fname);

    return mock_type (int);
}

int
__wrap_random_get_bytes (Random *random,
                         uint8_t dest[],
                         size_t count)
{
    UNUSED_PARAM (random);
    UNUSED_PARAM (count);

    uint8_t *src = mock_type (uint8_t*);
    size_t size = mock_type (size_t);
    assert_true (size <= count);

    memcpy (dest, &src, size);

    return size;
}

static void
init_thread_func_random_fail (void **state)
{
    UNUSED_PARAM (state);

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 1);
    assert_int_equal (init_thread_func (&data), EX_OSERR);
}

/* ipc_frontend_connect */
void
__wrap_ipc_frontend_connect (IpcFrontend  *self,
                             GMutex *mutex)
{
    UNUSED_PARAM (self);
    UNUSED_PARAM (mutex);

    return;
}

TSS2_RC
__wrap_Tss2_TctiLdr_Initialize (const char* conf,
                                TSS2_TCTI_CONTEXT **tcti_ctx)
{
    UNUSED_PARAM (conf);

    *tcti_ctx = mock_type (TSS2_TCTI_CONTEXT*);
    return mock_type (TSS2_RC);
}


static void
init_thread_func_tcti_factory_fail (void **state)
{
    UNUSED_PARAM (state);

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 0);
    will_return (__wrap_Tss2_TctiLdr_Initialize, NULL);
    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_TCTI_RC_IO_ERROR);
    assert_int_equal (init_thread_func (&data), EX_IOERR);
}

TSS2_RC
__wrap_tpm2_init_tpm (Tpm2 *tpm2)
{
    UNUSED_PARAM (tpm2);
    printf ("ftw\n");
    return mock_type (TSS2_RC);
}

static void
init_thread_func_tpm2_init_fail (void **state)
{
    UNUSED_PARAM (state);
    TSS2_TCTI_CONTEXT *tcti_ctx = tcti_mock_init_full ();

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 0);
    will_return (__wrap_Tss2_TctiLdr_Initialize, tcti_ctx);
    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_init_tpm, EX_UNAVAILABLE);
    assert_int_equal (init_thread_func (&data), EX_UNAVAILABLE);
}

gint
__wrap_command_attrs_init_tpm (CommandAttrs *attrs,
                               Tpm2 *tpm2)
{
    UNUSED_PARAM (attrs);
    UNUSED_PARAM (tpm2);
    return mock_type (gint);
}
void
__wrap_tpm2_flush_all_context (Tpm2 *tpm2)
{
    UNUSED_PARAM (tpm2);
    return;
}

static void
init_thread_func_cmdattrs_fail (void **state)
{
    UNUSED_PARAM (state);
    TSS2_TCTI_CONTEXT *tcti_ctx = tcti_mock_init_full ();

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 0);
    will_return (__wrap_Tss2_TctiLdr_Initialize, tcti_ctx);
    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_init_tpm, TSS2_RC_SUCCESS);
    will_return (__wrap_command_attrs_init_tpm, EX_UNAVAILABLE);
    assert_int_equal (init_thread_func (&data), EX_UNAVAILABLE);
}

gint
__wrap_thread_start (Thread *thread)
{
    UNUSED_PARAM (thread);
    return mock_type (gint);
}

static void
init_thread_func_cmdsrc_fail (void **state)
{
    UNUSED_PARAM (state);
    TSS2_TCTI_CONTEXT *tcti_ctx = tcti_mock_init_full ();

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 0);
    will_return (__wrap_Tss2_TctiLdr_Initialize, tcti_ctx);
    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_init_tpm, TSS2_RC_SUCCESS);
    will_return (__wrap_command_attrs_init_tpm, 0);
    will_return (__wrap_thread_start, 1);
    will_return (__wrap_g_main_loop_is_running, FALSE);
    assert_int_equal (init_thread_func (&data), EX_OSERR);
}
static void
init_thread_func_resmgr_fail (void **state)
{
    UNUSED_PARAM (state);
    TSS2_TCTI_CONTEXT *tcti_ctx = tcti_mock_init_full ();

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 0);
    will_return (__wrap_Tss2_TctiLdr_Initialize, tcti_ctx);
    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_init_tpm, TSS2_RC_SUCCESS);
    will_return (__wrap_command_attrs_init_tpm, 0);
    will_return (__wrap_thread_start, 0);
    will_return (__wrap_thread_start, 1);
    will_return (__wrap_g_main_loop_is_running, FALSE);
    assert_int_equal (init_thread_func (&data), EX_OSERR);
}
static void
init_thread_func_respsnk_fail (void **state)
{
    UNUSED_PARAM (state);
    TSS2_TCTI_CONTEXT *tcti_ctx = tcti_mock_init_full ();

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 0);
    will_return (__wrap_Tss2_TctiLdr_Initialize, tcti_ctx);
    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_init_tpm, TSS2_RC_SUCCESS);
    will_return (__wrap_command_attrs_init_tpm, 0);
    will_return (__wrap_thread_start, 0);
    will_return (__wrap_thread_start, 0);
    will_return (__wrap_thread_start, 1);
    will_return (__wrap_g_main_loop_is_running, FALSE);
    assert_int_equal (init_thread_func (&data), EX_OSERR);
}
static void
init_thread_func_success (void **state)
{
    UNUSED_PARAM (state);
    TSS2_TCTI_CONTEXT *tcti_ctx = tcti_mock_init_full ();

    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_g_unix_signal_add, 1);
    will_return (__wrap_random_seed_from_file, 0);
    will_return (__wrap_Tss2_TctiLdr_Initialize, tcti_ctx);
    will_return (__wrap_Tss2_TctiLdr_Initialize, TSS2_RC_SUCCESS);
    will_return (__wrap_tpm2_init_tpm, TSS2_RC_SUCCESS);
    will_return (__wrap_command_attrs_init_tpm, 0);
    will_return (__wrap_thread_start, 0);
    will_return (__wrap_thread_start, 0);
    will_return (__wrap_thread_start, 0);
    assert_int_equal (init_thread_func (&data), 0);
}

int
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup (on_ipc_frontend_disconnect_test,
                                test_setup),
        cmocka_unit_test_setup (init_thread_func_signal_add_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_random_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_tcti_factory_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_tpm2_init_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_cmdattrs_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_cmdsrc_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_resmgr_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_respsnk_fail,
                                init_thread_func_setup),
        cmocka_unit_test_setup (init_thread_func_success,
                                init_thread_func_setup),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
