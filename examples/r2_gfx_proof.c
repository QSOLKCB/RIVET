/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/gfx.h"
#include "ppm.h"

#include <stdio.h>

#define PROOF_WIDTH 16ul
#define PROOF_HEIGHT 12ul
#define PROOF_BYTES ((size_t)PROOF_WIDTH * (size_t)PROOF_HEIGHT * RIVET_GFX_PIXEL_BYTES)
#define PROOF_FNV1A64 0x1c0020d75a3b782dULL

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
    unsigned char pixels[PROOF_BYTES];
    static const unsigned char glyph_r[7] = {
        0xf0u, 0x88u, 0x88u, 0xf0u, 0xa0u, 0x90u, 0x88u
    };
    const char *output_path =
        argc > 1 ? argv[1] : "build/rivet-gfx-proof.ppm";
    rivet_surface surface;
    rivet_rect full = {0L, 0L, PROOF_WIDTH, PROOF_HEIGHT};
    rivet_rect clip = {2L, 2L, 12ul, 8ul};
    rivet_rect clipped_fill = {-2L, 1L, 10ul, 5ul};
    rivet_rect copy_source = {2L, 2L, 6ul, 4ul};
    rivet_rgba8 background = {0x10u, 0x18u, 0x20u, 0xffu};
    rivet_rgba8 red = {0xc0u, 0x30u, 0x20u, 0xffu};
    rivet_rgba8 yellow = {0xe0u, 0xe0u, 0x50u, 0xffu};
    unsigned long long hash;

    if (rivet_surface_attach(
            &surface,
            pixels,
            sizeof(pixels),
            PROOF_WIDTH,
            PROOF_HEIGHT,
            (size_t)PROOF_WIDTH * RIVET_GFX_PIXEL_BYTES) != RIVET_OK) {
        return 1;
    }

    if (rivet_surface_fill_rect(&surface, full, background) != RIVET_OK ||
        rivet_surface_set_clip(&surface, clip) != RIVET_OK ||
        rivet_surface_fill_rect(&surface, clipped_fill, red) != RIVET_OK ||
        rivet_surface_reset_clip(&surface) != RIVET_OK ||
        rivet_surface_blit_mono1(
            &surface,
            6L,
            3L,
            glyph_r,
            sizeof(glyph_r),
            1u,
            5ul,
            7ul,
            yellow) != RIVET_OK ||
        rivet_surface_copy_rect(
            &surface,
            copy_source,
            8L,
            7L) != RIVET_OK) {
        return 1;
    }

    hash = fnv1a64(pixels, sizeof(pixels));
    if (hash != PROOF_FNV1A64) {
        fprintf(
            stderr,
            "unexpected pixel hash: %016llx\n",
            hash
        );
        return 1;
    }

    if (!rivet_headless_write_ppm(output_path, &surface)) {
        return 1;
    }

    printf(
        "rivet-r2: ok fnv1a64=%016llx ppm=%s\n",
        hash,
        output_path
    );
    return 0;
}
