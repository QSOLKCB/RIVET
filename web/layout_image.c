/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/document.h"

#include <limits.h>

#define DOC_GLYPH_WIDTH 6ul
#define DOC_LINE_HEIGHT 8ul
#define DOC_INPUT_WIDTH 80ul
#define DOC_INPUT_HEIGHT 12ul
#define DOC_IMAGE_MAX_DIMENSION 4096ul

typedef struct layout_style {
    rivet_doc_color color;
    rivet_doc_color background;
    int has_background;
    unsigned long margin_top;
    unsigned long margin_bottom;
} layout_style;

typedef struct layout_context {
    const rivet_document *document;
    const rivet_css_rule *rules;
    size_t rule_count;
    unsigned long viewport_width;
    rivet_layout_box *boxes;
    size_t box_capacity;
    size_t box_count;
    unsigned long x;
    unsigned long y;
    unsigned long line_height;
    int emit;
    int pending_space;
    size_t pending_space_node;
    size_t pending_space_start;
    size_t pending_space_end;
    layout_style pending_space_style;
} layout_context;

static int layout_space(unsigned int codepoint)
{
    return codepoint == 0x09u ||
           codepoint == 0x0au ||
           codepoint == 0x0cu ||
           codepoint == 0x0du ||
           codepoint == 0x20u;
}

static rivet_result layout_utf8_one(
    const unsigned char *bytes,
    size_t byte_count,
    size_t offset,
    unsigned int *codepoint,
    size_t *used
)
{
    unsigned char first;
    unsigned int value;
    size_t need;
    size_t i;

    if (bytes == NULL ||
        offset >= byte_count ||
        codepoint == NULL ||
        used == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    first = bytes[offset];
    if (first <= 0x7fu) {
        if (first == 0u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        *codepoint = (unsigned int)first;
        *used = 1u;
        return RIVET_OK;
    }

    if (first >= 0xc2u && first <= 0xdfu) {
        value = (unsigned int)(first & 0x1fu);
        need = 2u;
    } else if (first >= 0xe0u && first <= 0xefu) {
        value = (unsigned int)(first & 0x0fu);
        need = 3u;
    } else if (first >= 0xf0u && first <= 0xf4u) {
        value = (unsigned int)(first & 0x07u);
        need = 4u;
    } else {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (need > byte_count - offset) {
        return RIVET_ERR_UNSUPPORTED;
    }

    for (i = 1u; i < need; ++i) {
        unsigned char continuation = bytes[offset + i];

        if ((continuation & 0xc0u) != 0x80u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        value = (value << 6u) |
                (unsigned int)(continuation & 0x3fu);
    }

    if ((need == 3u &&
         ((first == 0xe0u && bytes[offset + 1u] < 0xa0u) ||
          (first == 0xedu && bytes[offset + 1u] >= 0xa0u))) ||
        (need == 4u &&
         ((first == 0xf0u && bytes[offset + 1u] < 0x90u) ||
          (first == 0xf4u && bytes[offset + 1u] >= 0x90u))) ||
        value > 0x10ffffu ||
        (value >= 0xd800u && value <= 0xdfffu)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    *codepoint = value;
    *used = need;
    return RIVET_OK;
}

static rivet_doc_color layout_color(
    unsigned char r,
    unsigned char g,
    unsigned char b,
    unsigned char a
)
{
    rivet_doc_color color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

static layout_style layout_style_default(
    rivet_doc_color inherited
)
{
    layout_style style;
    style.color = inherited;
    style.background =
        layout_color(0u, 0u, 0u, 0u);
    style.has_background = 0;
    style.margin_top = 0ul;
    style.margin_bottom = 0ul;
    return style;
}

static layout_style layout_style_for(
    const layout_context *context,
    rivet_doc_node_kind kind,
    layout_style inherited
)
{
    layout_style style = inherited;
    size_t i;

    style.margin_top = 0ul;
    style.margin_bottom = 0ul;

    for (i = 0u; i < context->rule_count; ++i) {
        const rivet_css_rule *rule =
            &context->rules[i];

        if (rule->selector != kind) {
            continue;
        }

        if (rule->flags &
            RIVET_CSS_HAS_COLOR) {
            style.color = rule->color;
        }
        if (rule->flags &
            RIVET_CSS_HAS_BACKGROUND) {
            style.background =
                rule->background;
            style.has_background = 1;
        }
        if (rule->flags &
            RIVET_CSS_HAS_MARGIN_TOP) {
            style.margin_top =
                rule->margin_top;
        }
        if (rule->flags &
            RIVET_CSS_HAS_MARGIN_BOTTOM) {
            style.margin_bottom =
                rule->margin_bottom;
        }
    }

    return style;
}

static rivet_result layout_add_y(
    layout_context *context,
    unsigned long amount
)
{
    if (amount > ULONG_MAX - context->y) {
        return RIVET_ERR_CAPACITY;
    }
    context->y += amount;
    return RIVET_OK;
}

static rivet_result layout_newline(
    layout_context *context,
    int force
)
{
    unsigned long height =
        context->line_height;

    context->pending_space = 0;

    if (height == 0ul) {
        if (!force) {
            context->x = 0ul;
            return RIVET_OK;
        }
        height = DOC_LINE_HEIGHT;
    }

    if (layout_add_y(
            context, height) != RIVET_OK) {
        return RIVET_ERR_CAPACITY;
    }

    context->x = 0ul;
    context->line_height = 0ul;
    return RIVET_OK;
}

static rivet_result layout_emit(
    layout_context *context,
    rivet_layout_box box
)
{
    if (context->box_count == (size_t)-1) {
        return RIVET_ERR_CAPACITY;
    }

    if (context->emit) {
        if (context->box_count >=
                context->box_capacity ||
            context->boxes == NULL) {
            return RIVET_ERR_CAPACITY;
        }
        context->boxes[
            context->box_count] = box;
    }

    ++context->box_count;
    return RIVET_OK;
}

static rivet_result layout_emit_text_run(
    layout_context *context,
    size_t node_index,
    size_t source_start,
    size_t source_end,
    unsigned long glyph_count,
    layout_style style
)
{
    rivet_layout_box box;
    unsigned long width;

    if (glyph_count == 0ul ||
        source_end <= source_start) {
        return RIVET_OK;
    }

    if (glyph_count >
        ULONG_MAX / DOC_GLYPH_WIDTH) {
        return RIVET_ERR_CAPACITY;
    }
    width = glyph_count *
            DOC_GLYPH_WIDTH;

    if (context->x >
            context->viewport_width ||
        width >
            context->viewport_width -
            context->x) {
        return RIVET_ERR_CAPACITY;
    }

    box.kind = RIVET_LAYOUT_TEXT;
    box.node_index = node_index;
    box.source.offset = source_start;
    box.source.length =
        source_end - source_start;
    box.x = context->x;
    box.y = context->y;
    box.width = width;
    box.height = DOC_LINE_HEIGHT;
    box.foreground = style.color;
    box.background = style.background;
    box.has_background =
        style.has_background;

    if (layout_emit(context, box) !=
        RIVET_OK) {
        return RIVET_ERR_CAPACITY;
    }

    context->x += width;
    if (context->line_height <
        DOC_LINE_HEIGHT) {
        context->line_height =
            DOC_LINE_HEIGHT;
    }

    return RIVET_OK;
}

static void layout_defer_space(
    layout_context *context,
    size_t node_index,
    size_t source_start,
    size_t source_end,
    layout_style style
)
{
    if (!context->pending_space) {
        context->pending_space = 1;
        context->pending_space_node =
            node_index;
        context->pending_space_start =
            source_start;
        context->pending_space_end =
            source_end;
        context->pending_space_style =
            style;
    } else if (
        context->pending_space_node ==
            node_index &&
        context->pending_space_end ==
            source_start) {
        context->pending_space_end =
            source_end;
    }
}

static rivet_result layout_flush_pending_space(
    layout_context *context,
    unsigned long following_width
)
{
    rivet_result result;
    size_t node_index;
    size_t source_start;
    size_t source_end;
    layout_style style;

    if (!context->pending_space) {
        return RIVET_OK;
    }

    if (context->x == 0ul) {
        context->pending_space = 0;
        return RIVET_OK;
    }

    if (following_width >
            context->viewport_width ||
        context->x >
            context->viewport_width) {
        return RIVET_ERR_CAPACITY;
    }

    if (DOC_GLYPH_WIDTH >
            context->viewport_width -
            context->x ||
        following_width >
            context->viewport_width -
            context->x -
            DOC_GLYPH_WIDTH) {
        return layout_newline(
            context, 1
        );
    }

    node_index =
        context->pending_space_node;
    source_start =
        context->pending_space_start;
    source_end =
        context->pending_space_end;
    style =
        context->pending_space_style;

    result = layout_emit_text_run(
        context,
        node_index,
        source_start,
        source_end,
        1ul,
        style
    );
    if (result != RIVET_OK) {
        return result;
    }

    context->pending_space = 0;
    return RIVET_OK;
}

static rivet_result layout_text_node(
    layout_context *context,
    size_t node_index,
    layout_style style
)
{
    const rivet_doc_node *node =
        &context->document->nodes[node_index];
    size_t offset = node->text.offset;
    size_t end;
    size_t run_start = offset;
    size_t run_end = offset;
    unsigned long run_glyphs = 0ul;
    int pending_space = 0;
    size_t pending_space_start = 0u;
    size_t pending_space_end = 0u;

    if (node->text.offset >
            context->document->source_bytes ||
        node->text.length >
            context->document->source_bytes -
            node->text.offset) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    end = node->text.offset +
          node->text.length;

    while (offset < end) {
        unsigned int codepoint;
        size_t used;
        rivet_result result =
            layout_utf8_one(
                context->document->source,
                end,
                offset,
                &codepoint,
                &used
            );

        if (result != RIVET_OK) {
            return result;
        }

        if (layout_space(codepoint)) {
            if ((run_glyphs != 0ul ||
                 context->x != 0ul ||
                 context->pending_space) &&
                !pending_space) {
                pending_space = 1;
                pending_space_start =
                    offset;
            }
            if (pending_space) {
                pending_space_end =
                    offset + used;
            }
            offset += used;
            continue;
        }

        if (codepoint < 0x20u ||
            codepoint == 0x7fu) {
            return RIVET_ERR_UNSUPPORTED;
        }

        if (context->pending_space) {
            result =
                layout_flush_pending_space(
                    context,
                    DOC_GLYPH_WIDTH
                );
            if (result != RIVET_OK) {
                return result;
            }
            pending_space = 0;
        }

        {
            unsigned long extra =
                pending_space ? 2ul : 1ul;
            unsigned long existing =
                run_glyphs;

            if (extra >
                    ULONG_MAX - existing ||
                existing + extra >
                    ULONG_MAX /
                    DOC_GLYPH_WIDTH) {
                return RIVET_ERR_CAPACITY;
            }

            if (context->x >
                    context->viewport_width ||
                (existing + extra) *
                    DOC_GLYPH_WIDTH >
                    context->viewport_width -
                    context->x) {
                result =
                    layout_emit_text_run(
                        context,
                        node_index,
                        run_start,
                        run_end,
                        run_glyphs,
                        style
                    );
                if (result != RIVET_OK) {
                    return result;
                }

                result = layout_newline(
                    context, 1
                );
                if (result != RIVET_OK) {
                    return result;
                }

                run_start = offset;
                run_end = offset;
                run_glyphs = 0ul;
                pending_space = 0;
            }
        }

        if (pending_space) {
            if (run_glyphs == 0ul) {
                run_start =
                    pending_space_start;
            }
            ++run_glyphs;
            pending_space = 0;
        }

        if (run_glyphs == 0ul) {
            run_start = offset;
        }
        ++run_glyphs;
        run_end = offset + used;
        offset += used;
    }

    {
        rivet_result result =
            layout_emit_text_run(
                context,
                node_index,
                run_start,
                run_end,
                run_glyphs,
                style
            );
        if (result != RIVET_OK) {
            return result;
        }
    }

    if (pending_space &&
        pending_space_end >
            pending_space_start &&
        context->x != 0ul) {
        layout_defer_space(
            context,
            node_index,
            pending_space_start,
            pending_space_end,
            style
        );
    }

    return RIVET_OK;
}

static rivet_result layout_replaced(
    layout_context *context,
    size_t node_index,
    rivet_layout_box_kind kind,
    unsigned long width,
    unsigned long height,
    layout_style style
)
{
    rivet_layout_box box;
    rivet_result result;

    if (width == 0ul ||
        height == 0ul ||
        width > context->viewport_width) {
        return RIVET_ERR_CAPACITY;
    }

    result = layout_flush_pending_space(
        context,
        width
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (context->x >
            context->viewport_width ||
        width >
            context->viewport_width -
            context->x) {
        result = layout_newline(
            context, 1
        );
        if (result != RIVET_OK) {
            return result;
        }
    }

    box.kind = kind;
    box.node_index = node_index;
    box.source.offset = 0u;
    box.source.length = 0u;
    box.x = context->x;
    box.y = context->y;
    box.width = width;
    box.height = height;
    box.foreground = style.color;
    box.background = style.background;
    box.has_background =
        style.has_background;

    result = layout_emit(context, box);
    if (result != RIVET_OK) {
        return result;
    }

    context->x += width;
    if (context->line_height < height) {
        context->line_height = height;
    }

    return RIVET_OK;
}

static int layout_block_kind(
    rivet_doc_node_kind kind
)
{
    return kind == RIVET_DOC_NODE_BODY ||
           kind == RIVET_DOC_NODE_P ||
           kind == RIVET_DOC_NODE_H1 ||
           kind == RIVET_DOC_NODE_H2 ||
           kind == RIVET_DOC_NODE_FORM;
}

static void layout_skip_subtree(
    const layout_context *context,
    size_t node_index,
    size_t *cursor
)
{
    while (*cursor <
               context->document->node_count &&
           context->document->nodes[
               *cursor].parent ==
               node_index) {
        size_t child = *cursor;
        ++(*cursor);
        layout_skip_subtree(
            context,
            child,
            cursor
        );
    }
}

static rivet_result layout_node(
    layout_context *context,
    size_t *cursor,
    layout_style inherited
)
{
    size_t node_index;
    const rivet_doc_node *node;
    layout_style style;
    rivet_result result;
    int block;

    if (*cursor >=
        context->document->node_count) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    node_index = *cursor;
    node =
        &context->document->nodes[node_index];
    ++(*cursor);

    style = layout_style_for(
        context,
        node->kind,
        inherited
    );
    block = layout_block_kind(
        node->kind
    );

    if (node->kind ==
            RIVET_DOC_NODE_HEAD ||
        node->kind ==
            RIVET_DOC_NODE_STYLE) {
        layout_skip_subtree(
            context,
            node_index,
            cursor
        );
        return RIVET_OK;
    }

    if (block) {
        result = layout_newline(
            context, 0
        );
        if (result != RIVET_OK) {
            return result;
        }
        result = layout_add_y(
            context,
            style.margin_top
        );
        if (result != RIVET_OK) {
            return result;
        }
    }

    if (node->kind ==
        RIVET_DOC_NODE_TEXT) {
        return layout_text_node(
            context,
            node_index,
            style
        );
    }

    if (node->kind ==
        RIVET_DOC_NODE_IMG) {
        return layout_replaced(
            context,
            node_index,
            RIVET_LAYOUT_IMAGE,
            node->width,
            node->height,
            style
        );
    }

    if (node->kind ==
        RIVET_DOC_NODE_INPUT) {
        return layout_replaced(
            context,
            node_index,
            RIVET_LAYOUT_INPUT,
            DOC_INPUT_WIDTH,
            DOC_INPUT_HEIGHT,
            style
        );
    }

    if (node->kind ==
        RIVET_DOC_NODE_BR) {
        return layout_newline(
            context, 1
        );
    }

    while (*cursor <
               context->document->node_count &&
           context->document->nodes[
               *cursor].parent ==
               node_index) {
        result = layout_node(
            context,
            cursor,
            style
        );
        if (result != RIVET_OK) {
            return result;
        }
    }

    if (block) {
        result = layout_newline(
            context, 0
        );
        if (result != RIVET_OK) {
            return result;
        }
        result = layout_add_y(
            context,
            style.margin_bottom
        );
        if (result != RIVET_OK) {
            return result;
        }
    }

    return RIVET_OK;
}

static int layout_leaf_kind(
    rivet_doc_node_kind kind
)
{
    return kind == RIVET_DOC_NODE_TEXT ||
           kind == RIVET_DOC_NODE_IMG ||
           kind == RIVET_DOC_NODE_INPUT ||
           kind == RIVET_DOC_NODE_BR;
}

static rivet_result layout_validate_document(
    const rivet_document *document
)
{
    size_t stack[RIVET_DOCUMENT_MAX_DEPTH];
    size_t depth = 0u;
    size_t i;

    if (document == NULL ||
        document->source == NULL ||
        document->nodes == NULL ||
        document->node_count == 0u ||
        document->node_count >
            document->node_capacity) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u;
         i < document->node_count;
         ++i) {
        const rivet_doc_node *node =
            &document->nodes[i];

        if (i == 0u) {
            if (node->parent !=
                    RIVET_DOCUMENT_NO_PARENT ||
                node->kind !=
                    RIVET_DOC_NODE_HTML) {
                return RIVET_ERR_INVALID_ARGUMENT;
            }
            stack[0] = 0u;
            depth = 1u;
        } else {
            if (node->parent ==
                    RIVET_DOCUMENT_NO_PARENT ||
                node->parent >= i) {
                return RIVET_ERR_INVALID_ARGUMENT;
            }

            while (depth != 0u &&
                   stack[depth - 1u] !=
                       node->parent) {
                --depth;
            }
            if (depth == 0u) {
                return RIVET_ERR_INVALID_ARGUMENT;
            }
            if (layout_leaf_kind(
                    document->nodes[
                        node->parent].kind)) {
                return RIVET_ERR_INVALID_ARGUMENT;
            }
            if (!layout_leaf_kind(
                    node->kind)) {
                if (depth >=
                    RIVET_DOCUMENT_MAX_DEPTH) {
                    return RIVET_ERR_CAPACITY;
                }
                stack[depth] = i;
                ++depth;
            }
        }

        if (node->kind ==
                RIVET_DOC_NODE_TEXT &&
            (node->text.offset >
                 document->source_bytes ||
             node->text.length >
                 document->source_bytes -
                 node->text.offset)) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
    }

    return RIVET_OK;
}

static rivet_result layout_run(
    layout_context *context,
    unsigned long *height
)
{
    layout_style initial =
        layout_style_default(
            layout_color(
                0u, 0u, 0u, 0xffu
            )
        );
    size_t cursor = 0u;
    rivet_result result;
    unsigned long final_height;

    context->pending_space = 0;

    result = layout_node(
        context,
        &cursor,
        initial
    );
    if (result != RIVET_OK ||
        cursor !=
            context->document->node_count) {
        return result != RIVET_OK ?
            result :
            RIVET_ERR_INVALID_ARGUMENT;
    }

    final_height = context->y;
    if (context->line_height != 0ul) {
        if (context->line_height >
            ULONG_MAX - final_height) {
            return RIVET_ERR_CAPACITY;
        }
        final_height +=
            context->line_height;
    }

    *height = final_height;
    return RIVET_OK;
}

rivet_result rivet_document_layout(
    const rivet_document *document,
    const rivet_css_rule *rules,
    size_t rule_count,
    unsigned long viewport_width,
    rivet_layout_box *boxes,
    size_t box_capacity,
    size_t *box_count,
    unsigned long *document_height
)
{
    layout_context context;
    unsigned long measured_height = 0ul;
    unsigned long written_height = 0ul;
    size_t needed;
    rivet_result result;

    result = layout_validate_document(
        document
    );
    if (result != RIVET_OK ||
        box_count == NULL ||
        document_height == NULL ||
        viewport_width == 0ul ||
        (rule_count != 0u &&
         rules == NULL)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    context.document = document;
    context.rules = rules;
    context.rule_count = rule_count;
    context.viewport_width =
        viewport_width;
    context.boxes = NULL;
    context.box_capacity = 0u;
    context.box_count = 0u;
    context.x = 0ul;
    context.y = 0ul;
    context.line_height = 0ul;
    context.emit = 0;

    result = layout_run(
        &context,
        &measured_height
    );
    if (result != RIVET_OK) {
        return result;
    }

    needed = context.box_count;
    if (needed > box_capacity ||
        (needed != 0u &&
         boxes == NULL)) {
        return RIVET_ERR_CAPACITY;
    }

    context.boxes = boxes;
    context.box_capacity = box_capacity;
    context.box_count = 0u;
    context.x = 0ul;
    context.y = 0ul;
    context.line_height = 0ul;
    context.emit = 1;

    result = layout_run(
        &context,
        &written_height
    );
    if (result != RIVET_OK ||
        written_height != measured_height ||
        context.box_count != needed) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    *box_count = needed;
    *document_height =
        written_height;
    return RIVET_OK;
}

static int ppm_space(unsigned char byte)
{
    return byte == 0x09u ||
           byte == 0x0au ||
           byte == 0x0du ||
           byte == 0x20u;
}

static rivet_result ppm_skip(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *cursor
)
{
    for (;;) {
        while (*cursor < byte_count &&
               ppm_space(bytes[*cursor])) {
            ++(*cursor);
        }

        if (*cursor < byte_count &&
            bytes[*cursor] == 0x23u) {
            while (*cursor < byte_count &&
                   bytes[*cursor] != 0x0au &&
                   bytes[*cursor] != 0x0du) {
                unsigned char byte =
                    bytes[*cursor];
                if (byte < 0x20u &&
                    byte != 0x09u &&
                    byte != 0x0du) {
                    return RIVET_ERR_UNSUPPORTED;
                }
                ++(*cursor);
            }
            continue;
        }

        break;
    }

    return RIVET_OK;
}

static rivet_result ppm_uint(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *cursor,
    unsigned long *value
)
{
    unsigned long parsed = 0ul;
    int digits = 0;

    if (ppm_skip(
            bytes,
            byte_count,
            cursor) != RIVET_OK) {
        return RIVET_ERR_UNSUPPORTED;
    }

    while (*cursor < byte_count &&
           bytes[*cursor] >= 0x30u &&
           bytes[*cursor] <= 0x39u) {
        unsigned long digit =
            (unsigned long)(
                bytes[*cursor] - 0x30u);

        if (parsed >
            (ULONG_MAX - digit) / 10ul) {
            return RIVET_ERR_CAPACITY;
        }
        parsed = parsed * 10ul + digit;
        ++(*cursor);
        digits = 1;
    }

    if (!digits) {
        return RIVET_ERR_UNSUPPORTED;
    }

    *value = parsed;
    return RIVET_OK;
}

rivet_result rivet_image_decode_ppm(
    rivet_document_image *image,
    const unsigned char *bytes,
    size_t byte_count,
    unsigned char *pixels,
    size_t pixel_capacity
)
{
    size_t cursor = 0u;
    unsigned long width;
    unsigned long height;
    unsigned long max_value;
    size_t pixel_count;
    size_t source_bytes;
    size_t output_bytes;
    size_t i;
    rivet_document_image candidate;

    if (image == NULL ||
        bytes == NULL ||
        byte_count < 4u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (bytes[0] != 0x50u ||
        bytes[1] != 0x36u) {
        return RIVET_ERR_UNSUPPORTED;
    }
    cursor = 2u;

    if (cursor >= byte_count ||
        !ppm_space(bytes[cursor])) {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (ppm_uint(
            bytes,
            byte_count,
            &cursor,
            &width) != RIVET_OK ||
        ppm_uint(
            bytes,
            byte_count,
            &cursor,
            &height) != RIVET_OK ||
        ppm_uint(
            bytes,
            byte_count,
            &cursor,
            &max_value) != RIVET_OK) {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (width == 0ul ||
        height == 0ul ||
        width > DOC_IMAGE_MAX_DIMENSION ||
        height > DOC_IMAGE_MAX_DIMENSION ||
        max_value != 255ul) {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (cursor >= byte_count ||
        !ppm_space(bytes[cursor])) {
        return RIVET_ERR_UNSUPPORTED;
    }
    ++cursor;

    if (width > (unsigned long)(size_t)-1 ||
        height > (unsigned long)(size_t)-1) {
        return RIVET_ERR_CAPACITY;
    }

    if ((size_t)width >
        (size_t)-1 / (size_t)height) {
        return RIVET_ERR_CAPACITY;
    }
    pixel_count =
        (size_t)width * (size_t)height;

    if (pixel_count >
        (size_t)-1 / 3u) {
        return RIVET_ERR_CAPACITY;
    }
    source_bytes = pixel_count * 3u;

    if (source_bytes !=
        byte_count - cursor) {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (pixel_count >
        (size_t)-1 /
        RIVET_DOCUMENT_IMAGE_PIXEL_BYTES) {
        return RIVET_ERR_CAPACITY;
    }
    output_bytes =
        pixel_count *
        RIVET_DOCUMENT_IMAGE_PIXEL_BYTES;

    if (output_bytes > pixel_capacity ||
        (output_bytes != 0u &&
         pixels == NULL)) {
        return RIVET_ERR_CAPACITY;
    }

    for (i = 0u; i < pixel_count; ++i) {
        pixels[i * 4u + 0u] =
            bytes[cursor + i * 3u + 0u];
        pixels[i * 4u + 1u] =
            bytes[cursor + i * 3u + 1u];
        pixels[i * 4u + 2u] =
            bytes[cursor + i * 3u + 2u];
        pixels[i * 4u + 3u] = 0xffu;
    }

    candidate.width = width;
    candidate.height = height;
    candidate.stride_bytes =
        (size_t)width *
        RIVET_DOCUMENT_IMAGE_PIXEL_BYTES;
    candidate.pixels = pixels;
    candidate.pixel_bytes =
        output_bytes;

    *image = candidate;
    return RIVET_OK;
}
