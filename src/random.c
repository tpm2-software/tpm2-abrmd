/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#include <assert.h>
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
    random->rand_state[0] = 0x330E;
    random->rand_state[1] = rand_seed & 0xffff;
    random->rand_state[2] = (rand_seed >> 16) & 0xffff;

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

    g_assert_nonnull (random);
    assert (random->rand_state);
    for (i = 0; i < count; ++i) {
        *(&rand[0]) = nrand48 (random->rand_state);
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
