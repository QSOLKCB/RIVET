/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/document.h"

#include <stdio.h>

#define HTML_CAPACITY 4096u
#define IMAGE_CAPACITY 256u
#define NODE_CAPACITY 64u
#define RULE_CAPACITY 16u
#define BOX_CAPACITY 128u
#define PIXEL_CAPACITY 256u

#define EXPECTED_HTML_BYTES 428u
#define EXPECTED_HTML_FNV1A64 0x3987ce4034e4f3fcULL
#define EXPECTED_NODE_COUNT 15u
#define EXPECTED_CSS_RULE_COUNT 5u
#define EXPECTED_CAPABILITY_COUNT 5u
#define EXPECTED_UTF8_CODEPOINT_COUNT 12u
#define EXPECTED_BOX_COUNT 6u
#define EXPECTED_DOCUMENT_HEIGHT 46ul
#define EXPECTED_LAYOUT_FNV1A64 0x0209c1501da9396cULL
#define EXPECTED_IMAGE_BYTES 23u
#define EXPECTED_IMAGE_FNV1A64 0x8a4318bc590ba10dULL

static unsigned long long fnv1a64(
    const unsigned char *bytes,
    size_t count
)
{
    unsigned long long hash =
        0xcbf29ce484222325ULL;
    size_t i;

    for (i = 0u; i < count; ++i) {
        hash ^=
            (unsigned long long)bytes[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

static void fnv_mix_byte(
    unsigned long long *hash,
    unsigned char value
)
{
    *hash ^= (unsigned long long)value;
    *hash *= 0x100000001b3ULL;
}

static void fnv_mix_u64(
    unsigned long long *hash,
    unsigned long long value
)
{
    unsigned int i;
    for (i = 0u; i < 8u; ++i) {
        fnv_mix_byte(
            hash,
            (unsigned char)(
                (value >> (i * 8u)) & 0xffu
            )
        );
    }
}

static unsigned long long layout_hash(
    const rivet_layout_box *boxes,
    size_t count
)
{
    unsigned long long hash =
        0xcbf29ce484222325ULL;
    size_t i;

    for (i = 0u; i < count; ++i) {
        const rivet_layout_box *box =
            &boxes[i];

        fnv_mix_u64(
            &hash,
            (unsigned long long)box->kind
        );
        fnv_mix_u64(
            &hash,
            (unsigned long long)box->node_index
        );
        fnv_mix_u64(
            &hash,
            (unsigned long long)box->source.offset
        );
        fnv_mix_u64(
            &hash,
            (unsigned long long)box->source.length
        );
        fnv_mix_u64(
            &hash,
            (unsigned long long)box->x
        );
        fnv_mix_u64(
            &hash,
            (unsigned long long)box->y
        );
        fnv_mix_u64(
            &hash,
            (unsigned long long)box->width
        );
        fnv_mix_u64(
            &hash,
            (unsigned long long)box->height
        );
        fnv_mix_byte(&hash, box->foreground.r);
        fnv_mix_byte(&hash, box->foreground.g);
        fnv_mix_byte(&hash, box->foreground.b);
        fnv_mix_byte(&hash, box->foreground.a);
        fnv_mix_byte(&hash, box->background.r);
        fnv_mix_byte(&hash, box->background.g);
        fnv_mix_byte(&hash, box->background.b);
        fnv_mix_byte(&hash, box->background.a);
        fnv_mix_byte(
            &hash,
            (unsigned char)(
                box->has_background ? 1u : 0u
            )
        );
    }

    return hash;
}

static int read_file(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *count
)
{
    FILE *file;
    size_t used = 0u;

    file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }

    for (;;) {
        size_t received;

        if (used == capacity) {
            unsigned char extra;
            if (fread(&extra, 1u, 1u, file) != 0u) {
                fclose(file);
                return 0;
            }
            break;
        }

        received = fread(
            buffer + used,
            1u,
            capacity - used,
            file
        );
        used += received;

        if (received == 0u) {
            break;
        }
    }

    if (ferror(file) || fclose(file) != 0) {
        return 0;
    }

    *count = used;
    return 1;
}

static int find_node(
    const rivet_document *document,
    rivet_doc_node_kind kind,
    size_t *index
)
{
    size_t i;
    for (i = 0u; i < document->node_count; ++i) {
        if (document->nodes[i].kind == kind) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    unsigned char html_bytes[HTML_CAPACITY];
    unsigned char image_bytes[IMAGE_CAPACITY];
    unsigned char stream_buffer[64];
    unsigned char pixels[PIXEL_CAPACITY];
    size_t html_count = 0u;
    size_t image_count = 0u;
    rivet_byte_stream stream;
    size_t stream_total = 0u;
    unsigned long long stream_hash =
        0xcbf29ce484222325ULL;
    unsigned long long direct_html_hash;
    rivet_doc_node nodes[NODE_CAPACITY];
    rivet_document document;
    rivet_css_rule rules[RULE_CAPACITY];
    rivet_layout_box boxes[BOX_CAPACITY];
    size_t style_index = 0u;
    size_t link_index = 0u;
    size_t rule_count = 0u;
    size_t box_count = 0u;
    unsigned long document_height = 0ul;
    const char *required[8];
    size_t required_count = 0u;
    rivet_url link_url;
    rivet_document_image image;
    unsigned long long boxes_hash;
    unsigned long long image_hash;
    static const unsigned char utf8_sample[] = {
        0x52u,0x49u,0x56u,0x45u,0x54u,0x20u,
        0xe2u,0x80u,0x93u,0x20u,
        0x63u,0x61u,0x66u,0xc3u,0xa9u
    };
    unsigned int codepoints[16];
    size_t codepoint_count = 0u;
    const char *html_path =
        argc > 1 ?
        argv[1] :
        "fixtures/r7_document.html";
    const char *image_path =
        argc > 2 ?
        argv[2] :
        "fixtures/r7_image.ppm";

    if (!read_file(
            html_path,
            html_bytes,
            sizeof(html_bytes),
            &html_count) ||
        !read_file(
            image_path,
            image_bytes,
            sizeof(image_bytes),
            &image_count)) {
        return 1;
    }

    direct_html_hash =
        fnv1a64(html_bytes, html_count);

    if (rivet_byte_stream_init(
            &stream,
            html_bytes,
            html_count) != RIVET_OK) {
        return 1;
    }

    while (rivet_byte_stream_remaining(
               &stream) != 0u) {
        size_t read_count = 0u;
        size_t requested =
            rivet_byte_stream_remaining(
                &stream
            );
        size_t i;

        if (requested >
            sizeof(stream_buffer)) {
            requested =
                sizeof(stream_buffer);
        }

        if (rivet_byte_stream_read(
                &stream,
                stream_buffer,
                sizeof(stream_buffer),
                requested,
                &read_count) != RIVET_OK ||
            read_count == 0u) {
            return 1;
        }

        for (i = 0u; i < read_count; ++i) {
            fnv_mix_byte(
                &stream_hash,
                stream_buffer[i]
            );
        }
        stream_total += read_count;
    }

    if (stream_total != html_count ||
        stream_hash != direct_html_hash) {
        return 1;
    }

    if (rivet_utf8_decode(
            utf8_sample,
            sizeof(utf8_sample),
            codepoints,
            sizeof(codepoints) /
                sizeof(codepoints[0]),
            &codepoint_count) != RIVET_OK ||
        codepoint_count != EXPECTED_UTF8_CODEPOINT_COUNT ||
        codepoints[6] != 0x2013u ||
        codepoints[11] != 0xe9u) {
        return 1;
    }

    if (rivet_html_parse(
            &document,
            html_bytes,
            html_count,
            nodes,
            NODE_CAPACITY) != RIVET_OK ||
        !find_node(
            &document,
            RIVET_DOC_NODE_STYLE,
            &style_index) ||
        !find_node(
            &document,
            RIVET_DOC_NODE_A,
            &link_index)) {
        return 1;
    }

    if (style_index + 1u >=
            document.node_count ||
        document.nodes[
            style_index + 1u].kind !=
            RIVET_DOC_NODE_TEXT ||
        document.nodes[
            style_index + 1u].parent !=
            style_index) {
        return 1;
    }

    if (rivet_css_parse(
            document.source +
                document.nodes[
                    style_index + 1u].text.offset,
            document.nodes[
                style_index + 1u].text.length,
            rules,
            RULE_CAPACITY,
            &rule_count) != RIVET_OK) {
        return 1;
    }

    if (rivet_document_required_capabilities(
            &document,
            required,
            sizeof(required) /
                sizeof(required[0]),
            &required_count) != RIVET_OK ||
        required_count != EXPECTED_CAPABILITY_COUNT) {
        return 1;
    }

    if (rivet_url_parse(
            &link_url,
            document.source +
                document.nodes[
                    link_index].href.offset,
            document.nodes[
                link_index].href.length) !=
            RIVET_OK ||
        !link_url.secure ||
        link_url.port != 443u) {
        return 1;
    }

    if (rivet_document_layout(
            &document,
            rules,
            rule_count,
            120ul,
            boxes,
            BOX_CAPACITY,
            &box_count,
            &document_height) != RIVET_OK) {
        return 1;
    }

    boxes_hash =
        layout_hash(boxes, box_count);

    if (rivet_image_decode_ppm(
            &image,
            image_bytes,
            image_count,
            pixels,
            sizeof(pixels)) != RIVET_OK ||
        image.width != 2ul ||
        image.height != 2ul) {
        return 1;
    }

    image_hash =
        fnv1a64(
            pixels,
            image.pixel_bytes
        );

    printf(
        "rivet-r7: html_bytes=%lu html_fnv1a64=%016llx nodes=%lu css_rules=%lu capabilities=%lu utf8_codepoints=%lu boxes=%lu height=%lu layout_fnv1a64=%016llx image_bytes=%lu image_rgba_fnv1a64=%016llx link_secure=%d link_port=%u\n",
        (unsigned long)html_count,
        direct_html_hash,
        (unsigned long)document.node_count,
        (unsigned long)rule_count,
        (unsigned long)required_count,
        (unsigned long)codepoint_count,
        (unsigned long)box_count,
        document_height,
        boxes_hash,
        (unsigned long)image_count,
        image_hash,
        link_url.secure,
        link_url.port
    );

    if (html_count != EXPECTED_HTML_BYTES ||
        direct_html_hash != EXPECTED_HTML_FNV1A64 ||
        document.node_count != EXPECTED_NODE_COUNT ||
        rule_count != EXPECTED_CSS_RULE_COUNT ||
        required_count != EXPECTED_CAPABILITY_COUNT ||
        codepoint_count != EXPECTED_UTF8_CODEPOINT_COUNT ||
        box_count != EXPECTED_BOX_COUNT ||
        document_height != EXPECTED_DOCUMENT_HEIGHT ||
        boxes_hash != EXPECTED_LAYOUT_FNV1A64 ||
        image_count != EXPECTED_IMAGE_BYTES ||
        image_hash != EXPECTED_IMAGE_FNV1A64 ||
        link_url.secure != 1 ||
        link_url.port != 443u) {
        return 1;
    }

    return 0;
}
