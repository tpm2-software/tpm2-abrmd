#include <glib.h>

typedef struct echo_data {
  int fds[2];
  pthread_t thread_id;
  bool running;
} echo_data_t;

/* These two functions should have file scope but we must expose them so the
 * unit testing framework can access them.
 */
void* echo_start (void *arg);
int create_pipe_pairs (int pipe_fds_a[], int pipe_fds_b[], int flags);

/* public */
int CreateConnection (gpointer user_data, int client_fds[]);
