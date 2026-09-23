/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_PLATFORM_H
#define RIVET_PLATFORM_H

#include <stddef.h>

#include "rivet/rivet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RIVET_PLATFORM_ABI_VERSION 1u
#define RIVET_PLATFORM_PATH_MAX 255u

typedef struct rivet_platform_info {
    const char *backend_id;
    const char *os_api;
    unsigned int pointer_bits;
    int little_endian;
    rivet_capability_set capabilities;
} rivet_platform_info;

typedef struct rivet_platform {
    const rivet_platform_info *info;
    rivet_result (*read_file)(
        const char *path,
        unsigned char *buffer,
        size_t capacity,
        size_t *byte_count
    );
    rivet_result (*monotonic_ns)(
        unsigned long long *nanoseconds
    );
} rivet_platform;

rivet_result rivet_platform_validate(
    const rivet_platform *platform
);

rivet_result rivet_platform_read_file(
    const rivet_platform *platform,
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
);

rivet_result rivet_platform_monotonic_ns(
    const rivet_platform *platform,
    unsigned long long *nanoseconds
);

const rivet_platform *rivet_platform_current(void);

#ifdef __cplusplus
}
#endif

#endif
