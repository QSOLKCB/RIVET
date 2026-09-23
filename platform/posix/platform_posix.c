/* SPDX-License-Identifier: MPL-2.0 */

#define _POSIX_C_SOURCE 200809L

#include "rivet/platform.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

static const char *const posix_capabilities[] = {
    "filesystem.read",
    "timer.monotonic"
};

static rivet_result posix_read_file(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    int fd;
    size_t used = 0u;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT || errno == ENOTDIR) {
            return RIVET_ERR_NOT_FOUND;
        }
        return RIVET_ERR_UNSUPPORTED;
    }

    while (used < capacity) {
        size_t request = capacity - used;
        ssize_t received;

        if (request > (size_t)SSIZE_MAX) {
            request = (size_t)SSIZE_MAX;
        }

        received = read(
            fd,
            buffer + used,
            request
        );

        if (received > 0) {
            used += (size_t)received;
            continue;
        }
        if (received == 0) {
            if (close(fd) != 0) {
                return RIVET_ERR_UNSUPPORTED;
            }
            *byte_count = used;
            return RIVET_OK;
        }
        if (errno == EINTR) {
            continue;
        }

        close(fd);
        return RIVET_ERR_UNSUPPORTED;
    }

    for (;;) {
        unsigned char extra;
        ssize_t received = read(fd, &extra, 1u);

        if (received > 0) {
            close(fd);
            return RIVET_ERR_CAPACITY;
        }
        if (received == 0) {
            if (close(fd) != 0) {
                return RIVET_ERR_UNSUPPORTED;
            }
            *byte_count = used;
            return RIVET_OK;
        }
        if (errno != EINTR) {
            close(fd);
            return RIVET_ERR_UNSUPPORTED;
        }
    }
}

static rivet_result posix_monotonic_ns(
    unsigned long long *nanoseconds
)
{
    struct timespec now;
    unsigned long long seconds;
    unsigned long long base;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 ||
        now.tv_sec < 0 ||
        now.tv_nsec < 0 ||
        now.tv_nsec >= 1000000000L) {
        return RIVET_ERR_UNSUPPORTED;
    }

    seconds = (unsigned long long)now.tv_sec;
    if (seconds > ULLONG_MAX / 1000000000ULL) {
        return RIVET_ERR_CAPACITY;
    }

    base = seconds * 1000000000ULL;
    if (ULLONG_MAX - base <
        (unsigned long long)now.tv_nsec) {
        return RIVET_ERR_CAPACITY;
    }

    *nanoseconds =
        base + (unsigned long long)now.tv_nsec;
    return RIVET_OK;
}

static int posix_little_endian(void)
{
    unsigned int value = 1u;
    return *((const unsigned char *)&value) == 1u;
}

const rivet_platform *rivet_platform_current(void)
{
    static const rivet_platform_info little_info = {
        "posix-v1",
        "POSIX",
        (unsigned int)(sizeof(void *) * CHAR_BIT),
        1,
        {posix_capabilities, 2u}
    };
    static const rivet_platform_info big_info = {
        "posix-v1",
        "POSIX",
        (unsigned int)(sizeof(void *) * CHAR_BIT),
        0,
        {posix_capabilities, 2u}
    };
    static const rivet_platform little = {
        &little_info,
        posix_read_file,
        posix_monotonic_ns
    };
    static const rivet_platform big = {
        &big_info,
        posix_read_file,
        posix_monotonic_ns
    };

    return posix_little_endian() ? &little : &big;
}
