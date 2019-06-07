/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 */
#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "util.h"
#include "handle-map.h"
#include "ipc-frontend.h"
/*
 * Begin definition of TestIpcFrontend.
 * This is a GObject that derives from the IpcFrontend abstract base class.
 */
G_BEGIN_DECLS
typedef struct _TestIpcFrontend      TestIpcFrontend;
typedef struct _TestIpcFrontendClass TestIpcFrontendClass;

struct _TestIpcFrontend {
    IpcFrontend             parent;
    gboolean               connected;
};

struct _TestIpcFrontendClass {
    IpcFrontendClass        parent;
};

#define TYPE_TEST_IPC_FRONTEND             (test_ipc_frontend_get_type       ())
#define TEST_IPC_FRONTEND(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),   TYPE_TEST_IPC_FRONTEND, TestIpcFrontend))
#define IS_TEST_IPC_FRONTEND(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   TYPE_TEST_IPC_FRONTEND))

GType test_ipc_frontend_get_type (void);

G_END_DECLS

G_DEFINE_TYPE (TestIpcFrontend, test_ipc_frontend, TYPE_IPC_FRONTEND);
static void
test_ipc_frontend_init (TestIpcFrontend *self)
{
    self->connected = FALSE;
}
static void
test_ipc_frontend_finalize (GObject *obj)
{
    if (test_ipc_frontend_parent_class != NULL)
        G_OBJECT_CLASS (test_ipc_frontend_parent_class)->finalize (obj);
}
static void
test_ipc_frontend_connect (TestIpcFrontend *self,
                           GMutex          *init_mutex)
{
    IpcFrontend *frontend = IPC_FRONTEND (self);

    self->connected = TRUE;
    frontend->init_mutex = init_mutex;
}
static void
test_ipc_frontend_disconnect (TestIpcFrontend *self)
{
    IpcFrontend *frontend = IPC_FRONTEND (self);

    self->connected = FALSE;
    frontend->init_mutex = NULL;
    /*
     * This is where a child class would emit the 'disconnected' signal.
     * This test class however doesn't need to do this since it's tested
     * elsewhere.
     */
}
static void
test_ipc_frontend_class_init (TestIpcFrontendClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    IpcFrontendClass *ipc_frontend_class = IPC_FRONTEND_CLASS (klass);

    if (test_ipc_frontend_parent_class == NULL)
        test_ipc_frontend_parent_class = g_type_class_peek_parent (klass);
    object_class->finalize   = test_ipc_frontend_finalize;
    ipc_frontend_class->connect    = (IpcFrontendConnect)test_ipc_frontend_connect;
    ipc_frontend_class->disconnect = (IpcFrontendDisconnect)test_ipc_frontend_disconnect;
}

static TestIpcFrontend*
test_ipc_frontend_new (void)
{
    return TEST_IPC_FRONTEND (g_object_new (TYPE_TEST_IPC_FRONTEND, NULL));
}
/*
 * End definition of TestIpcFrontend GObject.
 */
/*
 * This is a 'setup' function used by the cmocka machinery to setup the
 * test state before a test is executed.
 */
static int
ipc_frontend_setup (void **state)
{
    TestIpcFrontend *test_ipc_frontend = NULL;

    test_ipc_frontend = test_ipc_frontend_new ();
    assert_non_null (test_ipc_frontend);
    *state = test_ipc_frontend;
    return 0;
}
/*
 * This 'teardown' function is used by the cmocka machinery to cleanup the
 * test state.
 */
static int
ipc_frontend_teardown (void **state)
{
    TestIpcFrontend *test_ipc_frontend = NULL;

    assert_non_null (state);
    test_ipc_frontend = TEST_IPC_FRONTEND (*state);
    g_object_unref (test_ipc_frontend);
    return 0;
}
/*
 * This test relies on the setup / teardown functions to instantiate an
 * instance of the TestIpcFrontend. It extracts this object from the state
 * parameter and uses the GObject type macros to ensure that the object
 * system identifies it as both the abstract base type and the derived
 * type.
 */
static void
ipc_frontend_type_test (void **state)
{
    assert_non_null (state);
    assert_true (IS_IPC_FRONTEND (*state));
    assert_true (IS_TEST_IPC_FRONTEND (*state));
}
/*
 * This callback function is used by the ipc_frontend_event_test to set a
 * boolean flag when the signal is emitted.
 */
static void
ipc_frontend_on_disconnected (IpcFrontend *ipc_frontend,
                              bool        *called_flag)
{
    UNUSED_PARAM(ipc_frontend);

    *called_flag = true;
}
/*
 * This test exercises the 'disconnected' event emitted by the IpcFrontend
 * abstract class.
 */
static void
ipc_frontend_event_test (void **state)
{
    IpcFrontend  *ipc_frontend = NULL;
    bool          called_flag = false;

    ipc_frontend      = IPC_FRONTEND (*state);
    g_signal_connect (ipc_frontend,
                      "disconnected",
                      (GCallback) ipc_frontend_on_disconnected,
                      &called_flag);
    /* pickup here and test the signal emission */
    ipc_frontend_disconnected_invoke (ipc_frontend);
    assert_true (called_flag);
}
/*
 */
static void
ipc_frontend_connect_test (void **state)
{
    IpcFrontend *ipc_frontend = NULL;
    TestIpcFrontend *test_ipc_frontend = NULL;

    ipc_frontend = IPC_FRONTEND (*state);
    test_ipc_frontend = TEST_IPC_FRONTEND (*state);

    ipc_frontend_connect (ipc_frontend, NULL);
    assert_true (test_ipc_frontend->connected);
}
/*
 */
static void
ipc_frontend_disconnect_test (void **state)
{
    IpcFrontend *ipc_frontend = NULL;
    TestIpcFrontend *test_ipc_frontend = NULL;

    ipc_frontend = IPC_FRONTEND (*state);
    test_ipc_frontend = TEST_IPC_FRONTEND (*state);

    ipc_frontend_connect (ipc_frontend, NULL);
    assert_true (test_ipc_frontend->connected);
    ipc_frontend_disconnect (ipc_frontend);
    assert_false (test_ipc_frontend->connected);
}

gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (ipc_frontend_type_test,
                                         ipc_frontend_setup,
                                         ipc_frontend_teardown),
        cmocka_unit_test_setup_teardown (ipc_frontend_event_test,
                                         ipc_frontend_setup,
                                         ipc_frontend_teardown),
        cmocka_unit_test_setup_teardown (ipc_frontend_connect_test,
                                         ipc_frontend_setup,
                                         ipc_frontend_teardown),
        cmocka_unit_test_setup_teardown (ipc_frontend_disconnect_test,
                                         ipc_frontend_setup,
                                         ipc_frontend_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
