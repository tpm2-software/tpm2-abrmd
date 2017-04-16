#include <stdint.h>
#include <unistd.h>

#include "tcti-tabrmd-priv.h"

int
read_data (
    int       fd,
    size_t   *index,
    uint8_t  *buf,
    size_t    count);
int
read_tpm_buffer (
    int       fd,
    size_t   *index,
    uint8_t  *buf,
    size_t    buf_size);
