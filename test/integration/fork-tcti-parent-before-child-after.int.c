/*
 * Copyright 2018, Intel Corporation
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <errno.h>
#include <glib.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <tss2/tss2_tcti.h>

#include "test-options.h"
#include "context-util.h"
/*
 * This code tests the ability to instantiate instances of the TCTI across
 * a process fork. This test specifically creates a TCTI (connection to
 * tabrmd), then forks and the child creates a TCTI while the parent process
 * waits. The parent must check the exit status as well as whether or not it
 * was killed by a signal (happens on g_error).
 */
int
main (void)
{
    TSS2_TCTI_CONTEXT *tcti_ctx [2];
    test_opts_t opts = TEST_OPTS_DEFAULT_INIT;
    pid_t pid;
    int status = 0;

    /* create TCTI in parent */
    get_test_opts_from_env (&opts);
    if (sanity_check_test_opts (&opts) != 0) {
        return 1;
    }
    tcti_ctx [0] = tcti_init_from_opts (&opts);
    if (tcti_ctx [0] == NULL) {
        g_error ("%s: Failed to create tcti context from opts", __func__);
    }
    pid = fork ();
    if (pid == 0) {
        /* create TCTI in child */
        g_debug ("%s: pid %d -> child", __func__, pid);
        tcti_ctx [1] = tcti_init_from_opts (&opts);
        if (tcti_ctx [1] == NULL) {
            g_error ("%s:%d: Failed to create tcti context from opts", __func__, pid);
        }
    } else {
        /* parent waitpid on child, check exit & signal status */
        g_debug ("%s: pid %d -> parent", __func__, pid);
        if (waitpid (pid, &status, 0) == -1) {
            perror ("waitpid");
            g_error ("%s:%d: waitpid failed: ", __func__, pid);
        } else if (WEXITSTATUS (status)) {
            g_error("%s:%d: child exit status %d", __func__, pid, status);
        } else if (WIFSIGNALED (status)) {
            g_error("%s:%d: child was killed by signal %d", __func__, pid, status);
        }
    }

    g_debug ("%s:%d Test succeeded.", __func__, pid);
    return 0;
}
