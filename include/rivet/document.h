/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_DOCUMENT_H
#define RIVET_DOCUMENT_H

#include <stddef.h>
#include <stdint.h>

#include "rivet/rivet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RIVET_DOCUMENT_ABI_VERSION 1u
#define RIVET_DOCUMENT_MAX_DEPTH 32u
#define RIVET_DOCUMENT_IMAGE_PIXEL_BYTES 4u
#define RIVET_DOCUMENT_NO_PARENT ((size_t)-1)

typedef struct rivet_doc_slice {
    size_t offset;
    size_t length;
} rivet_doc_slice;

typedef struct rivet_byte_stream {
    const unsigned char *bytes;
    size_t length;
    size_t offset;
} rivet_byte_stream;

rivet_result rivet_byte_stream_init(
    rivet_byte_stream *stream,
    const unsigned char *bytes,
    size_t length
);

rivet_result rivet_byte_stream_read(
    rivet_byte_stream *stream,
    unsigned char *buffer,
    size_t capacity,
    size_t requested,
    size_t *read_count
);

rivet_result rivet_byte_stream_peek(
    const rivet_byte_stream *stream,
    unsigned char *byte
);

size_t rivet_byte_stream_remaining(
    const rivet_byte_stream *stream
);

typedef struct rivet_url {
    const unsigned char *bytes;
    size_t byte_count;
    rivet_doc_slice scheme;
    rivet_doc_slice host;
    rivet_doc_slice path;
    rivet_doc_slice query;
    rivet_doc_slice fragment;
    unsigned int port;
    int has_port;
    int secure;
} rivet_url;

rivet_result rivet_url_parse(
    rivet_url *url,
    const unsigned char *bytes,
    size_t byte_count
);

rivet_result rivet_utf8_decode(
    const unsigned char *bytes,
    size_t byte_count,
    unsigned int *codepoints,
    size_t capacity,
    size_t *codepoint_count
);

typedef uint32_t rivet_doc_node_kind;

#define RIVET_DOC_NODE_HTML ((rivet_doc_node_kind)1u)
#define RIVET_DOC_NODE_HEAD ((rivet_doc_node_kind)2u)
#define RIVET_DOC_NODE_BODY ((rivet_doc_node_kind)3u)
#define RIVET_DOC_NODE_P ((rivet_doc_node_kind)4u)
#define RIVET_DOC_NODE_H1 ((rivet_doc_node_kind)5u)
#define RIVET_DOC_NODE_H2 ((rivet_doc_node_kind)6u)
#define RIVET_DOC_NODE_A ((rivet_doc_node_kind)7u)
#define RIVET_DOC_NODE_FORM ((rivet_doc_node_kind)8u)
#define RIVET_DOC_NODE_INPUT ((rivet_doc_node_kind)9u)
#define RIVET_DOC_NODE_IMG ((rivet_doc_node_kind)10u)
#define RIVET_DOC_NODE_BR ((rivet_doc_node_kind)11u)
#define RIVET_DOC_NODE_STYLE ((rivet_doc_node_kind)12u)
#define RIVET_DOC_NODE_TEXT ((rivet_doc_node_kind)13u)

typedef struct rivet_doc_node {
    rivet_doc_node_kind kind;
    size_t parent;
    rivet_doc_slice text;
    rivet_doc_slice href;
    rivet_doc_slice action;
    rivet_doc_slice src;
    rivet_doc_slice name;
    rivet_doc_slice value;
    unsigned long width;
    unsigned long height;
} rivet_doc_node;

enum {
    RIVET_DOC_REQUIRE_HTML = 1u << 0,
    RIVET_DOC_REQUIRE_CSS = 1u << 1,
    RIVET_DOC_REQUIRE_LINKS = 1u << 2,
    RIVET_DOC_REQUIRE_FORMS = 1u << 3,
    RIVET_DOC_REQUIRE_IMAGE_PPM = 1u << 4
};

typedef struct rivet_document {
    const unsigned char *source;
    size_t source_bytes;
    rivet_doc_node *nodes;
    size_t node_capacity;
    size_t node_count;
    unsigned int requirements;
} rivet_document;

rivet_result rivet_html_parse(
    rivet_document *document,
    const unsigned char *bytes,
    size_t byte_count,
    rivet_doc_node *nodes,
    size_t node_capacity
);

rivet_result rivet_document_required_capabilities(
    const rivet_document *document,
    const char **ids,
    size_t capacity,
    size_t *count
);

typedef struct rivet_doc_color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} rivet_doc_color;

enum {
    RIVET_CSS_HAS_COLOR = 1u << 0,
    RIVET_CSS_HAS_BACKGROUND = 1u << 1,
    RIVET_CSS_HAS_MARGIN_TOP = 1u << 2,
    RIVET_CSS_HAS_MARGIN_BOTTOM = 1u << 3
};

typedef struct rivet_css_rule {
    rivet_doc_node_kind selector;
    unsigned int flags;
    rivet_doc_color color;
    rivet_doc_color background;
    unsigned long margin_top;
    unsigned long margin_bottom;
} rivet_css_rule;

rivet_result rivet_css_parse(
    const unsigned char *bytes,
    size_t byte_count,
    rivet_css_rule *rules,
    size_t capacity,
    size_t *rule_count
);

typedef uint32_t rivet_layout_box_kind;

#define RIVET_LAYOUT_TEXT ((rivet_layout_box_kind)1u)
#define RIVET_LAYOUT_IMAGE ((rivet_layout_box_kind)2u)
#define RIVET_LAYOUT_INPUT ((rivet_layout_box_kind)3u)

typedef struct rivet_layout_box {
    rivet_layout_box_kind kind;
    size_t node_index;
    rivet_doc_slice source;
    unsigned long x;
    unsigned long y;
    unsigned long width;
    unsigned long height;
    rivet_doc_color foreground;
    rivet_doc_color background;
    int has_background;
} rivet_layout_box;

rivet_result rivet_document_layout(
    const rivet_document *document,
    const rivet_css_rule *rules,
    size_t rule_count,
    unsigned long viewport_width,
    rivet_layout_box *boxes,
    size_t box_capacity,
    size_t *box_count,
    unsigned long *document_height
);

typedef struct rivet_document_image {
    unsigned long width;
    unsigned long height;
    size_t stride_bytes;
    unsigned char *pixels;
    size_t pixel_bytes;
} rivet_document_image;

rivet_result rivet_image_decode_ppm(
    rivet_document_image *image,
    const unsigned char *bytes,
    size_t byte_count,
    unsigned char *pixels,
    size_t pixel_capacity
);

#ifdef __cplusplus
}
#endif

#endif
