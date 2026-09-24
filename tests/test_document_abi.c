/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/document.h"

#include <string.h>

#define CHECK(expr) do { if (!(expr)) return 1; } while (0)

int main(void)
{
    static const unsigned char source[] =
        "<html><body><p>X</p></body></html>";
    rivet_doc_node nodes[8];
    rivet_document document;
    rivet_css_rule rule;
    rivet_layout_box boxes[4];
    size_t box_count = 0u;
    unsigned long document_height = 0ul;

    CHECK(sizeof(rivet_doc_node_kind) == 4u);
    CHECK(sizeof(rivet_layout_box_kind) == 4u);

    CHECK(rivet_html_parse(
        &document,
        source,
        sizeof(source) - 1u,
        nodes,
        8u) == RIVET_OK);
    CHECK(document.node_count == 4u);
    CHECK(document.nodes[0].kind == RIVET_DOC_NODE_HTML);
    CHECK(document.nodes[1].kind == RIVET_DOC_NODE_BODY);
    CHECK(document.nodes[2].kind == RIVET_DOC_NODE_P);
    CHECK(document.nodes[3].kind == RIVET_DOC_NODE_TEXT);

    memset(&rule, 0xa5, sizeof(rule));
    rule.selector = RIVET_DOC_NODE_P;
    rule.flags = RIVET_CSS_HAS_COLOR;
    rule.color.r = 0xffu;
    rule.color.g = 0x00u;
    rule.color.b = 0x00u;
    rule.color.a = 0xffu;

    memset(boxes, 0xa5, sizeof(boxes));
    CHECK(rivet_document_layout(
        &document,
        &rule,
        1u,
        12ul,
        boxes,
        4u,
        &box_count,
        &document_height) == RIVET_OK);
    CHECK(box_count == 1u);
    CHECK(document_height == 8ul);
    CHECK(boxes[0].kind == RIVET_LAYOUT_TEXT);
    CHECK(boxes[0].foreground.r == 0xffu);
    CHECK(boxes[0].foreground.g == 0x00u);
    CHECK(boxes[0].foreground.b == 0x00u);
    CHECK(boxes[0].foreground.a == 0xffu);

    return 0;
}
