/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/platform.h"

#include <limits.h>

static int platform_cstr_valid(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static rivet_result platform_require_capability(
    const rivet_platform_info *info,
    const char *id
)
{
    int has = 0;
    rivet_result result;

    result = rivet_capability_has(
        &info->capabilities,
        id,
        &has
    );
    if (result != RIVET_OK) {
        return result;
    }

    return has ? RIVET_OK : RIVET_ERR_UNSUPPORTED;
}

static int platform_path_valid(const char *path)
{
    size_t i = 0u;

    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    while (path[i] != '\0') {
        unsigned char byte = (unsigned char)path[i];

        if (i == RIVET_PLATFORM_PATH_MAX ||
            byte < 0x20u || byte > 0x7eu) {
            return 0;
        }
        ++i;
    }

    return 1;
}

rivet_result rivet_platform_validate(
    const rivet_platform *platform
)
{
    rivet_result result;

    if (platform == NULL ||
        platform->info == NULL ||
        platform->read_file == NULL ||
        platform->monotonic_ns == NULL ||
        !platform_cstr_valid(platform->info->backend_id) ||
        !platform_cstr_valid(platform->info->os_api) ||
        platform->info->pointer_bits !=
            (unsigned int)(sizeof(void *) * CHAR_BIT) ||
        (platform->info->little_endian != 0 &&
         platform->info->little_endian != 1)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (platform->info->capabilities.count != 2u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    result = platform_require_capability(
        platform->info,
        "filesystem.read"
    );
    if (result != RIVET_OK) {
        return result;
    }

    return platform_require_capability(
        platform->info,
        "timer.monotonic"
    );
}

rivet_result rivet_platform_read_file(
    const rivet_platform *platform,
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    size_t observed = 0u;
    rivet_result result;

    result = rivet_platform_validate(platform);
    if (result != RIVET_OK) {
        return result;
    }
    if (!platform_path_valid(path) ||
        buffer == NULL ||
        byte_count == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    result = platform->read_file(
        path,
        buffer,
        capacity,
        &observed
    );
    if (result != RIVET_OK) {
        return result;
    }
    if (observed > capacity) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    *byte_count = observed;
    return RIVET_OK;
}

rivet_result rivet_platform_monotonic_ns(
    const rivet_platform *platform,
    unsigned long long *nanoseconds
)
{
    unsigned long long observed = 0u;
    rivet_result result;

    result = rivet_platform_validate(platform);
    if (result != RIVET_OK) {
        return result;
    }
    if (nanoseconds == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    result = platform->monotonic_ns(&observed);
    if (result != RIVET_OK) {
        return result;
    }

    *nanoseconds = observed;
    return RIVET_OK;
}
