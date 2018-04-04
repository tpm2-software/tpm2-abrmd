/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <glib.h>
#include <stdlib.h>

#include <setjmp.h>
#include <cmocka.h>

#include "ipc-frontend-dbus.h"

static int
ipc_frontend_dbus_setup (void **state)
{
    IpcFrontendDbus   *ipc_frontend_dbus = NULL;
    ConnectionManager *connection_manager = NULL;
    Random            *random = NULL;
    gint ret = 0;

    random = random_new ();
    ret = random_seed_from_file (random, "/dev/urandom");
    assert_int_equal (ret, 0);

    connection_manager = connection_manager_new (100);

    ipc_frontend_dbus = ipc_frontend_dbus_new (IPC_FRONTEND_DBUS_TYPE_DEFAULT,
                                               IPC_FRONTEND_DBUS_NAME_DEFAULT,
                                               connection_manager,
                                               100,
                                               random);
    assert_non_null (ipc_frontend_dbus);
    *state = ipc_frontend_dbus;
    g_object_unref (random);
    g_object_unref (connection_manager);
    return 0;
}

static int
ipc_frontend_dbus_teardown (void **state)
{
    IpcFrontendDbus *ipc_frontend_dbus = NULL;

    assert_non_null (state);
    ipc_frontend_dbus = IPC_FRONTEND_DBUS (*state);
    g_object_unref (ipc_frontend_dbus);
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
ipc_frontend_dbus_type_test (void **state)
{
    assert_non_null (state);
    assert_true (IS_IPC_FRONTEND (*state));
    assert_true (IS_IPC_FRONTEND_DBUS (*state));
}
gint
main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown (ipc_frontend_dbus_type_test,
                                         ipc_frontend_dbus_setup,
                                         ipc_frontend_dbus_teardown),
    };
    return cmocka_run_group_tests (tests, NULL, NULL);
}
