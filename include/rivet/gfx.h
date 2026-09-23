/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_GFX_H
#define RIVET_GFX_H

#include <stddef.h>

#include "rivet/rivet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RIVET_GFX_ABI_VERSION 1u
#define RIVET_GFX_PIXEL_BYTES 4u

typedef struct rivet_rgba8 {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} rivet_rgba8;

typedef struct rivet_rect {
    long x;
    long y;
    unsigned long width;
    unsigned long height;
} rivet_rect;

typedef struct rivet_surface {
    unsigned char *pixels;
    size_t buffer_bytes;
    unsigned long width;
    unsigned long height;
    size_t stride_bytes;
    rivet_rect clip;
} rivet_surface;

rivet_result rivet_surface_attach(
    rivet_surface *surface,
    unsigned char *pixels,
    size_t buffer_bytes,
    unsigned long width,
    unsigned long height,
    size_t stride_bytes
);

rivet_result rivet_surface_validate(const rivet_surface *surface);

rivet_result rivet_surface_reset_clip(rivet_surface *surface);

rivet_result rivet_surface_set_clip(
    rivet_surface *surface,
    rivet_rect clip
);

rivet_result rivet_surface_fill_rect(
    rivet_surface *surface,
    rivet_rect rect,
    rivet_rgba8 color
);

rivet_result rivet_surface_blit_mono1(
    rivet_surface *surface,
    long dst_x,
    long dst_y,
    const unsigned char *bits,
    size_t bits_bytes,
    size_t bit_stride_bytes,
    unsigned long width,
    unsigned long height,
    rivet_rgba8 color
);

rivet_result rivet_surface_copy_rect(
    rivet_surface *surface,
    rivet_rect source,
    long dst_x,
    long dst_y
);

#ifdef __cplusplus
}
#endif

#endif
