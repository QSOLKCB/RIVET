/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_TEXTVIEW_H
#define RIVET_TEXTVIEW_H

#include <stddef.h>

#include "rivet/gfx.h"
#include "rivet/rivet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RIVET_TEXTVIEW_CONTRACT_VERSION 1u

typedef struct rivet_textview_style {
    rivet_rgba8 background;
    rivet_rgba8 foreground;
    rivet_rgba8 match_background;
    rivet_rgba8 match_foreground;
} rivet_textview_style;

typedef struct rivet_textview {
    const unsigned char *bytes;
    size_t byte_count;
    size_t *line_offsets;
    size_t line_capacity;
    size_t line_count;
    size_t top_line;
    size_t match_offset;
    size_t match_length;
    int has_match;
} rivet_textview;

rivet_result rivet_textview_open(
    rivet_textview *viewer,
    const unsigned char *bytes,
    size_t byte_count,
    size_t *line_offsets,
    size_t line_capacity
);

rivet_result rivet_textview_validate(const rivet_textview *viewer);

rivet_result rivet_textview_line_up(rivet_textview *viewer);

rivet_result rivet_textview_line_down(rivet_textview *viewer);

rivet_result rivet_textview_page_up(
    rivet_textview *viewer,
    size_t visible_rows
);

rivet_result rivet_textview_page_down(
    rivet_textview *viewer,
    size_t visible_rows
);

rivet_result rivet_textview_find_next(
    rivet_textview *viewer,
    const unsigned char *query,
    size_t query_length
);

rivet_result rivet_textview_render(
    rivet_surface *surface,
    const rivet_textview *viewer,
    rivet_rect bounds,
    rivet_textview_style style
);

#ifdef __cplusplus
}
#endif

#endif
