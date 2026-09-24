/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/document.h"

#include <limits.h>

typedef struct html_parser {
    const unsigned char *bytes;
    size_t byte_count;
    rivet_doc_node *nodes;
    size_t capacity;
    size_t count;
    size_t stack_nodes[RIVET_DOCUMENT_MAX_DEPTH];
    rivet_doc_node_kind stack_kinds[RIVET_DOCUMENT_MAX_DEPTH];
    size_t depth;
    unsigned int requirements;
    int emit;
    int seen_html;
    int seen_head;
    int seen_body;
    int seen_doctype;
} html_parser;

static int doc_space(unsigned char byte)
{
    return byte == 0x09u ||
           byte == 0x0au ||
           byte == 0x0cu ||
           byte == 0x0du ||
           byte == 0x20u;
}

static unsigned char doc_lower(unsigned char byte)
{
    if (byte >= 0x41u && byte <= 0x5au) {
        return (unsigned char)(byte + 0x20u);
    }
    return byte;
}

static int doc_name_byte(unsigned char byte)
{
    byte = doc_lower(byte);
    return (byte >= 0x61u && byte <= 0x7au) ||
           (byte >= 0x30u && byte <= 0x39u) ||
           byte == 0x2du;
}

static int doc_slice_equal_ascii(
    const unsigned char *bytes,
    size_t offset,
    size_t length,
    const unsigned char *expected,
    size_t expected_length
)
{
    size_t i;

    if (length != expected_length) {
        return 0;
    }

    for (i = 0u; i < length; ++i) {
        if (doc_lower(bytes[offset + i]) != expected[i]) {
            return 0;
        }
    }
    return 1;
}

static rivet_result doc_utf8_validate(
    const unsigned char *bytes,
    size_t byte_count
)
{
    size_t offset = 0u;

    while (offset < byte_count) {
        unsigned char first = bytes[offset];
        size_t need;
        unsigned int value;
        size_t i;

        if (first <= 0x7fu) {
            if (first == 0u) {
                return RIVET_ERR_UNSUPPORTED;
            }
            ++offset;
            continue;
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

        offset += need;
    }

    return RIVET_OK;
}

static void doc_slice_clear(rivet_doc_slice *slice)
{
    slice->offset = 0u;
    slice->length = 0u;
}

static int doc_slice_present(const rivet_doc_slice *slice)
{
    return slice->offset != 0u ||
           slice->length != 0u;
}

static rivet_doc_node doc_node_blank(
    rivet_doc_node_kind kind,
    size_t parent
)
{
    rivet_doc_node node;

    node.kind = kind;
    node.parent = parent;
    doc_slice_clear(&node.text);
    doc_slice_clear(&node.href);
    doc_slice_clear(&node.action);
    doc_slice_clear(&node.src);
    doc_slice_clear(&node.name);
    doc_slice_clear(&node.value);
    node.width = 0ul;
    node.height = 0ul;
    return node;
}

static rivet_result html_emit_node(
    html_parser *parser,
    rivet_doc_node node,
    size_t *node_index
)
{
    size_t index;

    if (parser == NULL || node_index == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (parser->count == (size_t)-1) {
        return RIVET_ERR_CAPACITY;
    }

    index = parser->count++;
    if (parser->emit) {
        if (index >= parser->capacity ||
            parser->nodes == NULL) {
            return RIVET_ERR_CAPACITY;
        }
        parser->nodes[index] = node;
    }

    *node_index = index;
    return RIVET_OK;
}

static rivet_result html_kind_for_name(
    const unsigned char *bytes,
    size_t offset,
    size_t length,
    rivet_doc_node_kind *kind
)
{
    static const unsigned char html[] =
        {0x68u,0x74u,0x6du,0x6cu};
    static const unsigned char head[] =
        {0x68u,0x65u,0x61u,0x64u};
    static const unsigned char body[] =
        {0x62u,0x6fu,0x64u,0x79u};
    static const unsigned char p[] = {0x70u};
    static const unsigned char h1[] = {0x68u,0x31u};
    static const unsigned char h2[] = {0x68u,0x32u};
    static const unsigned char a[] = {0x61u};
    static const unsigned char form[] =
        {0x66u,0x6fu,0x72u,0x6du};
    static const unsigned char input[] =
        {0x69u,0x6eu,0x70u,0x75u,0x74u};
    static const unsigned char img[] =
        {0x69u,0x6du,0x67u};
    static const unsigned char br[] =
        {0x62u,0x72u};
    static const unsigned char style[] =
        {0x73u,0x74u,0x79u,0x6cu,0x65u};

    if (doc_slice_equal_ascii(
            bytes, offset, length, html, sizeof(html))) {
        *kind = RIVET_DOC_NODE_HTML;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, head, sizeof(head))) {
        *kind = RIVET_DOC_NODE_HEAD;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, body, sizeof(body))) {
        *kind = RIVET_DOC_NODE_BODY;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, p, sizeof(p))) {
        *kind = RIVET_DOC_NODE_P;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, h1, sizeof(h1))) {
        *kind = RIVET_DOC_NODE_H1;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, h2, sizeof(h2))) {
        *kind = RIVET_DOC_NODE_H2;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, a, sizeof(a))) {
        *kind = RIVET_DOC_NODE_A;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, form, sizeof(form))) {
        *kind = RIVET_DOC_NODE_FORM;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, input, sizeof(input))) {
        *kind = RIVET_DOC_NODE_INPUT;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, img, sizeof(img))) {
        *kind = RIVET_DOC_NODE_IMG;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, br, sizeof(br))) {
        *kind = RIVET_DOC_NODE_BR;
    } else if (doc_slice_equal_ascii(
                   bytes, offset, length, style, sizeof(style))) {
        *kind = RIVET_DOC_NODE_STYLE;
    } else {
        return RIVET_ERR_UNSUPPORTED;
    }

    return RIVET_OK;
}

static int html_void_kind(rivet_doc_node_kind kind)
{
    return kind == RIVET_DOC_NODE_INPUT ||
           kind == RIVET_DOC_NODE_IMG ||
           kind == RIVET_DOC_NODE_BR;
}

static rivet_result html_parse_unsigned(
    const unsigned char *bytes,
    rivet_doc_slice slice,
    unsigned long *value
)
{
    size_t i;
    unsigned long result = 0ul;

    if (slice.length == 0u || value == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < slice.length; ++i) {
        unsigned char byte = bytes[slice.offset + i];
        unsigned long digit;

        if (byte < 0x30u || byte > 0x39u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        digit = (unsigned long)(byte - 0x30u);
        if (result > (ULONG_MAX - digit) / 10ul) {
            return RIVET_ERR_CAPACITY;
        }
        result = result * 10ul + digit;
        if (result > 4096ul) {
            return RIVET_ERR_CAPACITY;
        }
    }

    if (result == 0ul) {
        return RIVET_ERR_UNSUPPORTED;
    }

    *value = result;
    return RIVET_OK;
}

static rivet_result html_apply_attribute(
    const unsigned char *bytes,
    size_t name_offset,
    size_t name_length,
    rivet_doc_slice value,
    rivet_doc_node *node
)
{
    static const unsigned char href[] =
        {0x68u,0x72u,0x65u,0x66u};
    static const unsigned char action[] =
        {0x61u,0x63u,0x74u,0x69u,0x6fu,0x6eu};
    static const unsigned char src[] =
        {0x73u,0x72u,0x63u};
    static const unsigned char name[] =
        {0x6eu,0x61u,0x6du,0x65u};
    static const unsigned char val[] =
        {0x76u,0x61u,0x75u,0x65u};
    static const unsigned char width[] =
        {0x77u,0x69u,0x64u,0x74u,0x68u};
    static const unsigned char height[] =
        {0x68u,0x65u,0x69u,0x67u,0x68u,0x74u};

    if (doc_slice_equal_ascii(
            bytes, name_offset, name_length,
            href, sizeof(href))) {
        if (node->kind == RIVET_DOC_NODE_A) {
            if (doc_slice_present(&node->href)) {
                return RIVET_ERR_DUPLICATE;
            }
            node->href = value;
        }
        return RIVET_OK;
    }

    if (doc_slice_equal_ascii(
            bytes, name_offset, name_length,
            action, sizeof(action))) {
        if (node->kind == RIVET_DOC_NODE_FORM) {
            if (doc_slice_present(&node->action)) {
                return RIVET_ERR_DUPLICATE;
            }
            node->action = value;
        }
        return RIVET_OK;
    }

    if (doc_slice_equal_ascii(
            bytes, name_offset, name_length,
            src, sizeof(src))) {
        if (node->kind == RIVET_DOC_NODE_IMG) {
            if (doc_slice_present(&node->src)) {
                return RIVET_ERR_DUPLICATE;
            }
            node->src = value;
        }
        return RIVET_OK;
    }

    if (doc_slice_equal_ascii(
            bytes, name_offset, name_length,
            name, sizeof(name))) {
        if (node->kind == RIVET_DOC_NODE_INPUT) {
            if (doc_slice_present(&node->name)) {
                return RIVET_ERR_DUPLICATE;
            }
            node->name = value;
        }
        return RIVET_OK;
    }

    if (doc_slice_equal_ascii(
            bytes, name_offset, name_length,
            val, sizeof(val))) {
        if (node->kind == RIVET_DOC_NODE_INPUT) {
            if (doc_slice_present(&node->value)) {
                return RIVET_ERR_DUPLICATE;
            }
            node->value = value;
        }
        return RIVET_OK;
    }

    if (doc_slice_equal_ascii(
            bytes, name_offset, name_length,
            width, sizeof(width))) {
        if (node->kind == RIVET_DOC_NODE_IMG) {
            if (node->width != 0ul) {
                return RIVET_ERR_DUPLICATE;
            }
            return html_parse_unsigned(
                bytes, value, &node->width
            );
        }
        return RIVET_OK;
    }

    if (doc_slice_equal_ascii(
            bytes, name_offset, name_length,
            height, sizeof(height))) {
        if (node->kind == RIVET_DOC_NODE_IMG) {
            if (node->height != 0ul) {
                return RIVET_ERR_DUPLICATE;
            }
            return html_parse_unsigned(
                bytes, value, &node->height
            );
        }
        return RIVET_OK;
    }

    return RIVET_OK;
}

static rivet_result html_parse_open_tag(
    html_parser *parser,
    size_t *cursor
)
{
    size_t pos = *cursor;
    size_t name_start;
    size_t name_end;
    rivet_doc_node_kind kind;
    size_t parent =
        parser->depth == 0u ?
        RIVET_DOCUMENT_NO_PARENT :
        parser->stack_nodes[parser->depth - 1u];
    rivet_doc_node node;
    size_t node_index;
    int self_closing = 0;
    rivet_result result;

    ++pos;
    name_start = pos;
    while (pos < parser->byte_count &&
           doc_name_byte(parser->bytes[pos])) {
        ++pos;
    }
    name_end = pos;

    if (name_end == name_start) {
        return RIVET_ERR_UNSUPPORTED;
    }

    result = html_kind_for_name(
        parser->bytes,
        name_start,
        name_end - name_start,
        &kind
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (kind == RIVET_DOC_NODE_HTML) {
        if (parser->seen_html ||
            parser->depth != 0u) {
            return RIVET_ERR_DUPLICATE;
        }
        parser->seen_html = 1;
    } else if (!parser->seen_html ||
               parser->depth == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (kind == RIVET_DOC_NODE_HEAD) {
        if (parser->seen_head) {
            return RIVET_ERR_DUPLICATE;
        }
        if (parser->depth != 1u ||
            parser->stack_kinds[0] !=
                RIVET_DOC_NODE_HTML ||
            parser->seen_body) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
        parser->seen_head = 1;
    }
    if (kind == RIVET_DOC_NODE_BODY) {
        if (parser->seen_body) {
            return RIVET_ERR_DUPLICATE;
        }
        if (parser->depth != 1u ||
            parser->stack_kinds[0] !=
                RIVET_DOC_NODE_HTML) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
        parser->seen_body = 1;
    }

    node = doc_node_blank(kind, parent);

    while (pos < parser->byte_count) {
        size_t attr_start;
        size_t attr_end;
        rivet_doc_slice attr_value;

        while (pos < parser->byte_count &&
               doc_space(parser->bytes[pos])) {
            ++pos;
        }

        if (pos >= parser->byte_count) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }

        if (parser->bytes[pos] == 0x3eu) {
            ++pos;
            break;
        }

        if (parser->bytes[pos] == 0x2fu &&
            pos + 1u < parser->byte_count &&
            parser->bytes[pos + 1u] == 0x3eu) {
            self_closing = 1;
            pos += 2u;
            break;
        }

        attr_start = pos;
        while (pos < parser->byte_count &&
               doc_name_byte(parser->bytes[pos])) {
            ++pos;
        }
        attr_end = pos;
        if (attr_end == attr_start) {
            return RIVET_ERR_UNSUPPORTED;
        }

        while (pos < parser->byte_count &&
               doc_space(parser->bytes[pos])) {
            ++pos;
        }
        if (pos >= parser->byte_count ||
            parser->bytes[pos] != 0x3du) {
            return RIVET_ERR_UNSUPPORTED;
        }
        ++pos;

        while (pos < parser->byte_count &&
               doc_space(parser->bytes[pos])) {
            ++pos;
        }
        if (pos >= parser->byte_count ||
            parser->bytes[pos] != 0x22u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        ++pos;

        attr_value.offset = pos;
        while (pos < parser->byte_count &&
               parser->bytes[pos] != 0x22u) {
            if (parser->bytes[pos] == 0x3cu ||
                parser->bytes[pos] == 0x26u ||
                parser->bytes[pos] == 0u) {
                return RIVET_ERR_UNSUPPORTED;
            }
            ++pos;
        }
        if (pos >= parser->byte_count) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
        attr_value.length =
            pos - attr_value.offset;
        ++pos;

        result = html_apply_attribute(
            parser->bytes,
            attr_start,
            attr_end - attr_start,
            attr_value,
            &node
        );
        if (result != RIVET_OK) {
            return result;
        }
    }

    if (self_closing && !html_void_kind(kind)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (kind == RIVET_DOC_NODE_A &&
        doc_slice_present(&node.href)) {
        parser->requirements |=
            RIVET_DOC_REQUIRE_LINKS;
    }
    if (kind == RIVET_DOC_NODE_FORM ||
        kind == RIVET_DOC_NODE_INPUT) {
        parser->requirements |=
            RIVET_DOC_REQUIRE_FORMS;
    }
    if (kind == RIVET_DOC_NODE_IMG) {
        if (node.src.length == 0u ||
            node.width == 0ul ||
            node.height == 0ul) {
            return RIVET_ERR_UNSUPPORTED;
        }
        parser->requirements |=
            RIVET_DOC_REQUIRE_IMAGE_PPM;
    }
    if (kind == RIVET_DOC_NODE_STYLE) {
        parser->requirements |=
            RIVET_DOC_REQUIRE_CSS;
    }

    result = html_emit_node(
        parser, node, &node_index
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (!html_void_kind(kind)) {
        if (parser->depth >=
            RIVET_DOCUMENT_MAX_DEPTH) {
            return RIVET_ERR_CAPACITY;
        }
        parser->stack_nodes[parser->depth] =
            node_index;
        parser->stack_kinds[parser->depth] =
            kind;
        ++parser->depth;
    }

    *cursor = pos;
    return RIVET_OK;
}

static rivet_result html_parse_close_tag(
    html_parser *parser,
    size_t *cursor
)
{
    size_t pos = *cursor + 2u;
    size_t name_start = pos;
    size_t name_end;
    rivet_doc_node_kind kind;
    rivet_result result;

    while (pos < parser->byte_count &&
           doc_name_byte(parser->bytes[pos])) {
        ++pos;
    }
    name_end = pos;

    while (pos < parser->byte_count &&
           doc_space(parser->bytes[pos])) {
        ++pos;
    }

    if (name_end == name_start ||
        pos >= parser->byte_count ||
        parser->bytes[pos] != 0x3eu) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    ++pos;

    result = html_kind_for_name(
        parser->bytes,
        name_start,
        name_end - name_start,
        &kind
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (html_void_kind(kind) ||
        parser->depth == 0u ||
        parser->stack_kinds[
            parser->depth - 1u] != kind) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    --parser->depth;
    *cursor = pos;
    return RIVET_OK;
}

static int html_text_has_content(
    const unsigned char *bytes,
    size_t offset,
    size_t length
)
{
    size_t i;

    for (i = 0u; i < length; ++i) {
        if (!doc_space(bytes[offset + i])) {
            return 1;
        }
    }
    return 0;
}

static rivet_result html_parse_text(
    html_parser *parser,
    size_t *cursor
)
{
    size_t start = *cursor;
    size_t pos = start;
    rivet_doc_node node;
    size_t node_index;
    size_t parent;

    while (pos < parser->byte_count &&
           parser->bytes[pos] != 0x3cu) {
        if (parser->bytes[pos] == 0x26u ||
            parser->bytes[pos] == 0u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        ++pos;
    }

    if (pos == start) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (!html_text_has_content(
            parser->bytes,
            start,
            pos - start)) {
        *cursor = pos;
        return RIVET_OK;
    }

    if (parser->depth == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    parent =
        parser->stack_nodes[parser->depth - 1u];
    node = doc_node_blank(
        RIVET_DOC_NODE_TEXT,
        parent
    );
    node.text.offset = start;
    node.text.length = pos - start;

    if (html_emit_node(
            parser,
            node,
            &node_index) != RIVET_OK) {
        return RIVET_ERR_CAPACITY;
    }

    *cursor = pos;
    return RIVET_OK;
}

static int html_doctype_at(
    const unsigned char *bytes,
    size_t byte_count,
    size_t offset
)
{
    static const unsigned char doctype[] = {
        0x3cu,0x21u,0x64u,0x6fu,0x63u,0x74u,
        0x79u,0x70u,0x65u,0x20u,0x68u,0x74u,
        0x6du,0x6cu,0x3eu
    };
    size_t i;

    if (sizeof(doctype) >
        byte_count - offset) {
        return 0;
    }

    for (i = 0u; i < sizeof(doctype); ++i) {
        unsigned char actual =
            bytes[offset + i];
        unsigned char expected =
            doctype[i];

        if (actual >= 0x41u &&
            actual <= 0x5au) {
            actual =
                (unsigned char)(actual + 0x20u);
        }
        if (actual != expected) {
            return 0;
        }
    }

    return 1;
}

static rivet_result html_run(
    html_parser *parser
)
{
    size_t cursor = 0u;

    while (cursor < parser->byte_count) {
        rivet_result result;

        if (parser->bytes[cursor] == 0x3cu) {
            if (html_doctype_at(
                    parser->bytes,
                    parser->byte_count,
                    cursor)) {
                if (parser->seen_doctype ||
                    parser->seen_html ||
                    parser->depth != 0u ||
                    parser->count != 0u) {
                    return RIVET_ERR_INVALID_ARGUMENT;
                }
                parser->seen_doctype = 1;
                cursor += 15u;
                continue;
            }

            if (cursor + 1u <
                    parser->byte_count &&
                parser->bytes[cursor + 1u] ==
                    0x2fu) {
                result = html_parse_close_tag(
                    parser, &cursor
                );
            } else {
                result = html_parse_open_tag(
                    parser, &cursor
                );
            }
        } else {
            result = html_parse_text(
                parser, &cursor
            );
        }

        if (result != RIVET_OK) {
            return result;
        }
    }

    if (parser->depth != 0u ||
        !parser->seen_html ||
        !parser->seen_body) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    return RIVET_OK;
}

rivet_result rivet_html_parse(
    rivet_document *document,
    const unsigned char *bytes,
    size_t byte_count,
    rivet_doc_node *nodes,
    size_t node_capacity
)
{
    html_parser parser;
    rivet_document candidate;
    rivet_result result;

    if (document == NULL ||
        bytes == NULL ||
        byte_count == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    result = doc_utf8_validate(
        bytes, byte_count
    );
    if (result != RIVET_OK) {
        return result;
    }

    parser.bytes = bytes;
    parser.byte_count = byte_count;
    parser.nodes = NULL;
    parser.capacity = 0u;
    parser.count = 0u;
    parser.depth = 0u;
    parser.requirements =
        RIVET_DOC_REQUIRE_HTML;
    parser.emit = 0;
    parser.seen_html = 0;
    parser.seen_head = 0;
    parser.seen_body = 0;
    parser.seen_doctype = 0;

    result = html_run(&parser);
    if (result != RIVET_OK) {
        return result;
    }

    if (parser.count > node_capacity ||
        (parser.count != 0u &&
         nodes == NULL)) {
        return RIVET_ERR_CAPACITY;
    }

    parser.nodes = nodes;
    parser.capacity = node_capacity;
    parser.count = 0u;
    parser.depth = 0u;
    parser.requirements =
        RIVET_DOC_REQUIRE_HTML;
    parser.emit = 1;
    parser.seen_html = 0;
    parser.seen_head = 0;
    parser.seen_body = 0;
    parser.seen_doctype = 0;

    result = html_run(&parser);
    if (result != RIVET_OK) {
        return result;
    }

    candidate.source = bytes;
    candidate.source_bytes = byte_count;
    candidate.nodes = nodes;
    candidate.node_capacity = node_capacity;
    candidate.node_count = parser.count;
    candidate.requirements =
        parser.requirements;

    *document = candidate;
    return RIVET_OK;
}

rivet_result rivet_document_required_capabilities(
    const rivet_document *document,
    const char **ids,
    size_t capacity,
    size_t *count
)
{
    static const char html_id[] = "document.html";
    static const char css_id[] = "document.css";
    static const char links_id[] = "document.links";
    static const char forms_id[] = "document.forms";
    static const char image_id[] = "image.ppm";
    size_t needed = 0u;
    size_t written = 0u;

    if (document == NULL ||
        count == NULL ||
        document->source == NULL ||
        document->nodes == NULL ||
        document->node_count == 0u ||
        document->node_count >
            document->node_capacity) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (document->requirements &
        RIVET_DOC_REQUIRE_HTML) {
        ++needed;
    }
    if (document->requirements &
        RIVET_DOC_REQUIRE_CSS) {
        ++needed;
    }
    if (document->requirements &
        RIVET_DOC_REQUIRE_LINKS) {
        ++needed;
    }
    if (document->requirements &
        RIVET_DOC_REQUIRE_FORMS) {
        ++needed;
    }
    if (document->requirements &
        RIVET_DOC_REQUIRE_IMAGE_PPM) {
        ++needed;
    }

    if (needed > capacity ||
        (needed != 0u && ids == NULL)) {
        return RIVET_ERR_CAPACITY;
    }

#define DOC_WRITE_CAP(flag, id_value) \
    do { \
        if (document->requirements & (flag)) { \
            ids[written++] = (id_value); \
        } \
    } while (0)

    DOC_WRITE_CAP(
        RIVET_DOC_REQUIRE_HTML, html_id);
    DOC_WRITE_CAP(
        RIVET_DOC_REQUIRE_CSS, css_id);
    DOC_WRITE_CAP(
        RIVET_DOC_REQUIRE_LINKS, links_id);
    DOC_WRITE_CAP(
        RIVET_DOC_REQUIRE_FORMS, forms_id);
    DOC_WRITE_CAP(
        RIVET_DOC_REQUIRE_IMAGE_PPM,
        image_id);

#undef DOC_WRITE_CAP

    *count = needed;
    return RIVET_OK;
}

static void css_skip_space(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *cursor
)
{
    while (*cursor < byte_count &&
           doc_space(bytes[*cursor])) {
        ++(*cursor);
    }
}

static int css_hex_value(
    unsigned char byte,
    unsigned int *value
)
{
    if (byte >= 0x30u && byte <= 0x39u) {
        *value = (unsigned int)(byte - 0x30u);
        return 1;
    }
    byte = doc_lower(byte);
    if (byte >= 0x61u && byte <= 0x66u) {
        *value =
            10u + (unsigned int)(byte - 0x61u);
        return 1;
    }
    return 0;
}

static rivet_result css_parse_color(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *cursor,
    rivet_doc_color *color
)
{
    unsigned int nibbles[6];
    size_t i;

    if (*cursor >= byte_count ||
        bytes[*cursor] != 0x23u) {
        return RIVET_ERR_UNSUPPORTED;
    }
    ++(*cursor);

    if (6u > byte_count - *cursor) {
        return RIVET_ERR_UNSUPPORTED;
    }

    for (i = 0u; i < 6u; ++i) {
        if (!css_hex_value(
                bytes[*cursor + i],
                &nibbles[i])) {
            return RIVET_ERR_UNSUPPORTED;
        }
    }
    *cursor += 6u;

    color->r = (unsigned char)(
        nibbles[0] * 16u + nibbles[1]);
    color->g = (unsigned char)(
        nibbles[2] * 16u + nibbles[3]);
    color->b = (unsigned char)(
        nibbles[4] * 16u + nibbles[5]);
    color->a = 0xffu;
    return RIVET_OK;
}

static rivet_result css_parse_pixels(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *cursor,
    unsigned long *pixels
)
{
    unsigned long value = 0ul;
    int digits = 0;

    while (*cursor < byte_count &&
           bytes[*cursor] >= 0x30u &&
           bytes[*cursor] <= 0x39u) {
        unsigned long digit =
            (unsigned long)(
                bytes[*cursor] - 0x30u);

        if (value > (ULONG_MAX - digit) /
                    10ul) {
            return RIVET_ERR_CAPACITY;
        }
        value = value * 10ul + digit;
        if (value > 1024ul) {
            return RIVET_ERR_CAPACITY;
        }
        ++(*cursor);
        digits = 1;
    }

    if (!digits ||
        *cursor + 2u > byte_count ||
        doc_lower(bytes[*cursor]) != 0x70u ||
        doc_lower(bytes[*cursor + 1u]) !=
            0x78u) {
        return RIVET_ERR_UNSUPPORTED;
    }

    *cursor += 2u;
    *pixels = value;
    return RIVET_OK;
}

static rivet_result css_selector_kind(
    const unsigned char *bytes,
    size_t offset,
    size_t length,
    rivet_doc_node_kind *kind
)
{
    return html_kind_for_name(
        bytes, offset, length, kind
    );
}

static int css_selector_supported(
    rivet_doc_node_kind kind
)
{
    return kind == RIVET_DOC_NODE_BODY ||
           kind == RIVET_DOC_NODE_P ||
           kind == RIVET_DOC_NODE_H1 ||
           kind == RIVET_DOC_NODE_H2 ||
           kind == RIVET_DOC_NODE_A ||
           kind == RIVET_DOC_NODE_FORM ||
           kind == RIVET_DOC_NODE_INPUT ||
           kind == RIVET_DOC_NODE_IMG;
}

static rivet_result css_parse_one(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *cursor,
    rivet_css_rule *rule
)
{
    static const unsigned char color_name[] =
        {0x63u,0x6fu,0x6cu,0x6fu,0x72u};
    static const unsigned char background_name[] = {
        0x62u,0x61u,0x63u,0x6bu,0x67u,0x72u,
        0x6fu,0x75u,0x6eu,0x64u,0x2du,0x63u,
        0x6fu,0x6cu,0x6fu,0x72u
    };
    static const unsigned char margin_top_name[] = {
        0x6du,0x61u,0x72u,0x67u,0x69u,0x6eu,
        0x2du,0x74u,0x6fu,0x70u
    };
    static const unsigned char margin_bottom_name[] = {
        0x6du,0x61u,0x72u,0x67u,0x69u,0x6eu,
        0x2du,0x62u,0x6fu,0x74u,0x74u,0x6fu,
        0x6du
    };
    size_t selector_start;
    size_t selector_end;
    rivet_doc_node_kind kind;

    css_skip_space(
        bytes, byte_count, cursor
    );
    selector_start = *cursor;
    while (*cursor < byte_count &&
           doc_name_byte(bytes[*cursor])) {
        ++(*cursor);
    }
    selector_end = *cursor;

    if (selector_end == selector_start ||
        css_selector_kind(
            bytes,
            selector_start,
            selector_end - selector_start,
            &kind) != RIVET_OK ||
        !css_selector_supported(kind)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    css_skip_space(
        bytes, byte_count, cursor
    );
    if (*cursor >= byte_count ||
        bytes[*cursor] != 0x7bu) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    ++(*cursor);

    rule->selector = kind;
    rule->flags = 0u;
    rule->color.r = 0u;
    rule->color.g = 0u;
    rule->color.b = 0u;
    rule->color.a = 0xffu;
    rule->background = rule->color;
    rule->margin_top = 0ul;
    rule->margin_bottom = 0ul;

    for (;;) {
        size_t name_start;
        size_t name_end;
        unsigned int flag;
        rivet_result result;

        css_skip_space(
            bytes, byte_count, cursor
        );

        if (*cursor >= byte_count) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
        if (bytes[*cursor] == 0x7du) {
            ++(*cursor);
            return RIVET_OK;
        }

        name_start = *cursor;
        while (*cursor < byte_count &&
               doc_name_byte(bytes[*cursor])) {
            ++(*cursor);
        }
        name_end = *cursor;

        if (name_end == name_start) {
            return RIVET_ERR_UNSUPPORTED;
        }

        css_skip_space(
            bytes, byte_count, cursor
        );
        if (*cursor >= byte_count ||
            bytes[*cursor] != 0x3au) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
        ++(*cursor);
        css_skip_space(
            bytes, byte_count, cursor
        );

        if (doc_slice_equal_ascii(
                bytes,
                name_start,
                name_end - name_start,
                color_name,
                sizeof(color_name))) {
            flag = RIVET_CSS_HAS_COLOR;
            if (rule->flags & flag) {
                return RIVET_ERR_DUPLICATE;
            }
            result = css_parse_color(
                bytes, byte_count, cursor,
                &rule->color
            );
        } else if (doc_slice_equal_ascii(
                       bytes,
                       name_start,
                       name_end - name_start,
                       background_name,
                       sizeof(background_name))) {
            flag = RIVET_CSS_HAS_BACKGROUND;
            if (rule->flags & flag) {
                return RIVET_ERR_DUPLICATE;
            }
            result = css_parse_color(
                bytes, byte_count, cursor,
                &rule->background
            );
        } else if (doc_slice_equal_ascii(
                       bytes,
                       name_start,
                       name_end - name_start,
                       margin_top_name,
                       sizeof(margin_top_name))) {
            flag = RIVET_CSS_HAS_MARGIN_TOP;
            if (rule->flags & flag) {
                return RIVET_ERR_DUPLICATE;
            }
            result = css_parse_pixels(
                bytes, byte_count, cursor,
                &rule->margin_top
            );
        } else if (doc_slice_equal_ascii(
                       bytes,
                       name_start,
                       name_end - name_start,
                       margin_bottom_name,
                       sizeof(margin_bottom_name))) {
            flag =
                RIVET_CSS_HAS_MARGIN_BOTTOM;
            if (rule->flags & flag) {
                return RIVET_ERR_DUPLICATE;
            }
            result = css_parse_pixels(
                bytes, byte_count, cursor,
                &rule->margin_bottom
            );
        } else {
            return RIVET_ERR_UNSUPPORTED;
        }

        if (result != RIVET_OK) {
            return result;
        }
        rule->flags |= flag;

        css_skip_space(
            bytes, byte_count, cursor
        );
        if (*cursor < byte_count &&
            bytes[*cursor] == 0x3bu) {
            ++(*cursor);
        } else if (*cursor >= byte_count ||
                   bytes[*cursor] != 0x7du) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
    }
}

static rivet_result css_run(
    const unsigned char *bytes,
    size_t byte_count,
    rivet_css_rule *rules,
    size_t capacity,
    int emit,
    size_t *count
)
{
    size_t cursor = 0u;
    size_t used = 0u;

    for (;;) {
        rivet_css_rule rule;
        rivet_result result;

        css_skip_space(
            bytes, byte_count, &cursor
        );
        if (cursor == byte_count) {
            break;
        }

        result = css_parse_one(
            bytes,
            byte_count,
            &cursor,
            &rule
        );
        if (result != RIVET_OK) {
            return result;
        }

        if (used == (size_t)-1) {
            return RIVET_ERR_CAPACITY;
        }

        if (emit) {
            if (used >= capacity ||
                rules == NULL) {
                return RIVET_ERR_CAPACITY;
            }
            rules[used] = rule;
        }
        ++used;
    }

    *count = used;
    return RIVET_OK;
}

rivet_result rivet_css_parse(
    const unsigned char *bytes,
    size_t byte_count,
    rivet_css_rule *rules,
    size_t capacity,
    size_t *rule_count
)
{
    size_t needed = 0u;
    size_t written = 0u;
    size_t i;
    rivet_result result;

    if ((byte_count != 0u &&
         bytes == NULL) ||
        rule_count == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < byte_count; ++i) {
        unsigned char byte = bytes[i];
        if (!(doc_space(byte) ||
              (byte >= 0x21u &&
               byte <= 0x7eu))) {
            return RIVET_ERR_UNSUPPORTED;
        }
    }

    result = css_run(
        bytes,
        byte_count,
        NULL,
        0u,
        0,
        &needed
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (needed > capacity ||
        (needed != 0u && rules == NULL)) {
        return RIVET_ERR_CAPACITY;
    }

    result = css_run(
        bytes,
        byte_count,
        rules,
        capacity,
        1,
        &written
    );
    if (result != RIVET_OK) {
        return result;
    }

    *rule_count = written;
    return RIVET_OK;
}
