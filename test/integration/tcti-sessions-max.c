/*
 * This is a test program intended to create a number of TCTI sessions
 * with the tabrmd. The goal is to hit the upper bound on the number of
 * concurrent sessions and get the TCTI to return an error.
 */
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#include "tcti-tabrmd.h"
#include "common.h"

#define TCTI_COUNT_DEFAULT 30
#define TCTI_COUNT_MAX 100

int
main (int    argc,
      char  *argv[])
{
    TSS2_TCTI_CONTEXT *tcti_context[TCTI_COUNT_MAX] = { NULL };
    TSS2_RC            rc;
    uint8_t            tcti_count = TCTI_COUNT_DEFAULT, i;

    if (argc == 2) {
        tcti_count = strtol (argv[1], NULL, 10);
    }
    if (tcti_count > TCTI_COUNT_MAX) {
        printf ("Cannot create more than %" PRIx8 " connections\n",
                TCTI_COUNT_MAX);
        exit (1);
    }

    for (i = 0; i < tcti_count; ++i) {
        printf ("creating TCTI %" PRIu8 "\n", i);
        rc = tcti_context_init (&tcti_context[i]);
        if (rc != TSS2_RC_SUCCESS) {
            printf ("failed to initialize TCTI connection number %" PRIu8
                    " with tabrmd: 0x%" PRIx32 "\n", i, rc);
            exit (2);
        }
    }
}
