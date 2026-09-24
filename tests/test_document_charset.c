/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/document.h"

#define CHECK(expr) do { if (!(expr)) return 1; } while (0)

int main(void)
{
    static const unsigned char url_bytes[] = {
        0x68u,0x74u,0x74u,0x70u,0x73u,0x3au,0x2fu,0x2fu,
        0x65u,0x78u,0x61u,0x6du,0x70u,0x6cu,0x65u,0x2eu,
        0x63u,0x6fu,0x6du,0x2fu
    };
    static const unsigned char html_bytes[] = {
        0x3cu,0x68u,0x74u,0x6du,0x6cu,0x3eu,
        0x3cu,0x62u,0x6fu,0x64u,0x79u,0x3eu,
        0x3cu,0x70u,0x3eu,0x41u,
        0x3cu,0x2fu,0x70u,0x3eu,
        0x3cu,0x2fu,0x62u,0x6fu,0x64u,0x79u,0x3eu,
        0x3cu,0x2fu,0x68u,0x74u,0x6du,0x6cu,0x3eu
    };
    static const unsigned char css_bytes[] = {
        0x70u,0x7bu,0x63u,0x6fu,0x6cu,0x6fu,0x72u,0x3au,
        0x23u,0x31u,0x31u,0x32u,0x32u,0x33u,0x33u,0x3bu,
        0x7du
    };
    rivet_url url;
    rivet_doc_node nodes[8];
    rivet_document document;
    rivet_css_rule rules[1];
    size_t count = 0u;

    CHECK(rivet_url_parse(
        &url,
        url_bytes,
        sizeof(url_bytes)) == RIVET_OK);
    CHECK(url.secure == 1);
    CHECK(url.port == 443u);

    CHECK(rivet_html_parse(
        &document,
        html_bytes,
        sizeof(html_bytes),
        nodes,
        8u) == RIVET_OK);
    CHECK(document.node_count == 4u);

    CHECK(rivet_css_parse(
        css_bytes,
        sizeof(css_bytes),
        rules,
        1u,
        &count) == RIVET_OK);
    CHECK(count == 1u);
    CHECK(rules[0].color.r == 0x11u);
    CHECK(rules[0].color.g == 0x22u);
    CHECK(rules[0].color.b == 0x33u);

    return 0;
}
