#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "session-data.h"

/* Create a pipe and return the recv and send fds. */
int
create_pipe_pair (int *recv,
                  int *send,
                  int flags)
{
    int ret, fds[2] = { 0, };

    ret = pipe2 (fds, flags);
    if (ret == -1)
        return ret;
    *recv = fds[0];
    *send = fds[1];
    return 0;
}
/* Create a pipe of receive / send pipe pairs. The parameter integer arrays
 * will both be given a receive / send end of two separate pipes. This allows
 * a sender and receiver to communicate over the two pipes. The flags parameter
 * is the same as passed to the pipe2 system call.
 */
int
create_pipe_pairs (int pipe_fds_a[],
                   int pipe_fds_b[],
                   int flags)
{
    int ret = 0;

    /* b -> a */
    ret = create_pipe_pair (&pipe_fds_a[0], &pipe_fds_b[1], flags);
    if (ret == -1)
        return ret;
    /* a -> b */
    ret = create_pipe_pair (&pipe_fds_b[0], &pipe_fds_a[1], flags);
    if (ret == -1)
        return ret;
    return 0;
}

/* CreateConnection builds two pipes for communicating with client
 * applications. It's provided with an array of two integers by the caller
 * and it returns this array populated with the receiving and sending pipe fds
 * respectively.
 */
session_t*
session_new (gint *receive_fd,
             gint *send_fd)
{
    g_info ("CreateConnection");
    int ret, session_fds[2], client_fds[2];
    session_t *session;

    session = calloc (1, sizeof (session_t));
    if (session == NULL)
        g_error ("Failed to allocate memory for session_t: %s", strerror (errno));
    ret = create_pipe_pairs (session_fds, client_fds, O_CLOEXEC);
    if (ret == -1)
        g_error ("CreateConnection failed to make pipe pair %s", strerror (errno));
    session->receive_fd = session_fds[0];
    *receive_fd = client_fds[0];
    session->send_fd = session_fds[1];
    *send_fd = client_fds[1];

    return session;
}

void
session_free (session_t *session)
{
    if (session == NULL)
        return;
    close (session->receive_fd);
    close (session->send_fd);
    free (session);
}

gpointer
session_key (session_t *session)
{
    return &session->receive_fd;
}

gboolean
session_equal (gconstpointer a,
               gconstpointer b)
{
    return g_int_equal (a, b);
}
