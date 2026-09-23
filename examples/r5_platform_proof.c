/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/platform.h"

#include <stdio.h>

#define FIXTURE_BYTES 237u
#define FIXTURE_FNV1A64 0x36aaff7f4aaa99abULL

static unsigned long long fnv1a64(
    const unsigned char *bytes,
    size_t count
)
{
    unsigned long long hash = 0xcbf29ce484222325ULL;
    size_t i;

    for (i = 0u; i < count; ++i) {
        hash ^= (unsigned long long)bytes[i];
        hash *= 0x100000001b3ULL;
    }

    return hash;
}

int main(int argc, char **argv)
{
    const rivet_platform *platform;
    unsigned char bytes[512];
    size_t byte_count = 0u;
    unsigned long long first = 0u;
    unsigned long long second = 0u;
    unsigned long long hash;
    unsigned char empty_buffer = 0u;
    size_t empty_count = 99u;
    const char *path =
        argc > 1 ? argv[1] : "fixtures/r4_textview.txt";
    const char *empty_path =
        argc > 2 ? argv[2] : "fixtures/r5_empty.txt";

    platform = rivet_platform_current();
    if (rivet_platform_validate(platform) != RIVET_OK) {
        return 1;
    }

    if (rivet_platform_read_file(
            platform,
            path,
            bytes,
            sizeof(bytes),
            &byte_count) != RIVET_OK ||
        byte_count != FIXTURE_BYTES) {
        return 1;
    }

    hash = fnv1a64(bytes, byte_count);
    if (hash != FIXTURE_FNV1A64) {
        return 1;
    }

    if (rivet_platform_read_file(
            platform,
            empty_path,
            &empty_buffer,
            0u,
            &empty_count) != RIVET_OK ||
        empty_count != 0u) {
        return 1;
    }

    if (rivet_platform_monotonic_ns(
            platform,
            &first) != RIVET_OK ||
        rivet_platform_monotonic_ns(
            platform,
            &second) != RIVET_OK ||
        second < first) {
        return 1;
    }

    printf(
        "rivet-r5: backend=%s os=%s pointer_bits=%u endian=%s bytes=%lu empty=%lu fnv1a64=%016llx monotonic=nondecreasing\n",
        platform->info->backend_id,
        platform->info->os_api,
        platform->info->pointer_bits,
        platform->info->little_endian ? "little" : "big",
        (unsigned long)byte_count,
        (unsigned long)empty_count,
        hash
    );
    return 0;
}
