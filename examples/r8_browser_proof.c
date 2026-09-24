/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/browser.h"
#include "ppm.h"

#include <stdio.h>
#include <string.h>

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
    unsigned char downloaded_url[RIVET_BROWSER_URL_MAX];
    size_t downloaded_url_length;
    size_t fetch_count;
    size_t download_count;
} proof_io;

static const unsigned char url_home[] =
    "https://rivet.test/home";
static const unsigned char url_about[] =
    "https://rivet.test/about";
static const unsigned char url_download[] =
    "https://rivet.test/download.txt";

static unsigned long long fnv1a64(
    const unsigned char *bytes,
    size_t count
)
{
    unsigned long long hash =
        0xcbf29ce484222325ULL;
    size_t i;

    for (i = 0u; i < count; ++i) {
        hash ^= (unsigned long long)bytes[i];
        hash *= 0x100000001b3ULL;
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

    while (used < capacity) {
        size_t received = fread(
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

    if (used == capacity) {
        unsigned char extra;
        if (fread(&extra, 1u, 1u, file) != 0u) {
            fclose(file);
            return 0;
        }
    }

    if (ferror(file) || fclose(file) != 0) {
        return 0;
    }

    *count = used;
    return 1;
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

    if (io == NULL || url == NULL ||
        bytes == NULL ||
        url_length > sizeof(io->downloaded_url) ||
        byte_count > sizeof(io->downloaded)) {
        return RIVET_ERR_CAPACITY;
    }

    memcpy(
        io->downloaded_url,
        url,
        url_length
    );
    io->downloaded_url_length =
        url_length;
    memcpy(
        io->downloaded,
        bytes,
        byte_count
    );
    io->downloaded_bytes =
        byte_count;
    ++io->download_count;
    return RIVET_OK;
}

#define REQUIRE_OK(expr) do { \
    rivet_result _result = (expr); \
    if (_result != RIVET_OK) { \
        fprintf(stderr, "proof failed: %s => %s\n", #expr, rivet_result_name(_result)); \
        return 1; \
    } \
} while (0)

int main(int argc, char **argv)
{
    unsigned char home_bytes[1024];
    unsigned char about_bytes[512];
    unsigned char download_bytes[256];
    unsigned char config_bytes[512];
    size_t home_count = 0u;
    size_t about_count = 0u;
    size_t download_count = 0u;
    size_t config_count = 0u;

    unsigned char document_bytes[2048];
    unsigned char scratch_bytes[512];
    rivet_doc_node nodes[64];
    rivet_css_rule rules[32];
    rivet_layout_box boxes[128];
    rivet_browser_url history[8];
    rivet_browser_url bookmarks[8];
    rivet_browser_storage storage;
    rivet_browser_config config;
    rivet_browser browser;
    proof_io io_state;
    rivet_browser_io io;

    rivet_command_slot slots[16];
    rivet_command_registry commands;
    const rivet_keymap *keymap;

    unsigned char pixels[PROOF_PIXELS];
    rivet_surface surface;
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

    rivet_key_event next_link =
        {0x4eu,0u,1};
    rivet_key_event enter =
        {RIVET_KEY_ENTER,0u,1};
    rivet_key_event download =
        {0x44u,RIVET_MOD_CTRL,1};
    rivet_key_event back =
        {0x42u,RIVET_MOD_ALT,1};
    rivet_key_event bookmark =
        {0x4bu,RIVET_MOD_CTRL,1};
    rivet_key_event source =
        {0x53u,RIVET_MOD_CTRL,1};

    unsigned long long document_hash;
    unsigned long long source_hash;
    size_t resident_bytes;
    const char *output_path =
        argc > 1 ?
        argv[1] :
        "build/rivet-web1-proof.ppm";

    if (!read_file(
            "fixtures/r8_home.html",
            home_bytes,
            sizeof(home_bytes),
            &home_count) ||
        !read_file(
            "fixtures/r8_about.html",
            about_bytes,
            sizeof(about_bytes),
            &about_count) ||
        !read_file(
            "fixtures/r8_download.txt",
            download_bytes,
            sizeof(download_bytes),
            &download_count) ||
        !read_file(
            "fixtures/r8_browser.conf",
            config_bytes,
            sizeof(config_bytes),
            &config_count)) {
        fprintf(stderr, "proof failed: fixture read\n");
        return 1;
    }

    memset(&io_state, 0, sizeof(io_state));
    io_state.resources[0].url = url_home;
    io_state.resources[0].url_length =
        sizeof(url_home) - 1u;
    io_state.resources[0].bytes = home_bytes;
    io_state.resources[0].byte_count = home_count;
    io_state.resources[1].url = url_about;
    io_state.resources[1].url_length =
        sizeof(url_about) - 1u;
    io_state.resources[1].bytes = about_bytes;
    io_state.resources[1].byte_count = about_count;
    io_state.resources[2].url = url_download;
    io_state.resources[2].url_length =
        sizeof(url_download) - 1u;
    io_state.resources[2].bytes = download_bytes;
    io_state.resources[2].byte_count = download_count;
    io_state.resource_count = 3u;

    io.context = &io_state;
    io.fetch = proof_fetch;
    io.download = proof_download;

    memset(&storage, 0, sizeof(storage));
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

    REQUIRE_OK(rivet_browser_config_parse(
        &config,
        config_bytes,
        config_count));
    REQUIRE_OK(rivet_browser_init(
        &browser,
        &io,
        &config,
        &storage,
        PROOF_WIDTH));
    REQUIRE_OK(rivet_browser_home(
        &browser));

    REQUIRE_OK(rivet_commands_init(
        &commands,
        slots,
        sizeof(slots) / sizeof(slots[0])));
    REQUIRE_OK(rivet_browser_register_commands(
        &browser,
        &commands));
    keymap = rivet_browser_default_keymap();
    REQUIRE_OK(rivet_keymap_validate(keymap));

    REQUIRE_OK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        PROOF_WIDTH,
        PROOF_HEIGHT,
        (size_t)PROOF_WIDTH *
            RIVET_GFX_PIXEL_BYTES));
    REQUIRE_OK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style));
    document_hash =
        fnv1a64(pixels, sizeof(pixels));

    REQUIRE_OK(rivet_keymap_dispatch(
        keymap,
        &commands,
        next_link));
    REQUIRE_OK(rivet_keymap_dispatch(
        keymap,
        &commands,
        download));

    if (io_state.download_count != 1u ||
        !bytes_equal(
            io_state.downloaded,
            io_state.downloaded_bytes,
            download_bytes,
            download_count)) {
        fprintf(stderr, "proof failed: download sink\n");
        return 1;
    }

    REQUIRE_OK(rivet_keymap_dispatch(
        keymap,
        &commands,
        next_link));
    REQUIRE_OK(rivet_keymap_dispatch(
        keymap,
        &commands,
        enter));

    if (browser.history_count != 2u ||
        browser.history_index != 1u ||
        !bytes_equal(
            browser.current_url.bytes,
            browser.current_url.length,
            url_about,
            sizeof(url_about) - 1u)) {
        fprintf(stderr, "proof failed: about navigation\n");
        return 1;
    }

    REQUIRE_OK(rivet_keymap_dispatch(
        keymap,
        &commands,
        back));
    REQUIRE_OK(rivet_keymap_dispatch(
        keymap,
        &commands,
        bookmark));

    if (browser.bookmark_count != 1u ||
        browser.history_index != 0u) {
        fprintf(stderr, "proof failed: history/bookmark\n");
        return 1;
    }

    REQUIRE_OK(rivet_keymap_dispatch(
        keymap,
        &commands,
        source));
    REQUIRE_OK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style));
    source_hash =
        fnv1a64(pixels, sizeof(pixels));

    if (!rivet_headless_write_ppm(
            output_path,
            &surface)) {
        fprintf(stderr, "proof failed: ppm write\n");
        return 1;
    }

    resident_bytes =
        sizeof(home_bytes) +
        sizeof(about_bytes) +
        sizeof(download_bytes) +
        sizeof(config_bytes) +
        sizeof(home_count) +
        sizeof(about_count) +
        sizeof(download_count) +
        sizeof(config_count) +
        sizeof(document_bytes) +
        sizeof(scratch_bytes) +
        sizeof(nodes) +
        sizeof(rules) +
        sizeof(boxes) +
        sizeof(history) +
        sizeof(bookmarks) +
        sizeof(storage) +
        sizeof(config) +
        sizeof(browser) +
        sizeof(io_state) +
        sizeof(io) +
        sizeof(slots) +
        sizeof(commands) +
        sizeof(keymap) +
        sizeof(pixels) +
        sizeof(surface) +
        sizeof(bounds) +
        sizeof(style) +
        sizeof(next_link) +
        sizeof(enter) +
        sizeof(download) +
        sizeof(back) +
        sizeof(bookmark) +
        sizeof(source) +
        sizeof(document_hash) +
        sizeof(source_hash) +
        sizeof(resident_bytes) +
        sizeof(output_path);

    printf(
        "rivet-r8: document_fnv1a64=%016llx source_fnv1a64=%016llx "
        "history=%lu bookmarks=%lu fetches=%lu downloads=%lu "
        "browser_state_bytes=%lu proof_resident_bytes=%lu ppm=%s\n",
        document_hash,
        source_hash,
        (unsigned long)browser.history_count,
        (unsigned long)browser.bookmark_count,
        (unsigned long)io_state.fetch_count,
        (unsigned long)io_state.download_count,
        (unsigned long)sizeof(browser),
        (unsigned long)resident_bytes,
        output_path
    );

    if (document_hash !=
            EXPECTED_DOCUMENT_FNV1A64 ||
        source_hash !=
            EXPECTED_SOURCE_FNV1A64) {
        fprintf(
            stderr,
            "proof identity pending: document=%016llx source=%016llx\n",
            document_hash,
            source_hash
        );
        return 1;
    }

    return 0;
}
