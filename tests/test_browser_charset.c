/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/browser.h"

#include <string.h>

#define CHECK(expr) do { if (!(expr)) return 1; } while (0)

static const unsigned char CONFIG_BYTES[] = {
    0x52u,0x49u,0x56u,0x45u,0x54u,0x2du,0x57u,0x45u,
    0x42u,0x31u,0x20u,0x31u,0x0au,0x68u,0x6fu,0x6du,
    0x65u,0x3du,0x68u,0x74u,0x74u,0x70u,0x73u,0x3au,
    0x2fu,0x2fu,0x73u,0x69u,0x74u,0x65u,0x2eu,0x74u,
    0x65u,0x73u,0x74u,0x2fu,0x68u,0x6fu,0x6du,0x65u,
    0x0au,0x64u,0x6fu,0x77u,0x6eu,0x6cu,0x6fu,0x61u,
    0x64u,0x73u,0x3du,0x30u,0x0au,0x75u,0x73u,0x65u,
    0x72u,0x2du,0x63u,0x73u,0x73u,0x3du,0x0au
};

static const unsigned char HOME_URL[] = {
    0x68u,0x74u,0x74u,0x70u,0x73u,0x3au,0x2fu,0x2fu,
    0x73u,0x69u,0x74u,0x65u,0x2eu,0x74u,0x65u,0x73u,
    0x74u,0x2fu,0x68u,0x6fu,0x6du,0x65u
};

static const unsigned char PAGE_BYTES[] = {
    0x3cu,0x68u,0x74u,0x6du,0x6cu,0x3eu,
    0x3cu,0x62u,0x6fu,0x64u,0x79u,0x3eu,
    0x3cu,0x2fu,0x62u,0x6fu,0x64u,0x79u,0x3eu,
    0x3cu,0x2fu,0x68u,0x74u,0x6du,0x6cu,0x3eu
};

static int bytes_equal(
    const unsigned char *left,
    size_t left_count,
    const unsigned char *right,
    size_t right_count
)
{
    return left_count == right_count &&
           memcmp(left, right, left_count) == 0;
}

static rivet_result charset_fetch(
    void *context,
    const unsigned char *url,
    size_t url_length,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    (void)context;

    if (url == NULL || buffer == NULL ||
        byte_count == NULL ||
        !bytes_equal(
            url,
            url_length,
            HOME_URL,
            sizeof(HOME_URL))) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (sizeof(PAGE_BYTES) > capacity) {
        return RIVET_ERR_CAPACITY;
    }

    memcpy(buffer, PAGE_BYTES, sizeof(PAGE_BYTES));
    *byte_count = sizeof(PAGE_BYTES);
    return RIVET_OK;
}

static int pixel_is(
    const unsigned char *pixels,
    size_t offset,
    rivet_rgba8 color
)
{
    return pixels[offset] == color.r &&
           pixels[offset + 1u] == color.g &&
           pixels[offset + 2u] == color.b &&
           pixels[offset + 3u] == color.a;
}

int main(void)
{
    unsigned char document_bytes[256];
    unsigned char scratch_bytes[64];
    rivet_doc_node nodes[16];
    rivet_css_rule rules[4];
    rivet_layout_box boxes[16];
    rivet_browser_url history[2];
    rivet_browser_url bookmarks[2];
    rivet_browser_storage storage;
    rivet_browser_config config;
    rivet_browser_io io;
    rivet_browser browser;
    unsigned char pixels[40u * 20u * RIVET_GFX_PIXEL_BYTES];
    rivet_surface surface;
    rivet_rect bounds = {0L,0L,40ul,20ul};
    rivet_browser_style style = {
        {0x01u,0x02u,0x03u,0xffu},
        {0x11u,0x22u,0x33u,0xffu},
        {0x04u,0x05u,0x06u,0xffu},
        {0x44u,0x55u,0x66u,0xffu},
        {0x77u,0x88u,0x99u,0xffu},
        {0xaau,0xbbu,0xccu,0xffu},
        {0xddu,0xeeu,0xf0u,0xffu}
    };
    size_t web1_w_pixel =
        (1u * 40u + 2u) * RIVET_GFX_PIXEL_BYTES;
    size_t source_s_pixel =
        (1u * 40u + 6u) * RIVET_GFX_PIXEL_BYTES;

    storage.document_bytes = document_bytes;
    storage.document_capacity = sizeof(document_bytes);
    storage.nodes = nodes;
    storage.node_capacity = 16u;
    storage.rules = rules;
    storage.rule_capacity = 4u;
    storage.boxes = boxes;
    storage.box_capacity = 16u;
    storage.history = history;
    storage.history_capacity = 2u;
    storage.bookmarks = bookmarks;
    storage.bookmark_capacity = 2u;
    storage.scratch_bytes = scratch_bytes;
    storage.scratch_capacity = sizeof(scratch_bytes);

    io.context = NULL;
    io.fetch = charset_fetch;
    io.download = NULL;

    CHECK(rivet_browser_config_parse(
        &config,
        CONFIG_BYTES,
        sizeof(CONFIG_BYTES)) == RIVET_OK);
    CHECK(rivet_browser_init(
        &browser,
        &io,
        &config,
        &storage,
        40ul) == RIVET_OK);
    CHECK(rivet_browser_home(&browser) == RIVET_OK);
    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        40ul,
        20ul,
        40u * RIVET_GFX_PIXEL_BYTES) == RIVET_OK);

    CHECK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style) == RIVET_OK);
    CHECK(pixel_is(
        pixels,
        web1_w_pixel,
        style.chrome_foreground));

    CHECK(rivet_browser_toggle_source(
        &browser) == RIVET_OK);
    CHECK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style) == RIVET_OK);
    CHECK(pixel_is(
        pixels,
        source_s_pixel,
        style.chrome_foreground));

    return 0;
}
