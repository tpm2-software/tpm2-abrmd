#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <glib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "connection-manager.h"

static echo_data_t echo_data = {
  .fds = { 0, 0 },
  .thread_id = 0,
  .running = false,
};

static void
echo_cleanup (void *arg)
{
  echo_data_t *data = (echo_data_t*)arg;
  close (data->fds[0]);
  close (data->fds[1]);
  data->running = false;
}

/* Create a pipe and return the recv and send fds. */
int
create_pipe_pair (int *recv, int *send, int flags)
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
create_pipe_pairs (int pipe_fds_a[], int pipe_fds_b[], int flags)
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

void*
echo_start (void *arg)
{
  echo_data_t *data = (echo_data_t*)arg;
  ssize_t ret = 0;
  uint8_t *buf = NULL;

  pthread_cleanup_push (echo_cleanup, data);

  buf = calloc (1, 4096);
  if (buf == NULL)
    g_error ("echo_start failed to calloc data buffer: %s", strerror (errno));
  ret = read (data->fds [0], buf, 4096);
  if (ret == -1) {
    g_warning ("read from fd: %d failed: %s", strerror (errno));
    goto echo_start_out;
  }
  ret = write (data->fds [1], buf, ret);
  if (ret == -1)
    g_warning ("write to fd: %d failed: %s", strerror (errno));
echo_start_out:
  if (buf)
    free (buf);
  pthread_cleanup_pop (1);
}

/* CreateConnection builds two pipes for communicating with client
 * applications. It's provided with an array of two integers by the caller
 * and it returns this array populated with the receiving and sending pipe fds
 * respectively.
 */
int
CreateConnection (gpointer user_data, int client_fds[])
{
  g_info ("CreateConnection");
  int ret, pipe_fds[2];

  /* Create pipe pairs, one to pass to the echo thread, one to return to the
   * client application.
   */
  ret = create_pipe_pairs (echo_data.fds, client_fds, O_CLOEXEC);
  if (ret == -1)
    g_error ("CreateConnection failed to make pipe pair %s", strerror (errno));

  /* Create thread to do something with the pipe / peer */
  echo_data.running = true;
  pthread_create (&echo_data.thread_id, NULL, &echo_start, &echo_data);

  return 0;
}
