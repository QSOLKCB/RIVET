/* SPDX-License-Identifier: MPL-2.0 */

#include "ppm.h"

#include <stdio.h>

int rivet_headless_write_ppm(
    const char *path,
    const rivet_surface *surface
)
{
    FILE *file;
    unsigned long y;
    unsigned long x;

    if (path == NULL || path[0] == '\0' ||
        rivet_surface_validate(surface) != RIVET_OK) {
        return 0;
    }

    file = fopen(path, "wb");
    if (file == NULL) {
        return 0;
    }

    if (fprintf(
            file,
            "P6\n%lu %lu\n255\n",
            surface->width,
            surface->height) < 0) {
        fclose(file);
        return 0;
    }

    for (y = 0ul; y < surface->height; ++y) {
        const unsigned char *pixel =
            surface->pixels + (size_t)y * surface->stride_bytes;

        for (x = 0ul; x < surface->width; ++x) {
            if (fwrite(pixel, 1u, 3u, file) != 3u) {
                fclose(file);
                return 0;
            }
            pixel += RIVET_GFX_PIXEL_BYTES;
        }
    }

    if (fclose(file) != 0) {
        return 0;
    }

    return 1;
}
