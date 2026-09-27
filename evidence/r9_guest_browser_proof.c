/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/browser.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifndef RIVET_R9_TARGET_PROFILE
#define RIVET_R9_TARGET_PROFILE "unspecified"
#endif

#ifndef RIVET_R9_SOURCE_REVISION
#define RIVET_R9_SOURCE_REVISION "unknown"
#endif

#define PROOF_WIDTH 120ul
#define PROOF_HEIGHT 64ul
#define PROOF_PIXELS ((size_t)PROOF_WIDTH * (size_t)PROOF_HEIGHT * RIVET_GFX_PIXEL_BYTES)
#define EXPECTED_DOCUMENT_FNV1A64 0x75be6cc92698ac1aULL
#define EXPECTED_SOURCE_FNV1A64 0x5cf7c63a1fa3d9b4ULL

typedef struct proof_resource {
    const unsigned char *url;
    size_t url_length;
    const unsigned char *bytes;
    size_t byte_count;
} proof_resource;

typedef struct proof_io {
    proof_resource resources[3];
    size_t resource_count;
    unsigned char downloaded[256];
    size_t downloaded_bytes;
    size_t fetch_count;
    size_t download_count;
} proof_io;

static const unsigned char URL_HOME[] =
    "https://rivet.test/home";
static const unsigned char URL_ABOUT[] =
    "https://rivet.test/about";
static const unsigned char URL_DOWNLOAD[] =
    "https://rivet.test/download.txt";

static const unsigned char HOME_BYTES[] =
    "<html><head><style>body{color:#202020;}a{color:#3366ff;}"
    "</style></head><body><p>HOME "
    "<a href=\"https://rivet.test/about\">ABOUT</a> "
    "<a href=\"https://rivet.test/download.txt\">FILE</a>"
    "</p></body></html>\n";

static const unsigned char ABOUT_BYTES[] =
    "<html><body><p>ABOUT "
    "<a href=\"https://rivet.test/home\">HOME</a>"
    "</p></body></html>\n";

static const unsigned char DOWNLOAD_BYTES[] =
    "RIVET WEB1 DOWNLOAD\n";

static const unsigned char CONFIG_BYTES[] =
    "RIVET-WEB1 1\n"
    "home=https://rivet.test/home\n"
    "downloads=1\n"
    "user-css=p{color:#ff0000;}\n";

static unsigned long long fnv1a64(
    const unsigned char *bytes,
    size_t count
)
{
    unsigned long long hash = 0xcbf29ce484222325ULL;
    size_t i;

    for (i = 0u; i < count; ++i) {
        hash ^= (unsigned long long)bytes[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

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

static rivet_result proof_fetch(
    void *context,
    const unsigned char *url,
    size_t url_length,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    proof_io *io = (proof_io *)context;
    size_t i;

    if (io == NULL || url == NULL ||
        buffer == NULL || byte_count == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    ++io->fetch_count;
    for (i = 0u; i < io->resource_count; ++i) {
        const proof_resource *resource =
            &io->resources[i];

        if (!bytes_equal(
                url,
                url_length,
                resource->url,
                resource->url_length)) {
            continue;
        }
        if (resource->byte_count > capacity) {
            return RIVET_ERR_CAPACITY;
        }

        memcpy(
            buffer,
            resource->bytes,
            resource->byte_count
        );
        *byte_count = resource->byte_count;
        return RIVET_OK;
    }

    return RIVET_ERR_NOT_FOUND;
}

static rivet_result proof_download(
    void *context,
    const unsigned char *url,
    size_t url_length,
    const unsigned char *bytes,
    size_t byte_count
)
{
    proof_io *io = (proof_io *)context;

    (void)url;
    (void)url_length;

    if (io == NULL || bytes == NULL ||
        byte_count > sizeof(io->downloaded)) {
        return RIVET_ERR_CAPACITY;
    }

    memcpy(io->downloaded, bytes, byte_count);
    io->downloaded_bytes = byte_count;
    ++io->download_count;
    return RIVET_OK;
}

static int little_endian(void)
{
    unsigned int value = 1u;
    return *((const unsigned char *)&value) == 1u;
}

static int fail(FILE *file, const char *message)
{
    if (file != NULL) {
        fprintf(file, "error=%s\n", message);
        fclose(file);
    }
    return 1;
}

int main(int argc, char **argv)
{
    const char *output_path =
        argc > 1 ? argv[1] : "RIVET-R9-RECEIPT.TXT";
    FILE *receipt;
    static unsigned char document_bytes[2048];
    static unsigned char scratch_bytes[512];
    static rivet_doc_node nodes[64];
    static rivet_css_rule rules[32];
    static rivet_layout_box boxes[128];
    static rivet_browser_url history[8];
    static rivet_browser_url bookmarks[8];
    static rivet_browser_storage storage;
    static rivet_browser_config config;
    static rivet_browser browser;
    static proof_io io_state;
    static rivet_browser_io io;
    static unsigned char pixels[PROOF_PIXELS];
    static rivet_surface surface;
    rivet_rect bounds =
        {0L,0L,PROOF_WIDTH,PROOF_HEIGHT};
    rivet_browser_style style = {
        {0x12u,0x16u,0x18u,0xffu},
        {0xf0u,0xb4u,0x4du,0xffu},
        {0x08u,0x0cu,0x10u,0xffu},
        {0xc0u,0xc8u,0xd0u,0xffu},
        {0x40u,0x48u,0x50u,0xffu},
        {0x48u,0x50u,0x58u,0xffu},
        {0x20u,0x28u,0x30u,0xffu}
    };
    unsigned long long document_hash;
    unsigned long long source_hash;

    receipt = fopen(output_path, "wb");
    if (receipt == NULL) {
        return 2;
    }

    memset(&io_state, 0, sizeof(io_state));
    io_state.resources[0].url = URL_HOME;
    io_state.resources[0].url_length =
        sizeof(URL_HOME) - 1u;
    io_state.resources[0].bytes = HOME_BYTES;
    io_state.resources[0].byte_count =
        sizeof(HOME_BYTES) - 1u;
    io_state.resources[1].url = URL_ABOUT;
    io_state.resources[1].url_length =
        sizeof(URL_ABOUT) - 1u;
    io_state.resources[1].bytes = ABOUT_BYTES;
    io_state.resources[1].byte_count =
        sizeof(ABOUT_BYTES) - 1u;
    io_state.resources[2].url = URL_DOWNLOAD;
    io_state.resources[2].url_length =
        sizeof(URL_DOWNLOAD) - 1u;
    io_state.resources[2].bytes = DOWNLOAD_BYTES;
    io_state.resources[2].byte_count =
        sizeof(DOWNLOAD_BYTES) - 1u;
    io_state.resource_count = 3u;

    io.context = &io_state;
    io.fetch = proof_fetch;
    io.download = proof_download;

    storage.document_bytes = document_bytes;
    storage.document_capacity = sizeof(document_bytes);
    storage.nodes = nodes;
    storage.node_capacity =
        sizeof(nodes) / sizeof(nodes[0]);
    storage.rules = rules;
    storage.rule_capacity =
        sizeof(rules) / sizeof(rules[0]);
    storage.boxes = boxes;
    storage.box_capacity =
        sizeof(boxes) / sizeof(boxes[0]);
    storage.history = history;
    storage.history_capacity =
        sizeof(history) / sizeof(history[0]);
    storage.bookmarks = bookmarks;
    storage.bookmark_capacity =
        sizeof(bookmarks) / sizeof(bookmarks[0]);
    storage.scratch_bytes = scratch_bytes;
    storage.scratch_capacity = sizeof(scratch_bytes);

    if (rivet_browser_config_parse(
            &config,
            CONFIG_BYTES,
            sizeof(CONFIG_BYTES) - 1u) != RIVET_OK) {
        return fail(receipt, "config");
    }
    if (rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            PROOF_WIDTH) != RIVET_OK ||
        rivet_browser_home(&browser) != RIVET_OK) {
        return fail(receipt, "home");
    }

    if (rivet_surface_attach(
            &surface,
            pixels,
            sizeof(pixels),
            PROOF_WIDTH,
            PROOF_HEIGHT,
            (size_t)PROOF_WIDTH *
                RIVET_GFX_PIXEL_BYTES) != RIVET_OK ||
        rivet_browser_render(
            &surface,
            &browser,
            bounds,
            style) != RIVET_OK) {
        return fail(receipt, "document-render");
    }
    document_hash = fnv1a64(pixels, sizeof(pixels));

    if (rivet_browser_next_link(&browser) != RIVET_OK ||
        rivet_browser_download_selected(
            &browser) != RIVET_OK ||
        io_state.download_count != 1u ||
        !bytes_equal(
            io_state.downloaded,
            io_state.downloaded_bytes,
            DOWNLOAD_BYTES,
            sizeof(DOWNLOAD_BYTES) - 1u)) {
        return fail(receipt, "download");
    }

    if (rivet_browser_next_link(&browser) != RIVET_OK ||
        rivet_browser_open_selected_link(
            &browser) != RIVET_OK ||
        rivet_browser_back(&browser) != RIVET_OK ||
        rivet_browser_bookmark_current(
            &browser) != RIVET_OK ||
        rivet_browser_toggle_source(
            &browser) != RIVET_OK ||
        rivet_browser_render(
            &surface,
            &browser,
            bounds,
            style) != RIVET_OK) {
        return fail(receipt, "navigation");
    }
    source_hash = fnv1a64(pixels, sizeof(pixels));

    fprintf(
        receipt,
        "rivet-r9-guest: target=%s source=%s "
        "pointer_bits=%u endian=%s "
        "document_fnv1a64=%016llx "
        "source_fnv1a64=%016llx "
        "history=%lu bookmarks=%lu fetches=%lu downloads=%lu\n",
        RIVET_R9_TARGET_PROFILE,
        RIVET_R9_SOURCE_REVISION,
        (unsigned int)(sizeof(void *) * CHAR_BIT),
        little_endian() ? "little" : "big",
        document_hash,
        source_hash,
        (unsigned long)browser.history_count,
        (unsigned long)browser.bookmark_count,
        (unsigned long)io_state.fetch_count,
        (unsigned long)io_state.download_count
    );

    if (fclose(receipt) != 0) {
        return 1;
    }

    return document_hash == EXPECTED_DOCUMENT_FNV1A64 &&
           source_hash == EXPECTED_SOURCE_FNV1A64 &&
           browser.history_count == 2u &&
           browser.bookmark_count == 1u &&
           io_state.fetch_count == 4u &&
           io_state.download_count == 1u ?
        0 : 1;
}
