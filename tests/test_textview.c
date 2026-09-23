/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/ui.h"
#include "textview.h"

#include <stdio.h>

#ifdef RIVET_EXEC_CHARSET_REGRESSION
#define CHECK(expr) do { if (!(expr)) return 1; } while (0)
#else
#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)
#endif

static const unsigned char doc_bytes[] = {
    0x41,0x4c,0x50,0x48,0x41,0x0a,
    0x42,0x45,0x54,0x41,0x20,0x31,0x32,0x33,0x0a,
    0x50,0x49,0x58,0x45,0x4c,0x53,0x20,0x4e,0x4f,0x54,0x20,0x47,0x50,0x55,0x53,0x0a,
    0x4f,0x4d,0x45,0x47,0x41
};

static const unsigned char query_pixels[] = {
    0x50,0x49,0x58,0x45,0x4c,0x53
};

static int test_open_and_index(void)
{
    rivet_textview viewer;
    size_t offsets[4];

    CHECK(rivet_textview_open(
        &viewer,
        doc_bytes,
        sizeof(doc_bytes),
        offsets,
        4u) == RIVET_OK);
    CHECK(viewer.line_count == 4u);
    CHECK(offsets[0] == 0u);
    CHECK(offsets[1] == 6u);
    CHECK(offsets[2] == 15u);
    CHECK(offsets[3] == 31u);
    CHECK(viewer.top_line == 0u);
    CHECK(viewer.has_match == 0);

    CHECK(rivet_textview_open(
        &viewer,
        doc_bytes,
        sizeof(doc_bytes),
        offsets,
        3u) == RIVET_ERR_CAPACITY);

    {
        static const unsigned char unsupported[] = {0x61u};
        CHECK(rivet_textview_open(
            &viewer,
            unsupported,
            sizeof(unsupported),
            offsets,
            4u) == RIVET_ERR_UNSUPPORTED);
    }

    return 0;
}

static int test_navigation(void)
{
    rivet_textview viewer;
    size_t offsets[4];

    CHECK(rivet_textview_open(
        &viewer,
        doc_bytes,
        sizeof(doc_bytes),
        offsets,
        4u) == RIVET_OK);

    CHECK(rivet_textview_line_up(&viewer) == RIVET_OK);
    CHECK(viewer.top_line == 0u);
    CHECK(rivet_textview_line_down(&viewer) == RIVET_OK);
    CHECK(rivet_textview_line_down(&viewer) == RIVET_OK);
    CHECK(viewer.top_line == 2u);
    CHECK(rivet_textview_page_down(&viewer, 2u) == RIVET_OK);
    CHECK(viewer.top_line == 3u);
    CHECK(rivet_textview_page_up(&viewer, 2u) == RIVET_OK);
    CHECK(viewer.top_line == 1u);
    CHECK(rivet_textview_page_up(&viewer, 2u) == RIVET_OK);
    CHECK(viewer.top_line == 0u);
    return 0;
}

static int test_search(void)
{
    rivet_textview viewer;
    size_t offsets[4];

    CHECK(rivet_textview_open(
        &viewer,
        doc_bytes,
        sizeof(doc_bytes),
        offsets,
        4u) == RIVET_OK);

    CHECK(rivet_textview_find_next(
        &viewer,
        query_pixels,
        sizeof(query_pixels)) == RIVET_OK);
    CHECK(viewer.has_match == 1);
    CHECK(viewer.match_offset == 15u);
    CHECK(viewer.match_length == sizeof(query_pixels));
    CHECK(viewer.top_line == 2u);

    CHECK(rivet_textview_find_next(
        &viewer,
        query_pixels,
        sizeof(query_pixels)) == RIVET_ERR_NOT_FOUND);
    CHECK(viewer.match_offset == 15u);
    CHECK(viewer.has_match == 1);

    {
        static const unsigned char missing[] = {0x5au,0x5au};
        CHECK(rivet_textview_find_next(
            &viewer,
            missing,
            sizeof(missing)) == RIVET_ERR_NOT_FOUND);
        CHECK(viewer.match_offset == 15u);
    }

    return 0;
}


static int test_single_byte_search_wrap(void)
{
    static const unsigned char bytes[] = {0x58u};
    static const unsigned char query[] = {0x58u};
    rivet_textview viewer;
    size_t offsets[1];

    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        1u) == RIVET_OK);
    CHECK(rivet_textview_find_next(
        &viewer,
        query,
        sizeof(query)) == RIVET_OK);
    CHECK(viewer.match_offset == 0u);
    CHECK(rivet_textview_find_next(
        &viewer,
        query,
        sizeof(query)) == RIVET_ERR_NOT_FOUND);
    CHECK(viewer.has_match == 1);
    CHECK(viewer.match_offset == 0u);
    return 0;
}


static int test_search_skips_overlap_inside_current_match(void)
{
    static const unsigned char bytes[] = {0x41u,0x41u,0x41u};
    static const unsigned char query[] = {0x41u,0x41u};
    rivet_textview viewer;
    size_t offsets[1];

    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        1u) == RIVET_OK);
    CHECK(rivet_textview_find_next(
        &viewer,
        query,
        sizeof(query)) == RIVET_OK);
    CHECK(viewer.match_offset == 0u);
    CHECK(viewer.match_length == 2u);
    CHECK(rivet_textview_find_next(
        &viewer,
        query,
        sizeof(query)) == RIVET_ERR_NOT_FOUND);
    CHECK(viewer.has_match == 1);
    CHECK(viewer.match_offset == 0u);
    CHECK(viewer.match_length == 2u);
    return 0;
}


static int test_changed_query_resets_search_boundary(void)
{
    static const unsigned char bytes[] = {0x41u,0x42u};
    static const unsigned char query_b[] = {0x42u};
    static const unsigned char query_ab[] = {0x41u,0x42u};
    rivet_textview viewer;
    size_t offsets[1];

    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        1u) == RIVET_OK);
    CHECK(rivet_textview_find_next(
        &viewer,
        query_b,
        sizeof(query_b)) == RIVET_OK);
    CHECK(viewer.match_offset == 1u);
    CHECK(viewer.match_length == 1u);

    CHECK(rivet_textview_find_next(
        &viewer,
        query_ab,
        sizeof(query_ab)) == RIVET_OK);
    CHECK(viewer.match_offset == 0u);
    CHECK(viewer.match_length == 2u);
    CHECK(viewer.top_line == 0u);
    return 0;
}

static int test_oversized_query_is_miss(void)
{
    static const unsigned char bytes[] = {0x41u};
    static const unsigned char query[] = {0x41u,0x41u};
    rivet_textview viewer;
    size_t offsets[1];

    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        1u) == RIVET_OK);
    CHECK(rivet_textview_find_next(
        &viewer,
        query,
        sizeof(query)) == RIVET_ERR_NOT_FOUND);
    CHECK(viewer.has_match == 0);
    CHECK(viewer.match_offset == 0u);
    CHECK(viewer.match_length == 0u);
    return 0;
}

static int test_render(void)
{
    unsigned char pixels[96u * 32u * 4u];
    rivet_surface surface;
    rivet_textview viewer;
    size_t offsets[4];
    rivet_rect bounds = {0L,0L,96ul,32ul};
    rivet_textview_style style = {
        {0x01u,0x02u,0x03u,0xffu},
        {0x10u,0x20u,0x30u,0xffu},
        {0x40u,0x50u,0x60u,0xffu},
        {0x70u,0x80u,0x90u,0xffu}
    };

    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        96ul,
        32ul,
        384u) == RIVET_OK);
    CHECK(rivet_textview_open(
        &viewer,
        doc_bytes,
        sizeof(doc_bytes),
        offsets,
        4u) == RIVET_OK);
    CHECK(rivet_textview_find_next(
        &viewer,
        query_pixels,
        sizeof(query_pixels)) == RIVET_OK);
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_OK);

    CHECK(pixels[0] == 0x70u);
    CHECK(pixels[(5u * 4u) + 0u] == 0x40u);

    bounds.x = -1L;
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_ERR_INVALID_ARGUMENT);
    return 0;
}



static int test_match_background_stays_inside_viewport(void)
{
    static const unsigned char bytes[] = {0x41u};
    static const unsigned char query[] = {0x41u};
    unsigned char pixels[6u * 8u * 4u];
    rivet_surface surface;
    rivet_textview viewer;
    size_t offsets[1];
    rivet_rect bounds = {0L,0L,5ul,7ul};
    rivet_textview_style style = {
        {0x01u,0x02u,0x03u,0xffu},
        {0x10u,0x20u,0x30u,0xffu},
        {0x40u,0x50u,0x60u,0xffu},
        {0x70u,0x80u,0x90u,0xffu}
    };
    size_t i;
    unsigned long x;
    unsigned long y;

    for (i = 0u; i < sizeof(pixels); ++i) {
        pixels[i] = 0xa5u;
    }

    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        6ul,
        8ul,
        24u) == RIVET_OK);
    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        1u) == RIVET_OK);
    CHECK(rivet_textview_find_next(
        &viewer,
        query,
        sizeof(query)) == RIVET_OK);
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_OK);

    for (y = 0ul; y < 7ul; ++y) {
        const unsigned char *pixel =
            pixels + (size_t)y * 24u + 5u * 4u;
        CHECK(pixel[0] == 0xa5u);
        CHECK(pixel[1] == 0xa5u);
        CHECK(pixel[2] == 0xa5u);
        CHECK(pixel[3] == 0xa5u);
    }

    for (x = 0ul; x < 6ul; ++x) {
        const unsigned char *pixel =
            pixels + 7u * 24u + (size_t)x * 4u;
        CHECK(pixel[0] == 0xa5u);
        CHECK(pixel[1] == 0xa5u);
        CHECK(pixel[2] == 0xa5u);
        CHECK(pixel[3] == 0xa5u);
    }

    return 0;
}


static int test_render_rejects_partial_surface_clip(void)
{
    static const unsigned char bytes[] = {0x41u};
    unsigned char pixels[8u * 8u * 4u];
    rivet_surface surface;
    rivet_textview viewer;
    size_t offsets[1];
    rivet_rect bounds = {0L,0L,8ul,8ul};
    rivet_rect partial_clip = {0L,0L,4ul,8ul};
    rivet_rect empty_clip = {0L,0L,0ul,0ul};
    rivet_textview_style style = {
        {0x01u,0x02u,0x03u,0xffu},
        {0x10u,0x20u,0x30u,0xffu},
        {0x40u,0x50u,0x60u,0xffu},
        {0x70u,0x80u,0x90u,0xffu}
    };

    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        8ul,
        8ul,
        32u) == RIVET_OK);
    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        1u) == RIVET_OK);

    CHECK(rivet_surface_set_clip(
        &surface,
        partial_clip) == RIVET_OK);
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_ERR_INVALID_ARGUMENT);

    CHECK(rivet_surface_set_clip(
        &surface,
        empty_clip) == RIVET_OK);
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_ERR_INVALID_ARGUMENT);

    CHECK(rivet_surface_reset_clip(&surface) == RIVET_OK);
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_OK);
    return 0;
}

static int test_render_counts_every_fitting_glyph_row(void)
{
    static const unsigned char bytes[] = {0x41u,0x0au,0x42u};
    unsigned char pixels[5u * 15u * 4u];
    rivet_surface surface;
    rivet_textview viewer;
    size_t offsets[2];
    rivet_rect bounds = {0L,0L,5ul,15ul};
    rivet_textview_style style = {
        {0x01u,0x02u,0x03u,0xffu},
        {0x10u,0x20u,0x30u,0xffu},
        {0x40u,0x50u,0x60u,0xffu},
        {0x70u,0x80u,0x90u,0xffu}
    };
    const unsigned char *second_row_first_pixel;

    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        5ul,
        15ul,
        20u) == RIVET_OK);
    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        2u) == RIVET_OK);
    CHECK(viewer.line_count == 2u);
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_OK);

    second_row_first_pixel = pixels + 8u * 20u;
    CHECK(second_row_first_pixel[0] == style.foreground.r);
    CHECK(second_row_first_pixel[1] == style.foreground.g);
    CHECK(second_row_first_pixel[2] == style.foreground.b);
    CHECK(second_row_first_pixel[3] == style.foreground.a);
    return 0;
}

static int test_trailing_newline_render(void)
{
    static const unsigned char bytes[] = {0x41u,0x0au};
    unsigned char pixels[16u * 8u * 4u];
    rivet_surface surface;
    rivet_textview viewer;
    size_t offsets[1];
    rivet_rect bounds = {0L,0L,16ul,8ul};
    rivet_textview_style style = {
        {0x01u,0x02u,0x03u,0xffu},
        {0x10u,0x20u,0x30u,0xffu},
        {0x40u,0x50u,0x60u,0xffu},
        {0x70u,0x80u,0x90u,0xffu}
    };

    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        16ul,
        8ul,
        64u) == RIVET_OK);
    CHECK(rivet_textview_open(
        &viewer,
        bytes,
        sizeof(bytes),
        offsets,
        1u) == RIVET_OK);
    CHECK(viewer.line_count == 1u);
    CHECK(rivet_textview_render(
        &surface,
        &viewer,
        bounds,
        style) == RIVET_OK);
    return 0;
}

int main(void)
{
    CHECK(RIVET_TEXTVIEW_CONTRACT_VERSION == 1u);
    CHECK(test_open_and_index() == 0);
    CHECK(test_navigation() == 0);
    CHECK(test_search() == 0);
    CHECK(test_single_byte_search_wrap() == 0);
    CHECK(test_search_skips_overlap_inside_current_match() == 0);
    CHECK(test_changed_query_resets_search_boundary() == 0);
    CHECK(test_oversized_query_is_miss() == 0);
    CHECK(test_render() == 0);
    CHECK(test_match_background_stays_inside_viewport() == 0);
    CHECK(test_render_rejects_partial_surface_clip() == 0);
    CHECK(test_render_counts_every_fitting_glyph_row() == 0);
    CHECK(test_trailing_newline_render() == 0);

#ifndef RIVET_EXEC_CHARSET_REGRESSION
    puts("rivet textview tests: ok");
#endif
    return 0;
}
