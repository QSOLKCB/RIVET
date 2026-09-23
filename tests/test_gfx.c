/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/gfx.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static int pixel_is(
    const rivet_surface *surface,
    unsigned long x,
    unsigned long y,
    rivet_rgba8 color
)
{
    const unsigned char *pixel =
        surface->pixels +
        (size_t)y * surface->stride_bytes +
        (size_t)x * RIVET_GFX_PIXEL_BYTES;

    return pixel[0] == color.r &&
           pixel[1] == color.g &&
           pixel[2] == color.b &&
           pixel[3] == color.a;
}

static int test_attach_and_validation(void)
{
    unsigned char pixels[4u * 3u * 4u];
    rivet_surface surface;

    CHECK(rivet_surface_attach(
        &surface, pixels, sizeof(pixels), 4ul, 3ul, 16u) == RIVET_OK);
    CHECK(rivet_surface_validate(&surface) == RIVET_OK);
    CHECK(surface.clip.x == 0L && surface.clip.y == 0L);
    CHECK(surface.clip.width == 4ul && surface.clip.height == 3ul);

    CHECK(rivet_surface_attach(
        NULL, pixels, sizeof(pixels), 4ul, 3ul, 16u) ==
        RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_surface_attach(
        &surface, NULL, sizeof(pixels), 4ul, 3ul, 16u) ==
        RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_surface_attach(
        &surface, pixels, sizeof(pixels) - 1u, 4ul, 3ul, 16u) ==
        RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_surface_attach(
        &surface, pixels, sizeof(pixels), 4ul, 3ul, 15u) ==
        RIVET_ERR_INVALID_ARGUMENT);

    CHECK(rivet_surface_attach(
        &surface, pixels, sizeof(pixels), 4ul, 3ul, 16u) == RIVET_OK);
    surface.clip.x = -1L;
    CHECK(rivet_surface_validate(&surface) == RIVET_ERR_INVALID_ARGUMENT);
    return 0;
}

static int test_fill_and_clip(void)
{
    unsigned char pixels[4u * 3u * 4u];
    rivet_surface surface;
    rivet_rgba8 black = {0u, 0u, 0u, 255u};
    rivet_rgba8 red = {255u, 0u, 0u, 255u};
    rivet_rect full = {0L, 0L, 4ul, 3ul};
    rivet_rect clip = {1L, 1L, 2ul, 1ul};
    rivet_rect oversized = {-5L, -5L, 20ul, 20ul};
    unsigned long y;
    unsigned long x;

    CHECK(rivet_surface_attach(
        &surface, pixels, sizeof(pixels), 4ul, 3ul, 16u) == RIVET_OK);
    CHECK(rivet_surface_fill_rect(&surface, full, black) == RIVET_OK);
    CHECK(rivet_surface_set_clip(&surface, clip) == RIVET_OK);
    CHECK(rivet_surface_fill_rect(&surface, oversized, red) == RIVET_OK);

    for (y = 0ul; y < 3ul; ++y) {
        for (x = 0ul; x < 4ul; ++x) {
            if (y == 1ul && (x == 1ul || x == 2ul)) {
                CHECK(pixel_is(&surface, x, y, red));
            } else {
                CHECK(pixel_is(&surface, x, y, black));
            }
        }
    }

    CHECK(rivet_surface_reset_clip(&surface) == RIVET_OK);
    CHECK(surface.clip.width == 4ul && surface.clip.height == 3ul);
    return 0;
}

static int test_mono_blit(void)
{
    unsigned char pixels[4u * 3u * 4u];
    const unsigned char bits[2] = {0xa0u, 0x40u};
    rivet_surface surface;
    rivet_rgba8 black = {0u, 0u, 0u, 255u};
    rivet_rgba8 white = {255u, 255u, 255u, 255u};
    rivet_rect full = {0L, 0L, 4ul, 3ul};

    CHECK(rivet_surface_attach(
        &surface, pixels, sizeof(pixels), 4ul, 3ul, 16u) == RIVET_OK);
    CHECK(rivet_surface_fill_rect(&surface, full, black) == RIVET_OK);
    CHECK(rivet_surface_blit_mono1(
        &surface, 0L, 0L, bits, sizeof(bits), 1u, 3ul, 2ul, white) ==
        RIVET_OK);

    CHECK(pixel_is(&surface, 0ul, 0ul, white));
    CHECK(pixel_is(&surface, 1ul, 0ul, black));
    CHECK(pixel_is(&surface, 2ul, 0ul, white));
    CHECK(pixel_is(&surface, 0ul, 1ul, black));
    CHECK(pixel_is(&surface, 1ul, 1ul, white));
    CHECK(pixel_is(&surface, 2ul, 1ul, black));

    CHECK(rivet_surface_blit_mono1(
        &surface, 0L, 0L, bits, 1u, 1u, 3ul, 2ul, white) ==
        RIVET_ERR_INVALID_ARGUMENT);
    return 0;
}

static int test_copy_overlap(void)
{
    unsigned char pixels[5u * 2u * 4u];
    rivet_surface surface;
    rivet_rect one = {0L, 0L, 1ul, 1ul};
    rivet_rect source = {0L, 0L, 4ul, 1ul};
    unsigned long x;

    CHECK(rivet_surface_attach(
        &surface, pixels, sizeof(pixels), 5ul, 2ul, 20u) == RIVET_OK);

    for (x = 0ul; x < 5ul; ++x) {
        rivet_rgba8 color = {
            (unsigned char)(x + 1ul), 0u, 0u, 255u
        };
        one.x = (long)x;
        CHECK(rivet_surface_fill_rect(&surface, one, color) == RIVET_OK);
    }

    CHECK(rivet_surface_copy_rect(&surface, source, 1L, 0L) == RIVET_OK);
    CHECK(pixels[0] == 1u);
    CHECK(pixels[4] == 1u);
    CHECK(pixels[8] == 2u);
    CHECK(pixels[12] == 3u);
    CHECK(pixels[16] == 4u);

    source.x = 0L;
    source.y = 0L;
    source.width = 5ul;
    source.height = 1ul;
    CHECK(rivet_surface_copy_rect(&surface, source, 0L, 1L) == RIVET_OK);
    CHECK(memcmp(pixels, pixels + 20u, 20u) == 0);
    return 0;
}

int main(void)
{
    CHECK(RIVET_GFX_ABI_VERSION == 1u);
    CHECK(RIVET_GFX_PIXEL_BYTES == 4u);
    CHECK(test_attach_and_validation() == 0);
    CHECK(test_fill_and_clip() == 0);
    CHECK(test_mono_blit() == 0);
    CHECK(test_copy_overlap() == 0);

    puts("rivet gfx tests: ok");
    return 0;
}
