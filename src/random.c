/*
 * Copyright (c) 2017 - 2018, Intel Corporation
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
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "util.h"
#include "random.h"

G_DEFINE_TYPE (Random, random, G_TYPE_OBJECT);

/*
 * G_DEFINE_TYPE requires an instance init even though we don't need it.
 */
static void
random_init (Random *obj)
{
    UNUSED_PARAM(obj);
    /* noop */
}
/*
 * Chain up to parent class finalize.
 */
static void
random_finalize (GObject *obj)
{
    g_debug ("random_finalize");
    G_OBJECT_CLASS (random_parent_class)->finalize (obj);
}

static void
random_class_init (RandomClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_debug ("random_class_init");
    if (random_parent_class == NULL)
        random_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = random_finalize;
}

/*
 * Allocate a new Random object. We do no initialization of the seed for
 * the RNG.
 */
Random*
random_new (void)
{
    return RANDOM (g_object_new (TYPE_RANDOM, NULL));
}
/*
 * Seed the underlying RNG from the provided file. The number of bytes
 * read should be sizeof (long int). If we can't get this
 * much entropy we return -1. Otherwise 0 on success.
 */
int
random_seed_from_file (Random *random,
                       const char *fname)
{
    gint rand_fd, ret = 0;
    long int rand_seed = 0;
    ssize_t read_ret;

    if (random == NULL) {
        g_error ("NULL random pointer passed to random_seed_from_file");
        return -1;
    }

    g_debug ("opening entropy source: %s", fname);
    g_assert_nonnull (random);
    rand_fd = open (fname, O_RDONLY);
    if (rand_fd == -1) {
        g_warning ("failed to open entropy source %s: %s",
                   fname,
                   strerror (errno));
        ret = -1;
        goto out;
    }
    g_debug ("reading from entropy source: %s", fname);
    read_ret = read (rand_fd, &rand_seed, sizeof (rand_seed));
    if (read_ret == -1) {
        g_warning ("failed to read from entropy source %s, %s",
                   fname,
                   strerror (errno));
        ret = -1;
        goto close_out;
    } else if (read_ret < (ssize_t) sizeof (rand_seed)) {
        g_warning ("short read on entropy source %s: got %zu bytes, expecting %zu",
                   fname, read_ret, sizeof (rand_seed));
        ret = -1;
        goto close_out;
    }
    g_debug ("seeding rand with %ld", rand_seed);
    srand48_r (rand_seed, &random->rand_state);

close_out:
    if (close (rand_fd) != 0)
        g_warning ("failed to close entropy source %s",
                   strerror (errno));
out:
    return ret;
}
/*
 * Get 'count' bytes of random data out of the provided Random object.
 * On success the number of bytes obtained is returned. On error 0 is
 * returned.
 * NOTE: This algorithm is pretty inefficient. For each byte we return
 *   to the caller we get a long int of data from rand48. The rest of
 *   the sizeof (long int) bytes are wasted.
 */
size_t
random_get_bytes (Random    *random,
                  uint8_t    dest[],
                  size_t     count)
{
    size_t i;
    uint8_t rand[sizeof (long int)] = { 0, };

    g_debug ("random_get_bytes: %p", random);
    g_assert_nonnull (random);
    for (i = 0; i < count; ++i) {
        lrand48_r (&random->rand_state, (long int*)&rand[0]);
        memcpy (&dest[i], &rand[0], sizeof (uint8_t));
    }
    return i;
}
/*
 * Get 64 bits of random data from the parameter 'random' object.
 * On error, return 0 instead of random data.
 */
uint64_t
random_get_uint64 (Random      *random)
{
    size_t ret;
    uint64_t dest;

    if (random == NULL) {
        g_error ("NULL random pointer passed to random_get_uint64");
        return -1;
    }


    ret = random_get_bytes (random, (uint8_t*)&dest, sizeof (uint64_t));
    g_assert_true (ret == sizeof (uint64_t));

    return dest;
}
/*
 * Get 32 bits of random data from the parameter 'random' object.
 * On error, return 0 instead of random data.
 */
uint32_t
random_get_uint32 (Random       *random)
{
    size_t ret;
    uint32_t dest;

    if (random == NULL) {
        g_error ("NULL random pointer passed to random_get_uint32");
        return -1;
    }


    ret = random_get_bytes (random, (uint8_t*)&dest, sizeof (uint32_t));
    g_assert_true (ret == sizeof (uint32_t));

    return dest;
}
/*
 * Get random 32bit number in the given range.
 */
uint32_t
random_get_uint32_range (Random *random,
                         uint32_t high,
                         uint32_t low)
{
    size_t ret;
    uint32_t dest;

    ret = random_get_bytes (random, (uint8_t*)&dest, sizeof (dest));
    if (ret != sizeof (uint32_t))
        return -1;

    return low + (dest / (UINT32_MAX / (high - low)));
}
