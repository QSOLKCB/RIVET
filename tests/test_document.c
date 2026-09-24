/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/document.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static int test_stream(void)
{
    static const unsigned char bytes[] = {0x41u,0x42u,0x43u};
    rivet_byte_stream stream;
    unsigned char out[2] = {0u,0u};
    unsigned char peek = 0u;
    size_t count = 99u;

    CHECK(rivet_byte_stream_init(
        &stream, bytes, sizeof(bytes)) == RIVET_OK);
    CHECK(rivet_byte_stream_remaining(&stream) == 3u);
    CHECK(rivet_byte_stream_peek(&stream, &peek) == RIVET_OK);
    CHECK(peek == 0x41u);

    CHECK(rivet_byte_stream_read(
        &stream, out, sizeof(out), 2u, &count) == RIVET_OK);
    CHECK(count == 2u);
    CHECK(out[0] == 0x41u && out[1] == 0x42u);
    CHECK(rivet_byte_stream_remaining(&stream) == 1u);

    count = 99u;
    CHECK(rivet_byte_stream_read(
        &stream, out, sizeof(out), 2u, &count) == RIVET_OK);
    CHECK(count == 1u && out[0] == 0x43u);
    CHECK(rivet_byte_stream_remaining(&stream) == 0u);

    peek = 0x55u;
    CHECK(rivet_byte_stream_peek(
        &stream, &peek) == RIVET_ERR_NOT_FOUND);
    CHECK(peek == 0x55u);

    count = 77u;
    CHECK(rivet_byte_stream_read(
        &stream, NULL, 0u, 0u, &count) == RIVET_OK);
    CHECK(count == 0u);
    return 0;
}

static int test_url(void)
{
    static const unsigned char valid[] =
        "https://example.com:8443/a%20b?q=1#top";
    static const unsigned char invalid_scheme[] =
        "ftp://example.com/";
    static const unsigned char invalid_percent[] =
        "https://example.com/%zz";
    static const unsigned char invalid_port[] =
        "https://example.com:70000/";
    rivet_url url;

    CHECK(rivet_url_parse(
        &url, valid, sizeof(valid) - 1u) == RIVET_OK);
    CHECK(url.secure == 1);
    CHECK(url.has_port == 1);
    CHECK(url.port == 8443u);
    CHECK(url.host.length == 11u);
    CHECK(url.path.length == 6u);
    CHECK(url.query.length == 3u);
    CHECK(url.fragment.length == 3u);

    CHECK(rivet_url_parse(
        &url,
        invalid_scheme,
        sizeof(invalid_scheme) - 1u) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_url_parse(
        &url,
        invalid_percent,
        sizeof(invalid_percent) - 1u) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_url_parse(
        &url,
        invalid_port,
        sizeof(invalid_port) - 1u) == RIVET_ERR_UNSUPPORTED);
    return 0;
}

static int test_utf8(void)
{
    static const unsigned char valid[] = {
        0x41u,0xc3u,0xa9u,0xf0u,0x9fu,0x98u,0x80u
    };
    static const unsigned char overlong[] = {0xc0u,0xafu};
    static const unsigned char truncated[] = {0xe2u,0x82u};
    static const unsigned char surrogate[] = {0xedu,0xa0u,0x80u};
    unsigned int codepoints[3] = {0u,0u,0u};
    size_t count = 99u;

    CHECK(rivet_utf8_decode(
        valid,
        sizeof(valid),
        codepoints,
        3u,
        &count) == RIVET_OK);
    CHECK(count == 3u);
    CHECK(codepoints[0] == 0x41u);
    CHECK(codepoints[1] == 0xe9u);
    CHECK(codepoints[2] == 0x1f600u);

    count = 88u;
    CHECK(rivet_utf8_decode(
        valid,
        sizeof(valid),
        codepoints,
        2u,
        &count) == RIVET_ERR_CAPACITY);
    CHECK(count == 88u);

    CHECK(rivet_utf8_decode(
        overlong,
        sizeof(overlong),
        codepoints,
        3u,
        &count) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_utf8_decode(
        truncated,
        sizeof(truncated),
        codepoints,
        3u,
        &count) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_utf8_decode(
        surrogate,
        sizeof(surrogate),
        codepoints,
        3u,
        &count) == RIVET_ERR_UNSUPPORTED);
    return 0;
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

static int test_html_css_layout(void)
{
    static const unsigned char source[] =
        "<!doctype html><html><head><style>"
        "body{color:#202020;}"
        "p{margin-bottom:4px;}"
        "a{color:#3366ff;}"
        "</style></head><body>"
        "<p>A <a href=\"https://example.com/a\">B</a> C</p>"
        "<img src=\"x.ppm\" width=\"2\" height=\"2\">"
        "<form action=\"https://example.com/f\">"
        "<input name=\"q\" value=\"v\">"
        "</form></body></html>";
    static const unsigned char malformed[] =
        "<html><body><p>A</body></html>";
    static const unsigned char unknown[] =
        "<html><body><div>A</div></body></html>";
    static const unsigned char entity[] =
        "<html><body><p>A&amp;B</p></body></html>";
    static const unsigned char bad_image[] =
        "<html><body><img src=\"x.ppm\"></body></html>";
    rivet_doc_node nodes[32];
    rivet_document document;
    rivet_css_rule rules[8];
    rivet_layout_box boxes[64];
    const char *capabilities[5];
    size_t capability_count = 0u;
    size_t style_index = 0u;
    size_t link_index = 0u;
    size_t rule_count = 0u;
    size_t box_count = 0u;
    unsigned long document_height = 0ul;
    rivet_url link_url;
    int saw_image = 0;
    int saw_input = 0;
    size_t i;

    CHECK(rivet_html_parse(
        &document,
        source,
        sizeof(source) - 1u,
        nodes,
        32u) == RIVET_OK);
    CHECK(document.node_count > 8u);
    CHECK((document.requirements & RIVET_DOC_REQUIRE_HTML) != 0u);
    CHECK((document.requirements & RIVET_DOC_REQUIRE_CSS) != 0u);
    CHECK((document.requirements & RIVET_DOC_REQUIRE_LINKS) != 0u);
    CHECK((document.requirements & RIVET_DOC_REQUIRE_FORMS) != 0u);
    CHECK((document.requirements & RIVET_DOC_REQUIRE_IMAGE_PPM) != 0u);

    CHECK(rivet_document_required_capabilities(
        &document,
        capabilities,
        5u,
        &capability_count) == RIVET_OK);
    CHECK(capability_count == 5u);
    CHECK(strcmp(capabilities[0], "document.html") == 0);
    CHECK(strcmp(capabilities[1], "document.css") == 0);
    CHECK(strcmp(capabilities[2], "document.links") == 0);
    CHECK(strcmp(capabilities[3], "document.forms") == 0);
    CHECK(strcmp(capabilities[4], "image.ppm") == 0);

    CHECK(find_node(
        &document,
        RIVET_DOC_NODE_STYLE,
        &style_index));
    CHECK(find_node(
        &document,
        RIVET_DOC_NODE_A,
        &link_index));

    {
        size_t css_text = style_index + 1u;
        CHECK(css_text < document.node_count);
        CHECK(document.nodes[css_text].kind == RIVET_DOC_NODE_TEXT);
        CHECK(document.nodes[css_text].parent == style_index);
        CHECK(rivet_css_parse(
            document.source + document.nodes[css_text].text.offset,
            document.nodes[css_text].text.length,
            rules,
            8u,
            &rule_count) == RIVET_OK);
        CHECK(rule_count == 3u);
    }

    CHECK(rivet_url_parse(
        &link_url,
        document.source + document.nodes[link_index].href.offset,
        document.nodes[link_index].href.length) == RIVET_OK);
    CHECK(link_url.secure == 1);
    CHECK(link_url.port == 443u);

    CHECK(rivet_document_layout(
        &document,
        rules,
        rule_count,
        96ul,
        boxes,
        64u,
        &box_count,
        &document_height) == RIVET_OK);
    CHECK(box_count > 3u);
    CHECK(document_height > 0ul);

    for (i = 0u; i < box_count; ++i) {
        if (boxes[i].kind == RIVET_LAYOUT_IMAGE) {
            saw_image = 1;
            CHECK(boxes[i].width == 2ul);
            CHECK(boxes[i].height == 2ul);
        }
        if (boxes[i].kind == RIVET_LAYOUT_INPUT) {
            saw_input = 1;
            CHECK(boxes[i].width == 80ul);
            CHECK(boxes[i].height == 12ul);
        }
    }
    CHECK(saw_image);
    CHECK(saw_input);

    CHECK(rivet_html_parse(
        &document,
        source,
        sizeof(source) - 1u,
        nodes,
        4u) == RIVET_ERR_CAPACITY);
    CHECK(rivet_html_parse(
        &document,
        malformed,
        sizeof(malformed) - 1u,
        nodes,
        32u) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_html_parse(
        &document,
        unknown,
        sizeof(unknown) - 1u,
        nodes,
        32u) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_html_parse(
        &document,
        entity,
        sizeof(entity) - 1u,
        nodes,
        32u) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_html_parse(
        &document,
        bad_image,
        sizeof(bad_image) - 1u,
        nodes,
        32u) == RIVET_ERR_UNSUPPORTED);

    return 0;
}

static int test_strict_structure_and_layout_state(void)
{
    static const unsigned char inline_source[] =
        "<!doctype html><html><body>"
        "<p>HELLO <a href=\"\">LINK</a> WORLD</p>"
        "</body></html>";
    static const unsigned char css_source[] =
        "body{background-color:#010203;}";
    static const unsigned char nested_body[] =
        "<html><head><body><p>VISIBLE</p>"
        "</body></head></html>";
    static const unsigned char nested_doctype[] =
        "<html><body><!doctype html></body></html>";
    static const unsigned char duplicate_doctype[] =
        "<!doctype html><!doctype html>"
        "<html><body></body></html>";
    static const unsigned char trailing_doctype[] =
        "<html><body></body></html><!doctype html>";
    static const unsigned char duplicate_empty_href[] =
        "<html><body><a href=\"\" href=\"\">X</a>"
        "</body></html>";
    static const unsigned char whitespace_between_links[] =
        "<html><body><p><a>A</a> <a>B</a></p>"
        "</body></html>";
    static const unsigned char bad_attribute_separator[] =
        "<html><body><p id=\"a\"title=\"b\">X</p>"
        "</body></html>";
    static const unsigned char visible_head[] =
        "<html><head><p>VISIBLE</p></head>"
        "<body></body></html>";
    static const unsigned char narrow_image[] =
        "<html><body><img src=\"x\" width=\"2\" height=\"2\">"
        "</body></html>";
    rivet_doc_node nodes[32];
    rivet_document document;
    rivet_css_rule rules[1];
    rivet_layout_box boxes[16];
    size_t rule_count = 0u;
    size_t box_count = 0u;
    unsigned long document_height = 0ul;
    size_t link_index = 0u;
    size_t link_text_index = 0u;
    size_t second_link_text_index = 0u;
    size_t i;
    int saw_link_text = 0;
    int saw_space_node = 0;
    int saw_second_link = 0;

    CHECK(rivet_html_parse(
        &document,
        inline_source,
        sizeof(inline_source) - 1u,
        nodes,
        32u) == RIVET_OK);
    CHECK((document.requirements &
           RIVET_DOC_REQUIRE_LINKS) != 0u);
    CHECK(find_node(
        &document,
        RIVET_DOC_NODE_A,
        &link_index));
    CHECK(link_index + 1u <
          document.node_count);
    link_text_index = link_index + 1u;
    CHECK(document.nodes[
        link_text_index].kind ==
        RIVET_DOC_NODE_TEXT);
    CHECK(document.nodes[
        link_text_index].parent ==
        link_index);

    CHECK(rivet_css_parse(
        css_source,
        sizeof(css_source) - 1u,
        rules,
        1u,
        &rule_count) == RIVET_OK);
    CHECK(rule_count == 1u);

    CHECK(rivet_document_layout(
        &document,
        rules,
        rule_count,
        120ul,
        boxes,
        16u,
        &box_count,
        &document_height) == RIVET_OK);
    CHECK(document_height > 0ul);

    for (i = 0u; i < box_count; ++i) {
        if (boxes[i].node_index ==
                link_text_index &&
            boxes[i].kind ==
                RIVET_LAYOUT_TEXT) {
            saw_link_text = 1;
            CHECK(boxes[i].x == 36ul);
            CHECK(boxes[i].has_background);
            CHECK(boxes[i].background.r == 0x01u);
            CHECK(boxes[i].background.g == 0x02u);
            CHECK(boxes[i].background.b == 0x03u);
            CHECK(boxes[i].background.a == 0xffu);
        }
    }
    CHECK(saw_link_text);

    CHECK(rivet_html_parse(
        &document,
        nested_body,
        sizeof(nested_body) - 1u,
        nodes,
        32u) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_html_parse(
        &document,
        nested_doctype,
        sizeof(nested_doctype) - 1u,
        nodes,
        32u) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_html_parse(
        &document,
        duplicate_doctype,
        sizeof(duplicate_doctype) - 1u,
        nodes,
        32u) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_html_parse(
        &document,
        trailing_doctype,
        sizeof(trailing_doctype) - 1u,
        nodes,
        32u) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_html_parse(
        &document,
        duplicate_empty_href,
        sizeof(duplicate_empty_href) - 1u,
        nodes,
        32u) == RIVET_ERR_DUPLICATE);

    CHECK(rivet_html_parse(
        &document,
        bad_attribute_separator,
        sizeof(bad_attribute_separator) - 1u,
        nodes,
        32u) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_html_parse(
        &document,
        visible_head,
        sizeof(visible_head) - 1u,
        nodes,
        32u) == RIVET_ERR_INVALID_ARGUMENT);

    CHECK(rivet_html_parse(
        &document,
        whitespace_between_links,
        sizeof(whitespace_between_links) - 1u,
        nodes,
        32u) == RIVET_OK);
    second_link_text_index = document.node_count;
    for (i = 0u; i < document.node_count; ++i) {
        if (document.nodes[i].kind !=
            RIVET_DOC_NODE_TEXT) {
            continue;
        }
        if (document.nodes[i].text.length == 1u &&
            document.source[
                document.nodes[i].text.offset] == 0x20u) {
            saw_space_node = 1;
        }
        if (document.nodes[i].text.length == 1u &&
            document.source[
                document.nodes[i].text.offset] == 0x42u) {
            second_link_text_index = i;
        }
    }
    CHECK(saw_space_node);
    CHECK(second_link_text_index <
          document.node_count);
    CHECK(rivet_document_layout(
        &document,
        NULL,
        0u,
        24ul,
        boxes,
        16u,
        &box_count,
        &document_height) == RIVET_OK);
    for (i = 0u; i < box_count; ++i) {
        if (boxes[i].node_index ==
                second_link_text_index &&
            boxes[i].kind ==
                RIVET_LAYOUT_TEXT) {
            saw_second_link = 1;
            CHECK(boxes[i].x == 12ul);
        }
    }
    CHECK(saw_second_link);

    CHECK(rivet_html_parse(
        &document,
        narrow_image,
        sizeof(narrow_image) - 1u,
        nodes,
        32u) == RIVET_OK);
    CHECK(rivet_document_layout(
        &document,
        NULL,
        0u,
        2ul,
        boxes,
        16u,
        &box_count,
        &document_height) == RIVET_OK);
    CHECK(box_count == 1u);
    CHECK(boxes[0].kind == RIVET_LAYOUT_IMAGE);
    CHECK(boxes[0].x == 0ul);
    CHECK(boxes[0].width == 2ul);
    CHECK(boxes[0].height == 2ul);

    return 0;
}

static int test_depth_limit(void)
{
    unsigned char bytes[512];
    size_t cursor = 0u;
    size_t i;
    rivet_doc_node nodes[64];
    rivet_document document;
    rivet_layout_box boxes[4];
    size_t box_count = 0u;
    unsigned long document_height = 0ul;
    static const unsigned char open_html[] = "<html><body>";
    static const unsigned char close_html[] = "</body></html>";
    static const unsigned char open_p[] = "<p>";
    static const unsigned char close_p[] = "</p>";

    memcpy(bytes + cursor, open_html, sizeof(open_html) - 1u);
    cursor += sizeof(open_html) - 1u;

    for (i = 0u; i < 30u; ++i) {
        memcpy(bytes + cursor, open_p, sizeof(open_p) - 1u);
        cursor += sizeof(open_p) - 1u;
    }

    bytes[cursor++] = 0x41u;

    for (i = 0u; i < 30u; ++i) {
        memcpy(bytes + cursor, close_p, sizeof(close_p) - 1u);
        cursor += sizeof(close_p) - 1u;
    }

    memcpy(bytes + cursor, close_html, sizeof(close_html) - 1u);
    cursor += sizeof(close_html) - 1u;

    CHECK(rivet_html_parse(
        &document,
        bytes,
        cursor,
        nodes,
        64u) == RIVET_OK);
    CHECK(rivet_document_layout(
        &document,
        NULL,
        0u,
        6ul,
        boxes,
        4u,
        &box_count,
        &document_height) == RIVET_OK);
    CHECK(box_count == 1u);
    CHECK(boxes[0].kind == RIVET_LAYOUT_TEXT);
    CHECK(boxes[0].width == 6ul);

    cursor = 0u;
    memcpy(bytes + cursor, open_html, sizeof(open_html) - 1u);
    cursor += sizeof(open_html) - 1u;

    for (i = 0u; i < 31u; ++i) {
        memcpy(bytes + cursor, open_p, sizeof(open_p) - 1u);
        cursor += sizeof(open_p) - 1u;
    }

    bytes[cursor++] = 0x41u;

    for (i = 0u; i < 31u; ++i) {
        memcpy(bytes + cursor, close_p, sizeof(close_p) - 1u);
        cursor += sizeof(close_p) - 1u;
    }

    memcpy(bytes + cursor, close_html, sizeof(close_html) - 1u);
    cursor += sizeof(close_html) - 1u;

    CHECK(rivet_html_parse(
        &document,
        bytes,
        cursor,
        nodes,
        64u) == RIVET_ERR_CAPACITY);
    return 0;
}

static int test_css_failures(void)
{
    static const unsigned char unknown[] =
        "p{font-size:12px;}";
    static const unsigned char duplicate[] =
        "p{color:#000000;color:#ffffff;}";
    static const unsigned char large[] =
        "p{margin-top:1025px;}";
    static const unsigned char valid[] =
        "p{color:#010203;}a{margin-top:2px;}";
    rivet_css_rule rules[2];
    size_t count = 99u;

    CHECK(rivet_css_parse(
        unknown,
        sizeof(unknown) - 1u,
        rules,
        2u,
        &count) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_css_parse(
        duplicate,
        sizeof(duplicate) - 1u,
        rules,
        2u,
        &count) == RIVET_ERR_DUPLICATE);
    CHECK(rivet_css_parse(
        large,
        sizeof(large) - 1u,
        rules,
        2u,
        &count) == RIVET_ERR_CAPACITY);
    CHECK(rivet_css_parse(
        valid,
        sizeof(valid) - 1u,
        rules,
        1u,
        &count) == RIVET_ERR_CAPACITY);
    return 0;
}

static int test_image(void)
{
    static const unsigned char ppm[] = {
        0x50u,0x36u,0x0au,
        0x32u,0x20u,0x32u,0x0au,
        0x32u,0x35u,0x35u,0x0au,
        0xffu,0x00u,0x00u,
        0x00u,0xffu,0x00u,
        0x00u,0x00u,0xffu,
        0xffu,0xffu,0xffu
    };
    static const unsigned char bad_magic[] = {
        0x50u,0x33u,0x0au,0x31u,0x20u,0x31u,0x0au,
        0x32u,0x35u,0x35u,0x0au,0x30u
    };
    static const unsigned char bad_adjacent_magic[] = {
        0x50u,0x36u,0x31u,0x20u,0x31u,0x20u,
        0x32u,0x35u,0x35u,0x0au,
        0x00u,0x00u,0x00u
    };
    static const unsigned char ppm_cr_comment[] = {
        0x50u,0x36u,0x0du,
        0x23u,0x63u,0x0du,
        0x31u,0x20u,0x31u,0x0du,
        0x32u,0x35u,0x35u,0x0du,
        0x12u,0x34u,0x56u
    };
    unsigned char pixels[16];
    rivet_document_image image;

    CHECK(rivet_image_decode_ppm(
        &image,
        ppm,
        sizeof(ppm),
        pixels,
        sizeof(pixels)) == RIVET_OK);
    CHECK(image.width == 2ul);
    CHECK(image.height == 2ul);
    CHECK(image.stride_bytes == 8u);
    CHECK(image.pixel_bytes == 16u);
    CHECK(pixels[0] == 0xffu &&
          pixels[1] == 0x00u &&
          pixels[2] == 0x00u &&
          pixels[3] == 0xffu);
    CHECK(pixels[12] == 0xffu &&
          pixels[13] == 0xffu &&
          pixels[14] == 0xffu &&
          pixels[15] == 0xffu);

    CHECK(rivet_image_decode_ppm(
        &image,
        ppm,
        sizeof(ppm),
        pixels,
        15u) == RIVET_ERR_CAPACITY);
    CHECK(rivet_image_decode_ppm(
        &image,
        ppm,
        sizeof(ppm) - 1u,
        pixels,
        sizeof(pixels)) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_image_decode_ppm(
        &image,
        bad_magic,
        sizeof(bad_magic),
        pixels,
        sizeof(pixels)) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_image_decode_ppm(
        &image,
        bad_adjacent_magic,
        sizeof(bad_adjacent_magic),
        pixels,
        sizeof(pixels)) == RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_image_decode_ppm(
        &image,
        ppm_cr_comment,
        sizeof(ppm_cr_comment),
        pixels,
        sizeof(pixels)) == RIVET_OK);
    CHECK(image.width == 1ul);
    CHECK(image.height == 1ul);
    CHECK(image.pixel_bytes == 4u);
    CHECK(pixels[0] == 0x12u);
    CHECK(pixels[1] == 0x34u);
    CHECK(pixels[2] == 0x56u);
    CHECK(pixels[3] == 0xffu);
    return 0;
}

int main(void)
{
    CHECK(RIVET_DOCUMENT_ABI_VERSION == 1u);
    CHECK(test_stream() == 0);
    CHECK(test_url() == 0);
    CHECK(test_utf8() == 0);
    CHECK(test_html_css_layout() == 0);
    CHECK(test_strict_structure_and_layout_state() == 0);
    CHECK(test_depth_limit() == 0);
    CHECK(test_css_failures() == 0);
    CHECK(test_image() == 0);

    puts("rivet document tests: ok");
    return 0;
}
