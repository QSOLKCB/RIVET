/* SPDX-License-Identifier: MPL-2.0 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "rivet/platform.h"

#include <limits.h>

static const char *const win32_capabilities[] = {
    "filesystem.read",
    "timer.monotonic"
};

static rivet_result win32_read_file(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    HANDLE file;
    size_t used = 0u;

    file = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (file == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();

        if (error == ERROR_FILE_NOT_FOUND ||
            error == ERROR_PATH_NOT_FOUND) {
            return RIVET_ERR_NOT_FOUND;
        }
        return RIVET_ERR_UNSUPPORTED;
    }

    while (used < capacity) {
        size_t remaining = capacity - used;
        DWORD chunk = remaining > (size_t)MAXDWORD ?
            MAXDWORD :
            (DWORD)remaining;
        DWORD received = 0u;

        if (!ReadFile(
                file,
                buffer + used,
                chunk,
                &received,
                NULL)) {
            CloseHandle(file);
            return RIVET_ERR_UNSUPPORTED;
        }

        if (received == 0u) {
            if (!CloseHandle(file)) {
                return RIVET_ERR_UNSUPPORTED;
            }
            *byte_count = used;
            return RIVET_OK;
        }

        used += (size_t)received;
    }

    {
        unsigned char extra;
        DWORD received = 0u;

        if (!ReadFile(
                file,
                &extra,
                1u,
                &received,
                NULL)) {
            CloseHandle(file);
            return RIVET_ERR_UNSUPPORTED;
        }

        if (received != 0u) {
            CloseHandle(file);
            return RIVET_ERR_CAPACITY;
        }
    }

    if (!CloseHandle(file)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    *byte_count = used;
    return RIVET_OK;
}

static rivet_result win32_monotonic_ns(
    unsigned long long *nanoseconds
)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    unsigned long long count;
    unsigned long long freq;
    unsigned long long seconds;
    unsigned long long remainder;
    unsigned long long base;
    unsigned long long fraction;

    if (!QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&counter) ||
        frequency.QuadPart <= 0 ||
        counter.QuadPart < 0) {
        return RIVET_ERR_UNSUPPORTED;
    }

    count = (unsigned long long)counter.QuadPart;
    freq = (unsigned long long)frequency.QuadPart;
    seconds = count / freq;
    remainder = count % freq;

    if (seconds > ULLONG_MAX / 1000000000ULL ||
        remainder > ULLONG_MAX / 1000000000ULL) {
        return RIVET_ERR_CAPACITY;
    }

    base = seconds * 1000000000ULL;
    fraction = (remainder * 1000000000ULL) / freq;

    if (ULLONG_MAX - base < fraction) {
        return RIVET_ERR_CAPACITY;
    }

    *nanoseconds = base + fraction;
    return RIVET_OK;
}

static int win32_little_endian(void)
{
    unsigned int value = 1u;
    return *((const unsigned char *)&value) == 1u;
}

const rivet_platform *rivet_platform_current(void)
{
    static const rivet_platform_info little_info = {
        "win32-v1",
        "Win32",
        (unsigned int)(sizeof(void *) * CHAR_BIT),
        1,
        {win32_capabilities, 2u}
    };
    static const rivet_platform_info big_info = {
        "win32-v1",
        "Win32",
        (unsigned int)(sizeof(void *) * CHAR_BIT),
        0,
        {win32_capabilities, 2u}
    };
    static const rivet_platform little = {
        &little_info,
        win32_read_file,
        win32_monotonic_ns
    };
    static const rivet_platform big = {
        &big_info,
        win32_read_file,
        win32_monotonic_ns
    };

    return win32_little_endian() ? &little : &big;
}
