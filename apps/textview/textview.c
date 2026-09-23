/* SPDX-License-Identifier: MPL-2.0 */

#include "textview.h"

#include <limits.h>

#define TV_GLYPH_WIDTH 5ul
#define TV_GLYPH_HEIGHT 7ul
#define TV_GLYPH_PITCH 6ul
#define TV_ROW_HEIGHT 8ul

static int tv_byte_supported(unsigned char code)
{
    return code == 0x0au ||
           code == 0x20u ||
           (code >= 0x2du && code <= 0x3au) ||
           (code >= 0x41u && code <= 0x5au);
}

static int tv_query_byte_supported(unsigned char code)
{
    return code != 0x0au && tv_byte_supported(code);
}

static int tv_glyph_rows(
    unsigned char code,
    unsigned char rows[7]
)
{
    static const unsigned char letters[26][7] = {
        {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11},
        {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
        {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e},
        {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
        {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f},
        {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
        {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f},
        {0x11,0x11,0x11,0x1f,0x11,0x11,0x11},
        {0x1f,0x04,0x04,0x04,0x04,0x04,0x1f},
        {0x07,0x02,0x02,0x02,0x12,0x12,0x0c},
        {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
        {0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
        {0x11,0x1b,0x15,0x15,0x11,0x11,0x11},
        {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
        {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e},
        {0x1e,0x11,0x11,0x1e,0x10,0x10,0x10},
        {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d},
        {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11},
        {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e},
        {0x1f,0x04,0x04,0x04,0x04,0x04,0x04},
        {0x11,0x11,0x11,0x11,0x11,0x11,0x0e},
        {0x11,0x11,0x11,0x11,0x11,0x0a,0x04},
        {0x11,0x11,0x11,0x15,0x15,0x1b,0x11},
        {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11},
        {0x11,0x11,0x0a,0x04,0x04,0x04,0x04},
        {0x1f,0x01,0x02,0x04,0x08,0x10,0x1f}
    };
    static const unsigned char digits[10][7] = {
        {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e},
        {0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
        {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f},
        {0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e},
        {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02},
        {0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e},
        {0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e},
        {0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
        {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e},
        {0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e}
    };
    size_t i;
    unsigned char pattern[7];

    if (rows == NULL) {
        return 0;
    }

    if (code >= 0x41u && code <= 0x5au) {
        for (i = 0u; i < 7u; ++i) {
            pattern[i] = letters[(unsigned int)(code - 0x41u)][i];
        }
    } else if (code >= 0x30u && code <= 0x39u) {
        for (i = 0u; i < 7u; ++i) {
            pattern[i] = digits[(unsigned int)(code - 0x30u)][i];
        }
    } else {
        static const unsigned char blank[7] =
            {0u,0u,0u,0u,0u,0u,0u};
        static const unsigned char dash[7] =
            {0u,0u,0u,0x1fu,0u,0u,0u};
        static const unsigned char dot[7] =
            {0u,0u,0u,0u,0u,0x0cu,0x0cu};
        static const unsigned char slash[7] =
            {0x01u,0x02u,0x04u,0x08u,0x10u,0u,0u};
        static const unsigned char colon[7] =
            {0u,0x0cu,0x0cu,0u,0x0cu,0x0cu,0u};
        const unsigned char *chosen = NULL;

        if (code == 0x20u) {
            chosen = blank;
        } else if (code == 0x2du) {
            chosen = dash;
        } else if (code == 0x2eu) {
            chosen = dot;
        } else if (code == 0x2fu) {
            chosen = slash;
        } else if (code == 0x3au) {
            chosen = colon;
        } else {
            return 0;
        }

        for (i = 0u; i < 7u; ++i) {
            pattern[i] = chosen[i];
        }
    }

    for (i = 0u; i < 7u; ++i) {
        rows[i] = (unsigned char)(pattern[i] << 3);
    }
    return 1;
}

static size_t tv_line_end(
    const rivet_textview *viewer,
    size_t line
)
{
    size_t start = viewer->line_offsets[line];
    size_t end =
        line < viewer->line_count - 1u ?
        viewer->line_offsets[line + 1u] :
        viewer->byte_count;

    if (end > start &&
        viewer->bytes[end - 1u] == 0x0au) {
        --end;
    }

    return end;
}

static size_t tv_line_for_offset(
    const rivet_textview *viewer,
    size_t offset
)
{
    size_t low = 0u;
    size_t high = viewer->line_count;

    while (low < high - 1u) {
        size_t mid = low + (high - low) / 2u;
        if (viewer->line_offsets[mid] <= offset) {
            low = mid;
        } else {
            high = mid;
        }
    }

    return low;
}

rivet_result rivet_textview_validate(const rivet_textview *viewer)
{
    size_t i;

    if (viewer == NULL ||
        viewer->line_offsets == NULL ||
        viewer->line_capacity == 0u ||
        viewer->line_count == 0u ||
        viewer->line_count > viewer->line_capacity ||
        viewer->top_line >= viewer->line_count ||
        (viewer->byte_count != 0u && viewer->bytes == NULL) ||
        (viewer->has_match != 0 && viewer->has_match != 1)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (viewer->line_offsets[0] != 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < viewer->line_count; ++i) {
        if (viewer->line_offsets[i] > viewer->byte_count) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
        if (i != 0u &&
            viewer->line_offsets[i] <=
                viewer->line_offsets[i - 1u]) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
    }

    if (viewer->has_match) {
        if (viewer->match_length == 0u ||
            viewer->match_offset > viewer->byte_count ||
            viewer->match_length >
                viewer->byte_count - viewer->match_offset) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
    } else if (viewer->match_length != 0u ||
               viewer->match_offset != 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    return RIVET_OK;
}

rivet_result rivet_textview_open(
    rivet_textview *viewer,
    const unsigned char *bytes,
    size_t byte_count,
    size_t *line_offsets,
    size_t line_capacity
)
{
    size_t i;
    size_t line_count = 1u;
    size_t line_index = 1u;
    rivet_textview candidate;

    if (viewer == NULL ||
        line_offsets == NULL ||
        line_capacity == 0u ||
        (byte_count != 0u && bytes == NULL)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < byte_count; ++i) {
        if (!tv_byte_supported(bytes[i])) {
            return RIVET_ERR_UNSUPPORTED;
        }
        if (bytes[i] == 0x0au &&
            i + 1u < byte_count) {
            if (line_count == (size_t)-1) {
                return RIVET_ERR_CAPACITY;
            }
            ++line_count;
        }
    }

    if (line_count > line_capacity) {
        return RIVET_ERR_CAPACITY;
    }

    line_offsets[0] = 0u;
    for (i = 0u; i < byte_count; ++i) {
        if (bytes[i] == 0x0au &&
            i + 1u < byte_count) {
            line_offsets[line_index++] = i + 1u;
        }
    }

    candidate.bytes = bytes;
    candidate.byte_count = byte_count;
    candidate.line_offsets = line_offsets;
    candidate.line_capacity = line_capacity;
    candidate.line_count = line_count;
    candidate.top_line = 0u;
    candidate.match_offset = 0u;
    candidate.match_length = 0u;
    candidate.has_match = 0;

    if (rivet_textview_validate(&candidate) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    *viewer = candidate;
    return RIVET_OK;
}

rivet_result rivet_textview_line_up(rivet_textview *viewer)
{
    if (rivet_textview_validate(viewer) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (viewer->top_line != 0u) {
        --viewer->top_line;
    }
    return RIVET_OK;
}

rivet_result rivet_textview_line_down(rivet_textview *viewer)
{
    if (rivet_textview_validate(viewer) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (viewer->top_line + 1u < viewer->line_count) {
        ++viewer->top_line;
    }
    return RIVET_OK;
}

rivet_result rivet_textview_page_up(
    rivet_textview *viewer,
    size_t visible_rows
)
{
    if (rivet_textview_validate(viewer) != RIVET_OK ||
        visible_rows == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (viewer->top_line > visible_rows) {
        viewer->top_line -= visible_rows;
    } else {
        viewer->top_line = 0u;
    }
    return RIVET_OK;
}

rivet_result rivet_textview_page_down(
    rivet_textview *viewer,
    size_t visible_rows
)
{
    size_t remaining;

    if (rivet_textview_validate(viewer) != RIVET_OK ||
        visible_rows == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    remaining = viewer->line_count - 1u - viewer->top_line;
    if (visible_rows < remaining) {
        viewer->top_line += visible_rows;
    } else {
        viewer->top_line = viewer->line_count - 1u;
    }
    return RIVET_OK;
}

rivet_result rivet_textview_find_next(
    rivet_textview *viewer,
    const unsigned char *query,
    size_t query_length
)
{
    size_t i;
    size_t start;
    size_t passes;

    if (rivet_textview_validate(viewer) != RIVET_OK ||
        query == NULL ||
        query_length == 0u ||
        query_length > viewer->byte_count) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < query_length; ++i) {
        if (!tv_query_byte_supported(query[i])) {
            return RIVET_ERR_UNSUPPORTED;
        }
    }

    if (viewer->has_match &&
        viewer->match_offset < viewer->byte_count) {
        start = viewer->match_offset + 1u;
    } else {
        start = viewer->line_offsets[viewer->top_line];
    }

    for (passes = 0u; passes < 2u; ++passes) {
        size_t begin = passes == 0u ? start : 0u;
        size_t end;

        if (passes == 0u) {
            end = viewer->byte_count;
        } else if (viewer->has_match) {
            end = viewer->match_offset;
        } else {
            end = start;
        }

        if (end >= query_length) {
            size_t last = end - query_length;
            for (i = begin; i <= last; ++i) {
                size_t q;
                int equal = 1;

                for (q = 0u; q < query_length; ++q) {
                    if (viewer->bytes[i + q] != query[q]) {
                        equal = 0;
                        break;
                    }
                }

                if (equal) {
                    viewer->match_offset = i;
                    viewer->match_length = query_length;
                    viewer->has_match = 1;
                    viewer->top_line =
                        tv_line_for_offset(viewer, i);
                    return RIVET_OK;
                }
            }
        }

        if (start == 0u) {
            break;
        }
    }

    return RIVET_ERR_NOT_FOUND;
}

static rivet_result tv_draw_glyph(
    rivet_surface *surface,
    long x,
    long y,
    unsigned char code,
    rivet_rgba8 color
)
{
    unsigned char rows[7];

    if (!tv_glyph_rows(code, rows)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    return rivet_surface_blit_mono1(
        surface,
        x,
        y,
        rows,
        sizeof(rows),
        1u,
        TV_GLYPH_WIDTH,
        TV_GLYPH_HEIGHT,
        color
    );
}

rivet_result rivet_textview_render(
    rivet_surface *surface,
    const rivet_textview *viewer,
    rivet_rect bounds,
    rivet_textview_style style
)
{
    size_t visible_rows;
    size_t row_index;

    if (rivet_surface_validate(surface) != RIVET_OK ||
        rivet_textview_validate(viewer) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (bounds.x < 0L || bounds.y < 0L ||
        (unsigned long)bounds.x > surface->width ||
        (unsigned long)bounds.y > surface->height ||
        bounds.width > surface->width - (unsigned long)bounds.x ||
        bounds.height > surface->height - (unsigned long)bounds.y) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (bounds.width < TV_GLYPH_WIDTH ||
        bounds.height < TV_GLYPH_HEIGHT) {
        return RIVET_ERR_CAPACITY;
    }

    if (rivet_surface_fill_rect(
            surface, bounds, style.background) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    visible_rows = (size_t)(bounds.height / TV_ROW_HEIGHT);
    if (visible_rows == 0u) {
        visible_rows = 1u;
    }

    for (row_index = 0u;
         row_index < visible_rows &&
         row_index < viewer->line_count - viewer->top_line;
         ++row_index) {
        size_t line =
            viewer->top_line + row_index;
        size_t start =
            viewer->line_offsets[line];
        size_t end =
            tv_line_end(viewer, line);
        size_t offset;
        unsigned long column = 0ul;
        unsigned long y_offset =
            (unsigned long)row_index * TV_ROW_HEIGHT;

        if (y_offset > (unsigned long)LONG_MAX ||
            bounds.y > LONG_MAX - (long)y_offset) {
            return RIVET_ERR_CAPACITY;
        }

        for (offset = start;
             offset < end;
             ++offset) {
            unsigned long x_offset;
            long x;
            long y;
            rivet_rgba8 foreground =
                style.foreground;

            if (column > ULONG_MAX / TV_GLYPH_PITCH) {
                return RIVET_ERR_CAPACITY;
            }
            x_offset = column * TV_GLYPH_PITCH;

            if (x_offset > ULONG_MAX - TV_GLYPH_WIDTH ||
                x_offset + TV_GLYPH_WIDTH > bounds.width) {
                break;
            }

            if (x_offset > (unsigned long)LONG_MAX ||
                bounds.x > LONG_MAX - (long)x_offset) {
                return RIVET_ERR_CAPACITY;
            }

            x = bounds.x + (long)x_offset;
            y = bounds.y + (long)y_offset;

            if (viewer->has_match &&
                offset >= viewer->match_offset &&
                offset - viewer->match_offset <
                    viewer->match_length) {
                rivet_rect match_cell;
                match_cell.x = x;
                match_cell.y = y;
                match_cell.width = TV_GLYPH_PITCH;
                match_cell.height = TV_ROW_HEIGHT;
                if (rivet_surface_fill_rect(
                        surface,
                        match_cell,
                        style.match_background) != RIVET_OK) {
                    return RIVET_ERR_INVALID_ARGUMENT;
                }
                foreground = style.match_foreground;
            }

            if (tv_draw_glyph(
                    surface,
                    x,
                    y,
                    viewer->bytes[offset],
                    foreground) != RIVET_OK) {
                return RIVET_ERR_UNSUPPORTED;
            }

            ++column;
        }
    }

    return RIVET_OK;
}
