/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/gfx.h"

#include <limits.h>
#include <string.h>

static int rivet_surface_shape_valid(
    unsigned long width,
    unsigned long height,
    size_t stride_bytes,
    size_t buffer_bytes
)
{
    size_t size_max = (size_t)-1;
    size_t row_bytes;

    if (width == 0ul || height == 0ul ||
        width > (unsigned long)LONG_MAX ||
        height > (unsigned long)LONG_MAX) {
        return 0;
    }

    if (width > (unsigned long)size_max ||
        (size_t)width > size_max / RIVET_GFX_PIXEL_BYTES) {
        return 0;
    }
    row_bytes = (size_t)width * RIVET_GFX_PIXEL_BYTES;

    if (stride_bytes < row_bytes || stride_bytes == 0u) {
        return 0;
    }
    if (height > (unsigned long)size_max ||
        (size_t)height > size_max / stride_bytes) {
        return 0;
    }
    if (stride_bytes * (size_t)height > buffer_bytes) {
        return 0;
    }

    return 1;
}

static void rivet_rect_empty(rivet_rect *rect)
{
    rect->x = 0;
    rect->y = 0;
    rect->width = 0ul;
    rect->height = 0ul;
}

static unsigned long rivet_negative_distance(long value)
{
    return (unsigned long)(-(value + 1L)) + 1ul;
}

static int rivet_rect_clip_to_bounds(
    rivet_rect rect,
    unsigned long bound_width,
    unsigned long bound_height,
    rivet_rect *out
)
{
    unsigned long x;
    unsigned long y;
    unsigned long width = rect.width;
    unsigned long height = rect.height;
    unsigned long skip;

    if (out == NULL) {
        return 0;
    }
    if (width == 0ul || height == 0ul) {
        rivet_rect_empty(out);
        return 1;
    }

    if (rect.x < 0L) {
        skip = rivet_negative_distance(rect.x);
        if (skip >= width) {
            rivet_rect_empty(out);
            return 1;
        }
        width -= skip;
        x = 0ul;
    } else {
        x = (unsigned long)rect.x;
        if (x >= bound_width) {
            rivet_rect_empty(out);
            return 1;
        }
    }

    if (rect.y < 0L) {
        skip = rivet_negative_distance(rect.y);
        if (skip >= height) {
            rivet_rect_empty(out);
            return 1;
        }
        height -= skip;
        y = 0ul;
    } else {
        y = (unsigned long)rect.y;
        if (y >= bound_height) {
            rivet_rect_empty(out);
            return 1;
        }
    }

    if (width > bound_width - x) {
        width = bound_width - x;
    }
    if (height > bound_height - y) {
        height = bound_height - y;
    }

    out->x = (long)x;
    out->y = (long)y;
    out->width = width;
    out->height = height;
    return 1;
}

static void rivet_rect_intersect(
    rivet_rect left,
    rivet_rect right,
    rivet_rect *out
)
{
    unsigned long lx = (unsigned long)left.x;
    unsigned long ly = (unsigned long)left.y;
    unsigned long rx = (unsigned long)right.x;
    unsigned long ry = (unsigned long)right.y;
    unsigned long x0 = lx > rx ? lx : rx;
    unsigned long y0 = ly > ry ? ly : ry;
    unsigned long left_x1 = lx + left.width;
    unsigned long left_y1 = ly + left.height;
    unsigned long right_x1 = rx + right.width;
    unsigned long right_y1 = ry + right.height;
    unsigned long x1 = left_x1 < right_x1 ? left_x1 : right_x1;
    unsigned long y1 = left_y1 < right_y1 ? left_y1 : right_y1;

    if (x1 <= x0 || y1 <= y0) {
        rivet_rect_empty(out);
        return;
    }

    out->x = (long)x0;
    out->y = (long)y0;
    out->width = x1 - x0;
    out->height = y1 - y0;
}

static rivet_result rivet_surface_clip_rect(
    const rivet_surface *surface,
    rivet_rect requested,
    rivet_rect *out
)
{
    rivet_rect bounded;

    if (rivet_surface_validate(surface) != RIVET_OK || out == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!rivet_rect_clip_to_bounds(
            requested, surface->width, surface->height, &bounded)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (bounded.width == 0ul || bounded.height == 0ul ||
        surface->clip.width == 0ul || surface->clip.height == 0ul) {
        rivet_rect_empty(out);
        return RIVET_OK;
    }

    rivet_rect_intersect(bounded, surface->clip, out);
    return RIVET_OK;
}

static int rivet_axis_source_offset(
    long origin,
    unsigned long clipped_coordinate,
    unsigned long *out
)
{
    unsigned long base;
    unsigned long skip;

    if (out == NULL) {
        return 0;
    }

    if (origin < 0L) {
        skip = rivet_negative_distance(origin);
        if (clipped_coordinate > ULONG_MAX - skip) {
            return 0;
        }
        *out = skip + clipped_coordinate;
        return 1;
    }

    base = (unsigned long)origin;
    if (clipped_coordinate < base) {
        return 0;
    }
    *out = clipped_coordinate - base;
    return 1;
}

static unsigned char *rivet_surface_pixel(
    rivet_surface *surface,
    unsigned long x,
    unsigned long y
)
{
    return surface->pixels +
           (size_t)y * surface->stride_bytes +
           (size_t)x * RIVET_GFX_PIXEL_BYTES;
}

rivet_result rivet_surface_attach(
    rivet_surface *surface,
    unsigned char *pixels,
    size_t buffer_bytes,
    unsigned long width,
    unsigned long height,
    size_t stride_bytes
)
{
    if (surface == NULL || pixels == NULL ||
        !rivet_surface_shape_valid(
            width, height, stride_bytes, buffer_bytes)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    surface->pixels = pixels;
    surface->buffer_bytes = buffer_bytes;
    surface->width = width;
    surface->height = height;
    surface->stride_bytes = stride_bytes;
    surface->clip.x = 0L;
    surface->clip.y = 0L;
    surface->clip.width = width;
    surface->clip.height = height;
    return RIVET_OK;
}

rivet_result rivet_surface_validate(const rivet_surface *surface)
{
    unsigned long clip_x;
    unsigned long clip_y;

    if (surface == NULL || surface->pixels == NULL ||
        !rivet_surface_shape_valid(
            surface->width,
            surface->height,
            surface->stride_bytes,
            surface->buffer_bytes)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (surface->clip.x < 0L || surface->clip.y < 0L) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    clip_x = (unsigned long)surface->clip.x;
    clip_y = (unsigned long)surface->clip.y;

    if (clip_x > surface->width || clip_y > surface->height) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (surface->clip.width > surface->width - clip_x ||
        surface->clip.height > surface->height - clip_y) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    return RIVET_OK;
}

rivet_result rivet_surface_reset_clip(rivet_surface *surface)
{
    if (rivet_surface_validate(surface) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    surface->clip.x = 0L;
    surface->clip.y = 0L;
    surface->clip.width = surface->width;
    surface->clip.height = surface->height;
    return RIVET_OK;
}

rivet_result rivet_surface_set_clip(
    rivet_surface *surface,
    rivet_rect clip
)
{
    rivet_rect bounded;

    if (rivet_surface_validate(surface) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!rivet_rect_clip_to_bounds(
            clip, surface->width, surface->height, &bounded)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    surface->clip = bounded;
    return RIVET_OK;
}

rivet_result rivet_surface_fill_rect(
    rivet_surface *surface,
    rivet_rect rect,
    rivet_rgba8 color
)
{
    rivet_rect clipped;
    unsigned long y;
    unsigned long x;

    if (rivet_surface_clip_rect(surface, rect, &clipped) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (y = 0ul; y < clipped.height; ++y) {
        unsigned char *pixel = rivet_surface_pixel(
            surface,
            (unsigned long)clipped.x,
            (unsigned long)clipped.y + y
        );
        for (x = 0ul; x < clipped.width; ++x) {
            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
            pixel[3] = color.a;
            pixel += RIVET_GFX_PIXEL_BYTES;
        }
    }

    return RIVET_OK;
}

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
)
{
    rivet_rect requested;
    rivet_rect clipped;
    unsigned long source_x;
    unsigned long source_y;
    unsigned long y;
    unsigned long x;
    unsigned long minimum_stride;
    size_t size_max = (size_t)-1;

    if (rivet_surface_validate(surface) != RIVET_OK ||
        bits == NULL || width == 0ul || height == 0ul ||
        width > (unsigned long)LONG_MAX ||
        height > (unsigned long)LONG_MAX) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    minimum_stride = width / 8ul + (width % 8ul != 0ul ? 1ul : 0ul);
    if (minimum_stride > (unsigned long)size_max ||
        bit_stride_bytes == 0u ||
        bit_stride_bytes < (size_t)minimum_stride ||
        height > (unsigned long)size_max ||
        (size_t)height > size_max / bit_stride_bytes ||
        bit_stride_bytes * (size_t)height > bits_bytes) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    requested.x = dst_x;
    requested.y = dst_y;
    requested.width = width;
    requested.height = height;

    if (rivet_surface_clip_rect(surface, requested, &clipped) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (clipped.width == 0ul || clipped.height == 0ul) {
        return RIVET_OK;
    }

    if (!rivet_axis_source_offset(
            dst_x, (unsigned long)clipped.x, &source_x) ||
        !rivet_axis_source_offset(
            dst_y, (unsigned long)clipped.y, &source_y) ||
        source_x > width ||
        source_y > height ||
        clipped.width > width - source_x ||
        clipped.height > height - source_y) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (y = 0ul; y < clipped.height; ++y) {
        const unsigned char *source_row =
            bits + (size_t)(source_y + y) * bit_stride_bytes;
        unsigned char *pixel = rivet_surface_pixel(
            surface,
            (unsigned long)clipped.x,
            (unsigned long)clipped.y + y
        );

        for (x = 0ul; x < clipped.width; ++x) {
            unsigned long bit_x = source_x + x;
            unsigned char mask =
                (unsigned char)(0x80u >> (unsigned int)(bit_x % 8ul));

            if ((source_row[bit_x / 8ul] & mask) != 0u) {
                pixel[0] = color.r;
                pixel[1] = color.g;
                pixel[2] = color.b;
                pixel[3] = color.a;
            }
            pixel += RIVET_GFX_PIXEL_BYTES;
        }
    }

    return RIVET_OK;
}

rivet_result rivet_surface_copy_rect(
    rivet_surface *surface,
    rivet_rect source,
    long dst_x,
    long dst_y
)
{
    rivet_rect clipped_source;
    rivet_rect requested_destination;
    rivet_rect destination;
    unsigned long crop_x;
    unsigned long crop_y;
    unsigned long source_x;
    unsigned long source_y;
    size_t row_bytes;
    unsigned long row;

    if (rivet_surface_validate(surface) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!rivet_rect_clip_to_bounds(
            source,
            surface->width,
            surface->height,
            &clipped_source)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (clipped_source.width == 0ul || clipped_source.height == 0ul) {
        return RIVET_OK;
    }

    requested_destination.x = dst_x;
    requested_destination.y = dst_y;
    requested_destination.width = clipped_source.width;
    requested_destination.height = clipped_source.height;

    if (rivet_surface_clip_rect(
            surface, requested_destination, &destination) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (destination.width == 0ul || destination.height == 0ul) {
        return RIVET_OK;
    }

    if (!rivet_axis_source_offset(
            dst_x, (unsigned long)destination.x, &crop_x) ||
        !rivet_axis_source_offset(
            dst_y, (unsigned long)destination.y, &crop_y) ||
        crop_x > clipped_source.width ||
        crop_y > clipped_source.height ||
        destination.width > clipped_source.width - crop_x ||
        destination.height > clipped_source.height - crop_y) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    source_x = (unsigned long)clipped_source.x + crop_x;
    source_y = (unsigned long)clipped_source.y + crop_y;
    row_bytes = (size_t)destination.width * RIVET_GFX_PIXEL_BYTES;

    if ((unsigned long)destination.y > source_y &&
        (unsigned long)destination.y < source_y + destination.height) {
        for (row = destination.height; row != 0ul; --row) {
            unsigned long offset = row - 1ul;
            memmove(
                rivet_surface_pixel(
                    surface,
                    (unsigned long)destination.x,
                    (unsigned long)destination.y + offset
                ),
                rivet_surface_pixel(
                    surface,
                    source_x,
                    source_y + offset
                ),
                row_bytes
            );
        }
    } else {
        for (row = 0ul; row < destination.height; ++row) {
            memmove(
                rivet_surface_pixel(
                    surface,
                    (unsigned long)destination.x,
                    (unsigned long)destination.y + row
                ),
                rivet_surface_pixel(
                    surface,
                    source_x,
                    source_y + row
                ),
                row_bytes
            );
        }
    }

    return RIVET_OK;
}
