/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/platform.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static const char *const good_caps[] = {
    "filesystem.read",
    "timer.monotonic"
};

static const char *const missing_caps[] = {
    "filesystem.read"
};

static const char *const extra_caps[] = {
    "filesystem.read",
    "timer.monotonic",
    "filesystem.write"
};

static rivet_result fake_read(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    static const unsigned char payload[] = {0x41u,0x42u,0x43u};

    (void)path;

    if (capacity < sizeof(payload)) {
        return RIVET_ERR_CAPACITY;
    }

    memcpy(buffer, payload, sizeof(payload));
    *byte_count = sizeof(payload);
    return RIVET_OK;
}

static rivet_result fake_empty_read(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    (void)path;
    (void)buffer;

    if (capacity != 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    *byte_count = 0u;
    return RIVET_OK;
}

static rivet_result fake_time(
    unsigned long long *nanoseconds
)
{
    *nanoseconds = 123456789ULL;
    return RIVET_OK;
}

static rivet_result fake_bad_read(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    (void)path;
    (void)buffer;
    *byte_count = capacity + 1u;
    return RIVET_OK;
}

#ifdef _WIN32
unsigned long long rivet_win32_test_mul_div_floor(
    unsigned long long multiplicand,
    unsigned long long multiplier,
    unsigned long long divisor
);
#endif

static int host_little_endian(void)
{
    unsigned int value = 1u;
    return *((const unsigned char *)&value) == 1u;
}

static int test_validate_and_dispatch(void)
{
    rivet_platform_info info = {
        "fake-v1",
        "FAKE",
        (unsigned int)(sizeof(void *) * CHAR_BIT),
        0,
        {good_caps, 2u}
    };
    rivet_platform platform = {
        &info,
        fake_read,
        fake_time
    };
    unsigned char bytes[4] = {0u,0u,0u,0u};
    size_t count = 99u;
    unsigned long long time = 0u;

    info.little_endian = host_little_endian();

    CHECK(rivet_platform_validate(&platform) == RIVET_OK);
    CHECK(rivet_platform_read_file(
        &platform,
        "ABC",
        bytes,
        sizeof(bytes),
        &count) == RIVET_OK);
    CHECK(count == 3u);
    CHECK(bytes[0] == 0x41u &&
          bytes[1] == 0x42u &&
          bytes[2] == 0x43u);

    CHECK(rivet_platform_monotonic_ns(
        &platform,
        &time) == RIVET_OK);
    CHECK(time == 123456789ULL);

    info.capabilities.ids = missing_caps;
    info.capabilities.count = 1u;
    CHECK(rivet_platform_validate(
        &platform) == RIVET_ERR_UNSUPPORTED);

    info.capabilities.ids = extra_caps;
    info.capabilities.count = 3u;
    CHECK(rivet_platform_validate(
        &platform) == RIVET_ERR_INVALID_ARGUMENT);

    info.capabilities.ids = good_caps;
    info.capabilities.count = 2u;
    info.pointer_bits = 1u;
    CHECK(rivet_platform_validate(
        &platform) == RIVET_ERR_INVALID_ARGUMENT);
    return 0;
}

static int test_validation_preserves_outputs(void)
{
    rivet_platform_info info = {
        "fake-v1",
        "FAKE",
        (unsigned int)(sizeof(void *) * CHAR_BIT),
        0,
        {good_caps, 2u}
    };
    rivet_platform platform = {
        &info,
        fake_read,
        fake_time
    };
    unsigned char bytes[4] = {0u,0u,0u,0u};
    size_t count = 77u;
    unsigned long long time = 88u;
    const char invalid_path[] = {'A','\n','B','\0'};

    info.little_endian = host_little_endian();

    CHECK(rivet_platform_read_file(
        &platform,
        invalid_path,
        bytes,
        sizeof(bytes),
        &count) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(count == 77u);

    CHECK(rivet_platform_read_file(
        &platform,
        "",
        bytes,
        sizeof(bytes),
        &count) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(count == 77u);

    CHECK(rivet_platform_monotonic_ns(
        &platform,
        NULL) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(time == 88u);

    platform.read_file = fake_empty_read;
    CHECK(rivet_platform_read_file(
        &platform,
        "EMPTY",
        bytes,
        0u,
        &count) == RIVET_OK);
    CHECK(count == 0u);

    count = 77u;
    platform.read_file = fake_bad_read;
    CHECK(rivet_platform_read_file(
        &platform,
        "ABC",
        bytes,
        sizeof(bytes),
        &count) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(count == 77u);
    return 0;
}

#ifdef _WIN32
static int test_win32_mul_div_conversion(void)
{
    CHECK(rivet_win32_test_mul_div_floor(
        19999999999ULL,
        1000000000ULL,
        20000000000ULL) == 999999999ULL);
    CHECK(rivet_win32_test_mul_div_floor(
        1ULL,
        1000000000ULL,
        3ULL) == 333333333ULL);
    return 0;
}
#endif

int main(void)
{
    CHECK(RIVET_PLATFORM_ABI_VERSION == 1u);
    CHECK(RIVET_PLATFORM_PATH_MAX == 255u);
    CHECK(test_validate_and_dispatch() == 0);
    CHECK(test_validation_preserves_outputs() == 0);
#ifdef _WIN32
    CHECK(test_win32_mul_div_conversion() == 0);
#endif

    puts("rivet platform tests: ok");
    return 0;
}
