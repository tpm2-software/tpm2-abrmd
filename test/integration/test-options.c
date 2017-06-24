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
#include <stdio.h>

#include "test-options.h"
#include "tcti-tabrmd.h"

/*
 * A structure to map a string name to an element in the TCTI_TYPE
 * enumeration.
 */
typedef struct {
    char       *name;
    TCTI_TYPE   type;
} tcti_map_entry_t;
/*
 * A table of tcti_map_entry_t structures. This is how we map a string
 * provided on the command line to the enumeration.
 */
tcti_map_entry_t tcti_map_table[] = {
#ifdef HAVE_TCTI_DEVICE
    {
        .name = "device",
        .type = DEVICE_TCTI,
    },
#endif
#ifdef HAVE_TCTI_SOCKET
    {
        .name = "socket",
        .type = SOCKET_TCTI,
    },
#endif
    {
        .name = "tabrmd",
        .type = TABRMD_TCTI,
    },
    {
        .name = "unknown",
        .type = UNKNOWN_TCTI,
    },
};
/*
 * Convert from a string to an element in the TCTI_TYPE enumeration.
 * An unkonwn name / string will map to UNKNOWN_TCTI.
 */
TCTI_TYPE
tcti_type_from_name (char const *tcti_str)
{
    int i;
    for (i = 0; i < N_TCTI - 1; ++i)
        if (strcmp (tcti_str, tcti_map_table[i].name) == 0)
            return tcti_map_table[i].type;
    return UNKNOWN_TCTI;
}
/*
 * Convert from an element in the TCTI_TYPE enumeration to a string
 * representation.
 */
char* const
tcti_name_from_type (TCTI_TYPE tcti_type)
{
    int i;
    for (i = 0; i < N_TCTI - 1; ++i)
        if (tcti_type == tcti_map_table[i].type)
            return tcti_map_table[i].name;
    return NULL;
}
TCTI_TABRMD_DBUS_TYPE
bus_type_from_str (const char *bus_type_str)
{
    if (strcmp (bus_type_str, "system") == 0) {
        return TCTI_TABRMD_DBUS_TYPE_SYSTEM;
    } else if (strcmp (bus_type_str, "session") == 0) {
        return TCTI_TABRMD_DBUS_TYPE_SESSION;
    } else {
        g_error ("Invalid bus type for %s", bus_type_str);
        return TCTI_TABRMD_DBUS_TYPE_DEFAULT;
    }
}
const char*
bus_str_from_type (TCTI_TABRMD_DBUS_TYPE bus_type)
{
    switch (bus_type) {
    case TCTI_TABRMD_DBUS_TYPE_SESSION:
        return "session";
    case TCTI_TABRMD_DBUS_TYPE_SYSTEM:
        return "system";
    default:
        return NULL;
    }
}
/*
 * return 0 if sanity test passes
 * return 1 if sanity test fails
 */
int
sanity_check_test_opts (test_opts_t  *opts)
{
    switch (opts->tcti_type) {
#ifdef HAVE_TCTI_DEVICE
    case DEVICE_TCTI:
        if (opts->device_file == NULL) {
            fprintf (stderr, "device-path is NULL, check env\n");
            return 1;
        }
        break;
#endif
#ifdef HAVE_TCTI_SOCKET
    case SOCKET_TCTI:
        if (opts->socket_address == NULL || opts->socket_port == 0) {
            fprintf (stderr,
                     "socket_address or socket_port is NULL, check env\n");
            return 1;
        }
        break;
#endif
    case TABRMD_TCTI:
        switch (opts->tabrmd_bus_type) {
        case TCTI_TABRMD_DBUS_TYPE_SYSTEM:
        case TCTI_TABRMD_DBUS_TYPE_SESSION:
            break;
        default:
            fprintf (stderr,
                     "tabrmd_bus_type is 0, check env\n");
            return 1;
        }
        if (opts->tabrmd_bus_name == NULL) {
            fprintf (stderr,
                     "tabrmd_bus_name is NULL, check env\n");
            return 1;
        }
        break;
    default:
        fprintf (stderr, "unknown TCTI type, check env\n");
        return 1;
    }
    return 0;
}

/*
 * Parse command line options from argv extracting test options. These are
 * returned to the caller in the provided options structure.
 */
int
get_test_opts_from_env (test_opts_t          *test_opts)
{
    char *env_str, *end_ptr;

    if (test_opts == NULL)
        return 1;
    env_str = getenv (ENV_TCTI_NAME);
    if (env_str != NULL)
        test_opts->tcti_type = tcti_type_from_name (env_str);
    env_str = getenv (ENV_DEVICE_FILE);
    if (env_str != NULL)
        test_opts->device_file = env_str;
    env_str = getenv (ENV_SOCKET_ADDRESS);
    if (env_str != NULL)
        test_opts->socket_address = env_str;
    env_str = getenv (ENV_SOCKET_PORT);
    if (env_str != NULL)
        test_opts->socket_port = strtol (env_str, &end_ptr, 10);
    env_str = getenv (ENV_TABRMD_BUS_TYPE);
    if (env_str != NULL) {
        test_opts->tabrmd_bus_type = bus_type_from_str (env_str);
    }
    env_str = getenv (ENV_TABRMD_BUS_NAME);
    if (env_str != NULL) {
        test_opts->tabrmd_bus_name = env_str;
    }
    return 0;
}
/*
 * Dump the contents of the test_opts_t structure to stdout.
 */
void
dump_test_opts (test_opts_t *opts)
{
    printf ("test_opts_t:\n");
    printf ("  tcti_type:      %s\n", tcti_name_from_type (opts->tcti_type));
    printf ("  device_file:    %s\n", opts->device_file);
    printf ("  socket_address: %s\n", opts->socket_address);
    printf ("  socket_port:    %d\n", opts->socket_port);
    printf ("  tabrmd_bus_type: %s\n", bus_str_from_type (opts->tabrmd_bus_type));
    printf ("  tabrmd_bus_name: %s\n", opts->tabrmd_bus_name);
}
