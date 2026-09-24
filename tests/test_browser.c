/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/browser.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

typedef struct test_io_state {
    unsigned char downloaded[64];
    size_t downloaded_bytes;
    unsigned char downloaded_url[RIVET_BROWSER_URL_MAX];
    size_t downloaded_url_length;
    size_t fetch_count;
    size_t download_count;
} test_io_state;

static const unsigned char url_home[] =
    "https://site.test/home";
static const unsigned char url_about[] =
    "https://site.test/about";
static const unsigned char url_file[] =
    "https://site.test/file.txt";
static const unsigned char url_nested[] =
    "https://site.test/nested";
static const unsigned char url_fail[] =
    "https://site.test/fail";

static const unsigned char page_home[] =
    "<html><head><style>"
    "body{color:#202020;}a{color:#3366ff;}"
    "</style></head><body>"
    "<p>HOME <a href=\"https://site.test/about\">ABOUT</a> "
    "<a href=\"https://site.test/file.txt\">FILE</a></p>"
    "</body></html>";

static const unsigned char page_about[] =
    "<html><body><p>ABOUT "
    "<a href=\"https://site.test/home\">HOME</a>"
    "</p></body></html>";

static const unsigned char page_nested[] =
    "<html><body><a href=\"https://site.test/about\">"
    "<p>NESTED</p></a></body></html>";

static const unsigned char file_payload[] =
    "RIVET DOWNLOAD\n";

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

static rivet_result test_fetch(
    void *context,
    const unsigned char *url,
    size_t url_length,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    test_io_state *state =
        (test_io_state *)context;
    const unsigned char *source = NULL;
    size_t source_count = 0u;

    if (state == NULL || url == NULL ||
        buffer == NULL || byte_count == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    ++state->fetch_count;

    if (bytes_equal(
            url, url_length,
            url_home, sizeof(url_home) - 1u)) {
        source = page_home;
        source_count = sizeof(page_home) - 1u;
    } else if (bytes_equal(
                   url, url_length,
                   url_about, sizeof(url_about) - 1u)) {
        source = page_about;
        source_count = sizeof(page_about) - 1u;
    } else if (bytes_equal(
                   url, url_length,
                   url_file, sizeof(url_file) - 1u)) {
        source = file_payload;
        source_count = sizeof(file_payload) - 1u;
    } else if (bytes_equal(
                   url, url_length,
                   url_nested, sizeof(url_nested) - 1u)) {
        source = page_nested;
        source_count = sizeof(page_nested) - 1u;
    } else if (bytes_equal(
                   url, url_length,
                   url_fail, sizeof(url_fail) - 1u)) {
        static const unsigned char partial[] =
            "<html>";
        if (sizeof(partial) - 1u > capacity) {
            return RIVET_ERR_CAPACITY;
        }
        memcpy(
            buffer,
            partial,
            sizeof(partial) - 1u
        );
        *byte_count = sizeof(partial) - 1u;
        return RIVET_ERR_NOT_FOUND;
    } else {
        return RIVET_ERR_NOT_FOUND;
    }

    if (source_count > capacity) {
        return RIVET_ERR_CAPACITY;
    }

    memcpy(buffer, source, source_count);
    *byte_count = source_count;
    return RIVET_OK;
}

static rivet_result test_download(
    void *context,
    const unsigned char *url,
    size_t url_length,
    const unsigned char *bytes,
    size_t byte_count
)
{
    test_io_state *state =
        (test_io_state *)context;

    if (state == NULL || url == NULL ||
        bytes == NULL ||
        url_length > sizeof(state->downloaded_url) ||
        byte_count > sizeof(state->downloaded)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    memcpy(
        state->downloaded_url,
        url,
        url_length
    );
    state->downloaded_url_length =
        url_length;
    memcpy(
        state->downloaded,
        bytes,
        byte_count
    );
    state->downloaded_bytes =
        byte_count;
    ++state->download_count;
    return RIVET_OK;
}

static int find_red_text(
    const rivet_browser *browser
)
{
    size_t i;

    for (i = 0u; i < browser->box_count; ++i) {
        const rivet_layout_box *box =
            &browser->storage.boxes[i];
        if (box->kind == RIVET_LAYOUT_TEXT &&
            box->foreground.r == 0xffu &&
            box->foreground.g == 0x00u &&
            box->foreground.b == 0x00u) {
            return 1;
        }
    }
    return 0;
}

static const rivet_layout_box *find_nested_selected_box(
    const rivet_browser *browser
)
{
    size_t i;

    for (i = 0u; i < browser->box_count; ++i) {
        const rivet_layout_box *box =
            &browser->storage.boxes[i];
        size_t node_index = box->node_index;
        size_t hops = 0u;
        size_t remaining =
            browser->document.node_count;

        if (box->kind != RIVET_LAYOUT_TEXT) {
            continue;
        }

        while (node_index <
                   browser->document.node_count &&
               remaining != 0u) {
            if (node_index ==
                browser->selected_link_node) {
                if (hops >= 2u) {
                    return box;
                }
                break;
            }
            node_index = browser->document.nodes[
                node_index].parent;
            ++hops;
            --remaining;
        }
    }

    return NULL;
}

static int test_browser_flow(void)
{
    static const unsigned char config_bytes[] =
        "RIVET-WEB1 1\n"
        "home=https://site.test/home\n"
        "downloads=1\n"
        "user-css=p{color:#ff0000;}\n";
    unsigned char document_bytes[1024];
    unsigned char scratch_bytes[256];
    rivet_doc_node nodes[32];
    rivet_css_rule rules[16];
    rivet_layout_box boxes[64];
    rivet_browser_url history[8];
    rivet_browser_url bookmarks[4];
    rivet_browser_storage storage;
    rivet_browser_config config;
    rivet_browser_io io;
    rivet_browser browser;
    test_io_state io_state;
    rivet_command_slot slots[16];
    rivet_command_registry commands;
    const rivet_keymap *keymap;
    rivet_key_event alt_b =
        {0x42u,RIVET_MOD_ALT,1};
    rivet_key_event alt_f =
        {0x46u,RIVET_MOD_ALT,1};
    rivet_key_event source =
        {0x53u,RIVET_MOD_CTRL,1};
    unsigned char pixels[120u * 64u * 4u];
    unsigned char chrome_before[
        120u *
        RIVET_BROWSER_CHROME_HEIGHT *
        RIVET_GFX_PIXEL_BYTES
    ];
    rivet_surface surface;
    rivet_rect bounds = {0L,0L,120ul,64ul};
    rivet_browser_style style = {
        {0x12u,0x16u,0x18u,0xffu},
        {0xf0u,0xb4u,0x4du,0xffu},
        {0x08u,0x0cu,0x10u,0xffu},
        {0xc0u,0xc8u,0xd0u,0xffu},
        {0x40u,0x48u,0x50u,0xffu},
        {0x48u,0x50u,0x58u,0xffu},
        {0x20u,0x28u,0x30u,0xffu}
    };

    memset(&io_state, 0, sizeof(io_state));
    memset(&storage, 0, sizeof(storage));

    CHECK(rivet_browser_config_parse(
        &config,
        config_bytes,
        sizeof(config_bytes) - 1u) == RIVET_OK);
    CHECK(config.downloads_enabled == 1);

    storage.document_bytes =
        document_bytes;
    storage.document_capacity =
        sizeof(document_bytes);
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
        sizeof(bookmarks) /
        sizeof(bookmarks[0]);
    storage.scratch_bytes =
        scratch_bytes;
    storage.scratch_capacity =
        sizeof(scratch_bytes);

    io.context = &io_state;
    io.fetch = test_fetch;
    io.download = NULL;

    CHECK(rivet_browser_init(
        &browser,
        &io,
        &config,
        &storage,
        120ul) == RIVET_ERR_INVALID_ARGUMENT);

    io.download = test_download;

    CHECK(rivet_browser_init(
        &browser,
        &io,
        &config,
        &storage,
        120ul) == RIVET_OK);
    CHECK(rivet_browser_home(
        &browser) == RIVET_OK);
    CHECK(browser.loaded);
    CHECK(browser.history_count == 1u);
    CHECK(browser.history_index == 0u);
    CHECK(browser.selected_link_node !=
          RIVET_DOCUMENT_NO_PARENT);
    CHECK(find_red_text(&browser));

    CHECK(rivet_browser_next_link(
        &browser) == RIVET_OK);
    CHECK(rivet_browser_download_selected(
        &browser) == RIVET_OK);
    CHECK(io_state.download_count == 1u);
    CHECK(bytes_equal(
        io_state.downloaded_url,
        io_state.downloaded_url_length,
        url_file,
        sizeof(url_file) - 1u));
    CHECK(bytes_equal(
        io_state.downloaded,
        io_state.downloaded_bytes,
        file_payload,
        sizeof(file_payload) - 1u));

    CHECK(rivet_browser_next_link(
        &browser) == RIVET_OK);
    CHECK(rivet_browser_open_selected_link(
        &browser) == RIVET_OK);
    CHECK(browser.history_count == 2u);
    CHECK(browser.history_index == 1u);
    CHECK(bytes_equal(
        browser.current_url.bytes,
        browser.current_url.length,
        url_about,
        sizeof(url_about) - 1u));

    CHECK(rivet_browser_bookmark_current(
        &browser) == RIVET_OK);
    CHECK(browser.bookmark_count == 1u);
    CHECK(rivet_browser_bookmark_current(
        &browser) == RIVET_ERR_DUPLICATE);

    CHECK(rivet_browser_back(
        &browser) == RIVET_OK);
    CHECK(browser.history_index == 0u);
    CHECK(bytes_equal(
        browser.current_url.bytes,
        browser.current_url.length,
        url_home,
        sizeof(url_home) - 1u));

    CHECK(rivet_browser_forward(
        &browser) == RIVET_OK);
    CHECK(browser.history_index == 1u);

    CHECK(rivet_commands_init(
        &commands,
        slots,
        sizeof(slots) / sizeof(slots[0])) ==
        RIVET_OK);
    CHECK(rivet_browser_register_commands(
        &browser,
        &commands) == RIVET_OK);
    keymap = rivet_browser_default_keymap();
    CHECK(rivet_keymap_validate(
        keymap) == RIVET_OK);

    CHECK(rivet_keymap_dispatch(
        keymap,
        &commands,
        alt_b) == RIVET_OK);
    CHECK(browser.history_index == 0u);
    CHECK(rivet_keymap_dispatch(
        keymap,
        &commands,
        alt_f) == RIVET_OK);
    CHECK(browser.history_index == 1u);

    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        120ul,
        64ul,
        120u * 4u) == RIVET_OK);
    CHECK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style) == RIVET_OK);

    memcpy(
        chrome_before,
        pixels,
        sizeof(chrome_before)
    );
    browser.scroll_y = 8ul;
    CHECK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style) == RIVET_OK);
    CHECK(memcmp(
        chrome_before,
        pixels,
        sizeof(chrome_before)) == 0);
    browser.scroll_y = 0ul;

    CHECK(rivet_keymap_dispatch(
        keymap,
        &commands,
        source) == RIVET_OK);
    CHECK(browser.source_mode == 1);
    CHECK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style) == RIVET_OK);

    CHECK(rivet_browser_toggle_source(
        &browser) == RIVET_OK);
    CHECK(browser.source_mode == 0);

    CHECK(rivet_browser_open(
        &browser,
        url_nested,
        sizeof(url_nested) - 1u) == RIVET_OK);
    {
        const rivet_layout_box *nested_box =
            find_nested_selected_box(&browser);
        size_t pixel_offset;

        CHECK(nested_box != NULL);
        CHECK(nested_box->width != 0ul);
        CHECK(nested_box->x +
              nested_box->width <= 120ul);
        CHECK(nested_box->y +
              RIVET_BROWSER_CHROME_HEIGHT <
              64ul);

        CHECK(rivet_browser_render(
            &surface,
            &browser,
            bounds,
            style) == RIVET_OK);

        pixel_offset =
            ((size_t)(
                 nested_box->y +
                 RIVET_BROWSER_CHROME_HEIGHT) *
             120u +
             (size_t)(
                 nested_box->x +
                 nested_box->width - 1ul)) *
            RIVET_GFX_PIXEL_BYTES;

        CHECK(pixels[pixel_offset] ==
              style.link_focus_background.r);
        CHECK(pixels[pixel_offset + 1u] ==
              style.link_focus_background.g);
        CHECK(pixels[pixel_offset + 2u] ==
              style.link_focus_background.b);
        CHECK(pixels[pixel_offset + 3u] ==
              style.link_focus_background.a);
    }

    CHECK(rivet_browser_open(
        &browser,
        url_fail,
        sizeof(url_fail) - 1u) ==
        RIVET_ERR_NOT_FOUND);
    CHECK(!browser.loaded);
    CHECK(browser.document_bytes == 0u);
    CHECK(browser.rule_count == 0u);
    CHECK(browser.box_count == 0u);

    CHECK(io_state.fetch_count >= 8u);
    return 0;
}

static int test_config_failures(void)
{
    static const unsigned char bad_magic[] =
        "RIVET-WEB1 2\n"
        "home=https://site.test/home\n"
        "downloads=1\n"
        "user-css=\n";
    static const unsigned char bad_download[] =
        "RIVET-WEB1 1\n"
        "home=https://site.test/home\n"
        "downloads=2\n"
        "user-css=\n";
    static const unsigned char bad_css[] =
        "RIVET-WEB1 1\n"
        "home=https://site.test/home\n"
        "downloads=0\n"
        "user-css=p{color:#zzzzzz;}\n";
    static const unsigned char prefix[] =
        "RIVET-WEB1 1\nhome=https://site.test/";
    static const unsigned char suffix[] =
        "\ndownloads=0\nuser-css=\n";
    unsigned char long_home[768];
    size_t cursor;
    size_t path_bytes;
    rivet_browser_config config;

    CHECK(rivet_browser_config_parse(
        &config,
        bad_magic,
        sizeof(bad_magic) - 1u) ==
        RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_browser_config_parse(
        &config,
        bad_download,
        sizeof(bad_download) - 1u) ==
        RIVET_ERR_UNSUPPORTED);
    CHECK(rivet_browser_config_parse(
        &config,
        bad_css,
        sizeof(bad_css) - 1u) !=
        RIVET_OK);

    cursor = 0u;
    memcpy(
        long_home + cursor,
        prefix,
        sizeof(prefix) - 1u
    );
    cursor += sizeof(prefix) - 1u;
    path_bytes =
        RIVET_BROWSER_URL_MAX + 1u -
        (sizeof("https://site.test/") - 1u);
    memset(
        long_home + cursor,
        0x61,
        path_bytes
    );
    cursor += path_bytes;
    memcpy(
        long_home + cursor,
        suffix,
        sizeof(suffix) - 1u
    );
    cursor += sizeof(suffix) - 1u;

    CHECK(rivet_browser_config_parse(
        &config,
        long_home,
        cursor) == RIVET_ERR_CAPACITY);
    return 0;
}

int main(void)
{
    CHECK(RIVET_BROWSER_ABI_VERSION == 1u);
    CHECK(test_browser_flow() == 0);
    CHECK(test_config_failures() == 0);

    puts("rivet browser tests: ok");
    return 0;
}
