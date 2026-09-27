/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/browser.h"

#include <limits.h>
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
static const unsigned char url_replaced[] =
    "https://site.test/replaced";

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

static const unsigned char page_empty[] =
    "<html><body></body></html>";

static const unsigned char page_bad_layout[] =
    "<html><head><style>"
    "p{color:bogus;}"
    "</style></head><body><p>X</p></body></html>";

static const unsigned char page_replaced[] =
    "<html><head><style>"
    "img{background-color:#112233;}"
    "input{background-color:#445566;}"
    "</style></head><body>"
    "<p>T</p>"
    "<img src=\"https://site.test/file.txt\" width=\"8\" height=\"8\">"
    "<input name=\"x\" value=\"y\">"
    "</body></html>";

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

static int url_tail_zero(
    const rivet_browser_url *url
)
{
    size_t i;

    if (url == NULL ||
        url->length > RIVET_BROWSER_URL_MAX) {
        return 0;
    }

    for (i = url->length;
         i < RIVET_BROWSER_URL_MAX;
         ++i) {
        if (url->bytes[i] != 0u) {
            return 0;
        }
    }
    return 1;
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
                   url_replaced, sizeof(url_replaced) - 1u)) {
        source = page_replaced;
        source_count = sizeof(page_replaced) - 1u;
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

static rivet_result test_empty_fetch(
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
            url_home,
            sizeof(url_home) - 1u) ||
        sizeof(page_empty) - 1u > capacity) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    memcpy(
        buffer,
        page_empty,
        sizeof(page_empty) - 1u
    );
    *byte_count = sizeof(page_empty) - 1u;
    return RIVET_OK;
}

static rivet_result test_bad_layout_fetch(
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
            url_home,
            sizeof(url_home) - 1u) ||
        sizeof(page_bad_layout) - 1u > capacity) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    memcpy(
        buffer,
        page_bad_layout,
        sizeof(page_bad_layout) - 1u
    );
    *byte_count = sizeof(page_bad_layout) - 1u;
    return RIVET_OK;
}

static rivet_result test_zero_success_fetch(
    void *context,
    const unsigned char *url,
    size_t url_length,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    (void)context;
    (void)url;
    (void)url_length;

    if (buffer == NULL || byte_count == NULL ||
        capacity == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    buffer[0] = 0x58u;
    *byte_count = 0u;
    return RIVET_OK;
}

static rivet_result test_oversize_success_fetch(
    void *context,
    const unsigned char *url,
    size_t url_length,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
)
{
    (void)context;
    (void)url;
    (void)url_length;

    if (buffer == NULL || byte_count == NULL ||
        capacity == 0u ||
        capacity == (size_t)-1) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    buffer[0] = 0x58u;
    *byte_count = capacity + 1u;
    return RIVET_OK;
}

static rivet_result test_aliasing_fetch(
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
        sizeof(page_about) - 1u > capacity) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    memset(buffer, 0x58, capacity);
    if (!bytes_equal(
            url,
            url_length,
            url_about,
            sizeof(url_about) - 1u)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    memcpy(
        buffer,
        page_about,
        sizeof(page_about) - 1u
    );
    *byte_count = sizeof(page_about) - 1u;
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

static rivet_layout_box *find_box_kind(
    rivet_browser *browser,
    rivet_layout_box_kind kind
)
{
    size_t i;

    for (i = 0u; i < browser->box_count; ++i) {
        if (browser->storage.boxes[i].kind == kind) {
            return &browser->storage.boxes[i];
        }
    }
    return NULL;
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
    unsigned char oversized_url[
        RIVET_BROWSER_URL_MAX + 1u
    ];
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
    CHECK(browser.document.source == NULL);
    CHECK(browser.document.nodes == NULL);
    CHECK(browser.document.source_bytes == 0u);
    CHECK(browser.document.node_capacity == 0u);
    CHECK(browser.document.node_count == 0u);
    CHECK(browser.document.requirements == 0u);
    CHECK(!browser.loaded);
    CHECK(!browser.source_mode);
    CHECK(rivet_browser_home(
        &browser) == RIVET_OK);
    CHECK(browser.loaded);
    CHECK(browser.history_count == 1u);
    CHECK(browser.history_index == 0u);
    CHECK(url_tail_zero(&browser.current_url));
    CHECK(url_tail_zero(&history[0]));
    CHECK(browser.selected_link_node !=
          RIVET_DOCUMENT_NO_PARENT);
    CHECK(find_red_text(&browser));

    {
        size_t selected = browser.selected_link_node;
        rivet_doc_slice saved_href =
            browser.document.nodes[selected].href;
        const unsigned char *saved_source =
            browser.document.source;
        size_t saved_source_bytes =
            browser.document.source_bytes;
        size_t fetch_before = io_state.fetch_count;
        size_t download_before = io_state.download_count;
        static const unsigned char prefix[] =
            "https://site.test/";

        CHECK(selected <
              browser.document.node_count);
        CHECK(sizeof(prefix) - 1u <
              sizeof(oversized_url));
        memcpy(
            oversized_url,
            prefix,
            sizeof(prefix) - 1u
        );
        memset(
            oversized_url + sizeof(prefix) - 1u,
            0x61,
            sizeof(oversized_url) -
                (sizeof(prefix) - 1u)
        );

        browser.document.source = oversized_url;
        browser.document.source_bytes =
            sizeof(oversized_url);
        browser.document.nodes[selected].href.offset = 0u;
        browser.document.nodes[selected].href.length =
            sizeof(oversized_url);

        CHECK(rivet_browser_download_selected(
            &browser) == RIVET_ERR_CAPACITY);
        CHECK(io_state.fetch_count == fetch_before);
        CHECK(io_state.download_count == download_before);

        browser.document.source = saved_source;
        browser.document.source_bytes =
            saved_source_bytes;
        browser.document.nodes[selected].href =
            saved_href;
    }

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
    CHECK(url_tail_zero(&browser.current_url));
    CHECK(url_tail_zero(&history[1]));
    CHECK(bytes_equal(
        browser.current_url.bytes,
        browser.current_url.length,
        url_about,
        sizeof(url_about) - 1u));

    CHECK(rivet_browser_bookmark_current(
        &browser) == RIVET_OK);
    CHECK(browser.bookmark_count == 1u);
    CHECK(url_tail_zero(&bookmarks[0]));
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

static int test_review_regressions(void)
{
    static const unsigned char config_bytes[] =
        "RIVET-WEB1 1\n"
        "home=https://site.test/home\n"
        "downloads=0\n"
        "user-css=\n";
    static const unsigned char crowded_css[] =
        "RIVET-WEB1 1\n"
        "home=https://site.test/home\n"
        "downloads=0\n"
        "user-css=p{color:#010203;}a{color:#040506;}\n";
    static const unsigned char bare_url_source[] =
        "https://site.test/home";
    unsigned char document_bytes[1024];
    unsigned char scratch_bytes[256];
    rivet_doc_node nodes[32];
    rivet_css_rule rules[16];
    rivet_layout_box boxes[64];
    rivet_browser_url history[8];
    rivet_browser_url bookmarks[4];
    rivet_browser_storage storage;
    rivet_browser_config config;
    rivet_browser_config crowded;
    rivet_browser_config aliased_config;
    rivet_browser_config malformed_config;
    rivet_browser_config tampered_config;
    rivet_browser_io io;
    rivet_browser browser;
    union {
        rivet_browser browser;
        unsigned char bytes[sizeof(rivet_browser)];
    } browser_alias;
    union {
        rivet_command_slot slots[32];
        unsigned char bytes[
            32u * sizeof(rivet_command_slot)
        ];
    } command_alias;
    test_io_state io_state;
    unsigned char pixels[120u * 64u * 4u];
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
    unsigned char long_url[RIVET_BROWSER_URL_MAX + 1u];
    size_t path_start =
        sizeof("https://site.test/") - 1u;
    size_t expected_rows;
    rivet_layout_box *image_box;
    rivet_layout_box *input_box;
    rivet_layout_box *text_box;
    size_t image_pixel;
    size_t input_pixel;
    size_t i;

    memset(&io_state, 0, sizeof(io_state));
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

    io.context = &io_state;
    io.fetch = test_fetch;
    io.download = NULL;

    CHECK(rivet_browser_config_parse(
        &config,
        config_bytes,
        sizeof(config_bytes) - 1u) == RIVET_OK);
    CHECK(rivet_browser_config_parse(
        &crowded,
        crowded_css,
        sizeof(crowded_css) - 1u) == RIVET_OK);

    memset(&malformed_config, 0, sizeof(malformed_config));
    malformed_config.source = bare_url_source;
    malformed_config.source_bytes =
        sizeof(bare_url_source) - 1u;
    malformed_config.home_url.offset = 0u;
    malformed_config.home_url.length =
        sizeof(bare_url_source) - 1u;
    malformed_config.user_css.offset =
        sizeof(bare_url_source) - 1u;
    malformed_config.user_css.length = 0u;
    malformed_config.downloads_enabled = 0;
    CHECK(rivet_browser_init(
        &browser,
        &io,
        &malformed_config,
        &storage,
        120ul) == RIVET_ERR_UNSUPPORTED);

    tampered_config = config;
    tampered_config.user_css.offset = 0u;
    tampered_config.user_css.length = 0u;
    CHECK(rivet_browser_init(
        &browser,
        &io,
        &tampered_config,
        &storage,
        120ul) == RIVET_ERR_INVALID_ARGUMENT);

    storage.rule_capacity = 1u;
    CHECK(rivet_browser_init(
        &browser,
        &io,
        &crowded,
        &storage,
        120ul) == RIVET_ERR_CAPACITY);
    storage.rule_capacity =
        sizeof(rules) / sizeof(rules[0]);

    {
        unsigned char *saved_scratch =
            storage.scratch_bytes;
        size_t saved_scratch_capacity =
            storage.scratch_capacity;
        rivet_browser_url *saved_bookmarks =
            storage.bookmarks;
        size_t saved_bookmark_capacity =
            storage.bookmark_capacity;

        storage.scratch_bytes =
            storage.document_bytes;
        storage.scratch_capacity =
            storage.document_capacity;
        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            120ul) == RIVET_ERR_INVALID_ARGUMENT);

        storage.scratch_bytes =
            storage.document_bytes + 1u;
        storage.scratch_capacity =
            storage.document_capacity - 1u;
        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            120ul) == RIVET_ERR_INVALID_ARGUMENT);

        storage.scratch_bytes = saved_scratch;
        storage.scratch_capacity =
            saved_scratch_capacity;

        storage.bookmarks = storage.history;
        storage.bookmark_capacity = 4u;
        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            120ul) == RIVET_ERR_INVALID_ARGUMENT);

        storage.bookmarks = storage.history + 1u;
        storage.bookmark_capacity = 4u;
        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            120ul) == RIVET_ERR_INVALID_ARGUMENT);

        storage.bookmarks = saved_bookmarks;
        storage.bookmark_capacity =
            saved_bookmark_capacity;

        storage.scratch_bytes =
            browser_alias.bytes;
        storage.scratch_capacity =
            sizeof(browser_alias.bytes);
        CHECK(rivet_browser_init(
            &browser_alias.browser,
            &io,
            &config,
            &storage,
            120ul) == RIVET_ERR_INVALID_ARGUMENT);

        storage.scratch_bytes = saved_scratch;
        storage.scratch_capacity =
            saved_scratch_capacity;
    }

    CHECK(sizeof(config_bytes) - 1u <=
          sizeof(document_bytes));
    memcpy(
        document_bytes,
        config_bytes,
        sizeof(config_bytes) - 1u
    );
    CHECK(rivet_browser_config_parse(
        &aliased_config,
        document_bytes,
        sizeof(config_bytes) - 1u) == RIVET_OK);
    CHECK(rivet_browser_init(
        &browser,
        &io,
        &aliased_config,
        &storage,
        120ul) == RIVET_ERR_INVALID_ARGUMENT);

    if (ULONG_MAX > (unsigned long)LONG_MAX) {
        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            (unsigned long)LONG_MAX + 1ul) ==
            RIVET_ERR_CAPACITY);
    }

    if (sizeof(size_t) <= sizeof(unsigned long)) {
        size_t size_max = (size_t)-1;
        unsigned long row_width_limit =
            (unsigned long)(
                size_max /
                RIVET_GFX_PIXEL_BYTES
            );

        if (row_width_limit <
            (unsigned long)LONG_MAX) {
            CHECK(rivet_browser_init(
                &browser,
                &io,
                &config,
                &storage,
                row_width_limit + 1ul) ==
                RIVET_ERR_CAPACITY);
        }
    }

    CHECK(rivet_browser_init(
        &browser,
        &io,
        &config,
        &storage,
        120ul) == RIVET_OK);
    CHECK(rivet_browser_home(&browser) == RIVET_OK);

    {
        rivet_browser alias_browser;
        rivet_browser_storage alias_storage = storage;
        rivet_command_registry alias_commands;

        alias_storage.document_bytes =
            command_alias.bytes;
        alias_storage.document_capacity =
            sizeof(command_alias.bytes);

        CHECK(rivet_browser_init(
            &alias_browser,
            &io,
            &config,
            &alias_storage,
            120ul) == RIVET_OK);
        CHECK(rivet_browser_home(
            &alias_browser) == RIVET_OK);
        CHECK(rivet_commands_init(
            &alias_commands,
            command_alias.slots,
            sizeof(command_alias.slots) /
                sizeof(command_alias.slots[0])) ==
            RIVET_OK);
        CHECK(rivet_browser_register_commands(
            &alias_browser,
            &alias_commands) ==
            RIVET_ERR_INVALID_ARGUMENT);
        CHECK(alias_browser.loaded);
        CHECK(rivet_browser_open_selected_link(
            &alias_browser) == RIVET_OK);
    }

    browser.io.fetch = test_bad_layout_fetch;
    CHECK(rivet_browser_reload(&browser) ==
          RIVET_ERR_UNSUPPORTED);
    CHECK(!browser.loaded);
    CHECK(browser.document.source == NULL);
    CHECK(browser.document.nodes == NULL);
    CHECK(browser.document.source_bytes == 0u);
    CHECK(browser.document.node_capacity == 0u);
    CHECK(browser.document.node_count == 0u);
    CHECK(browser.document.requirements == 0u);
    CHECK(browser.document_bytes == 0u);
    CHECK(browser.rule_count == 0u);
    CHECK(browser.box_count == 0u);
    CHECK(browser.document_height == 0ul);
    CHECK(bytes_equal(
        browser.current_url.bytes,
        browser.current_url.length,
        url_home,
        sizeof(url_home) - 1u));

    browser.io.fetch = test_zero_success_fetch;
    CHECK(rivet_browser_reload(&browser) ==
          RIVET_ERR_UNSUPPORTED);
    CHECK(!browser.loaded);
    CHECK(browser.document.source == NULL);
    CHECK(browser.document.nodes == NULL);
    CHECK(bytes_equal(
        browser.current_url.bytes,
        browser.current_url.length,
        url_home,
        sizeof(url_home) - 1u));

    browser.io.fetch = test_fetch;
    CHECK(rivet_browser_reload(&browser) == RIVET_OK);
    CHECK(browser.loaded);
    CHECK(bytes_equal(
        browser.current_url.bytes,
        browser.current_url.length,
        url_home,
        sizeof(url_home) - 1u));

    CHECK(rivet_browser_open_selected_link(NULL) ==
          RIVET_ERR_INVALID_ARGUMENT);

    {
        size_t saved_node_count =
            browser.document.node_count;
        size_t saved_selected =
            browser.selected_link_node;
        const unsigned char *saved_source =
            browser.document.source;
        size_t saved_source_bytes =
            browser.document.source_bytes;
        size_t url_prefix =
            sizeof("https://site.test/") - 1u;

        memcpy(
            long_url,
            "https://site.test/",
            url_prefix
        );
        memset(
            long_url + url_prefix,
            0x61,
            RIVET_BROWSER_URL_MAX + 1u -
            url_prefix
        );

        nodes[0].kind = RIVET_DOC_NODE_A;
        nodes[0].href.offset = 0u;
        nodes[0].href.length =
            RIVET_BROWSER_URL_MAX + 1u;
        browser.document.source = long_url;
        browser.document.source_bytes =
            RIVET_BROWSER_URL_MAX + 1u;
        browser.document.node_count = 1u;
        browser.selected_link_node = 0u;

        CHECK(rivet_browser_open_selected_link(
            &browser) == RIVET_ERR_CAPACITY);

        browser.document.source = saved_source;
        browser.document.source_bytes =
            saved_source_bytes;
        browser.document.node_count =
            saved_node_count;
        browser.selected_link_node =
            saved_selected;
    }

    memcpy(
        long_url,
        "https://site.test/",
        path_start
    );
    memset(
        long_url + path_start,
        0x61,
        sizeof(long_url) - path_start
    );
    CHECK(rivet_browser_open(
        &browser,
        long_url,
        sizeof(long_url)) == RIVET_ERR_CAPACITY);
    CHECK(browser.loaded);

    CHECK(rivet_browser_toggle_source(&browser) == RIVET_OK);
    expected_rows =
        (browser.document_bytes +
         (size_t)(120ul / 6ul) - 1u) /
        (size_t)(120ul / 6ul);
    while (rivet_browser_scroll_down(&browser) == RIVET_OK) {
    }
    CHECK(browser.scroll_y ==
          (unsigned long)expected_rows * 8ul);
    CHECK(rivet_browser_toggle_source(&browser) == RIVET_OK);

    {
        const rivet_doc_node *link =
            &browser.document.nodes[
                browser.selected_link_node];
        const unsigned char *aliased_url =
            browser.document.source +
            link->href.offset;
        size_t aliased_length = link->href.length;

        browser.io.fetch = test_aliasing_fetch;
        CHECK(rivet_browser_open(
            &browser,
            aliased_url,
            aliased_length) == RIVET_OK);
        CHECK(bytes_equal(
            browser.current_url.bytes,
            browser.current_url.length,
            url_about,
            sizeof(url_about) - 1u));
    }

    browser.io.fetch = test_fetch;
    CHECK(rivet_browser_home(&browser) == RIVET_OK);
    browser.io.fetch = test_zero_success_fetch;
    CHECK(rivet_browser_open(
        &browser,
        url_about,
        sizeof(url_about) - 1u) ==
        RIVET_ERR_UNSUPPORTED);
    CHECK(!browser.loaded);

    browser.io.fetch = test_fetch;
    CHECK(rivet_browser_home(&browser) == RIVET_OK);
    browser.io.fetch = test_oversize_success_fetch;
    CHECK(rivet_browser_open(
        &browser,
        url_about,
        sizeof(url_about) - 1u) ==
        RIVET_ERR_CAPACITY);
    CHECK(!browser.loaded);

    browser.io.fetch = test_fetch;
    CHECK(rivet_browser_open(
        &browser,
        url_replaced,
        sizeof(url_replaced) - 1u) == RIVET_OK);
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

    image_box =
        find_box_kind(&browser, RIVET_LAYOUT_IMAGE);
    input_box =
        find_box_kind(&browser, RIVET_LAYOUT_INPUT);
    text_box =
        find_box_kind(&browser, RIVET_LAYOUT_TEXT);
    CHECK(image_box != NULL);
    CHECK(input_box != NULL);
    CHECK(text_box != NULL);
    CHECK(image_box->has_background);
    CHECK(input_box->has_background);

    image_pixel =
        ((size_t)(
             image_box->y +
             RIVET_BROWSER_CHROME_HEIGHT) *
         120u +
         (size_t)image_box->x) *
        RIVET_GFX_PIXEL_BYTES;
    input_pixel =
        ((size_t)(
             input_box->y +
             RIVET_BROWSER_CHROME_HEIGHT) *
         120u +
         (size_t)input_box->x) *
        RIVET_GFX_PIXEL_BYTES;

    CHECK(pixels[image_pixel] == 0x11u);
    CHECK(pixels[image_pixel + 1u] == 0x22u);
    CHECK(pixels[image_pixel + 2u] == 0x33u);
    CHECK(pixels[input_pixel] == 0x44u);
    CHECK(pixels[input_pixel + 1u] == 0x55u);
    CHECK(pixels[input_pixel + 2u] == 0x66u);

    text_box->y = (unsigned long)LONG_MAX;
    image_box->y = (unsigned long)LONG_MAX;
    browser.scroll_y = (unsigned long)LONG_MAX;
    CHECK(rivet_browser_render(
        &surface,
        &browser,
        bounds,
        style) == RIVET_OK);
    browser.scroll_y = 0ul;

    if (ULONG_MAX > (unsigned long)LONG_MAX) {
        text_box->y =
            (unsigned long)LONG_MAX + 1ul;
        CHECK(rivet_browser_render(
            &surface,
            &browser,
            bounds,
            style) == RIVET_OK);
    }

    {
        unsigned long negative_limit =
            (unsigned long)(-(LONG_MIN + 1L)) + 1ul;

        if (negative_limit <=
            ULONG_MAX -
                RIVET_BROWSER_CHROME_HEIGHT -
                1ul) {
            for (i = 0u; i < browser.box_count; ++i) {
                browser.storage.boxes[i].y = 0ul;
            }
            browser.scroll_y =
                negative_limit +
                RIVET_BROWSER_CHROME_HEIGHT +
                1ul;
            CHECK(rivet_browser_render(
                &surface,
                &browser,
                bounds,
                style) == RIVET_OK);
            browser.scroll_y = 0ul;
        }
    }

    {
        unsigned char narrow_pixels[40u * 32u * 4u];
        rivet_surface narrow_surface;
        rivet_rect narrow_bounds =
            {5L,0L,12ul,32ul};

        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            12ul) == RIVET_OK);
        CHECK(rivet_browser_home(&browser) == RIVET_OK);
        memset(
            narrow_pixels,
            0x7b,
            sizeof(narrow_pixels)
        );
        CHECK(rivet_surface_attach(
            &narrow_surface,
            narrow_pixels,
            sizeof(narrow_pixels),
            40ul,
            32ul,
            40u * 4u) == RIVET_OK);
        CHECK(rivet_browser_render(
            &narrow_surface,
            &browser,
            narrow_bounds,
            style) == RIVET_OK);

        for (i = 0u;
             i < (size_t)RIVET_BROWSER_CHROME_HEIGHT;
             ++i) {
            size_t x;
            for (x = 17u; x < 40u; ++x) {
                size_t pixel =
                    (i * 40u + x) *
                    RIVET_GFX_PIXEL_BYTES;
                CHECK(narrow_pixels[pixel] == 0x7bu);
                CHECK(narrow_pixels[pixel + 1u] == 0x7bu);
                CHECK(narrow_pixels[pixel + 2u] == 0x7bu);
                CHECK(narrow_pixels[pixel + 3u] == 0x7bu);
            }
        }
    }

    {
        unsigned char tiny_pixels[5u * 16u * 4u];
        rivet_surface tiny_surface;
        rivet_rect tiny_bounds =
            {0L,0L,5ul,16ul};

        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            5ul) == RIVET_OK);
        browser.io.fetch = test_empty_fetch;
        CHECK(rivet_browser_home(&browser) == RIVET_OK);
        CHECK(rivet_browser_toggle_source(
            &browser) == RIVET_OK);
        CHECK(rivet_surface_attach(
            &tiny_surface,
            tiny_pixels,
            sizeof(tiny_pixels),
            5ul,
            16ul,
            5u * 4u) == RIVET_OK);
        CHECK(rivet_browser_render(
            &tiny_surface,
            &browser,
            tiny_bounds,
            style) == RIVET_OK);
    }

    {
        rivet_surface aliased_surface;
        rivet_rect aliased_bounds =
            {0L,0L,20ul,10ul};

        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            20ul) == RIVET_OK);
        browser.io.fetch = test_fetch;
        CHECK(rivet_browser_home(&browser) == RIVET_OK);
        CHECK(rivet_surface_attach(
            &aliased_surface,
            document_bytes,
            sizeof(document_bytes),
            20ul,
            10ul,
            20u * RIVET_GFX_PIXEL_BYTES) == RIVET_OK);
        CHECK(rivet_browser_render(
            &aliased_surface,
            &browser,
            aliased_bounds,
            style) == RIVET_ERR_INVALID_ARGUMENT);
        CHECK(browser.loaded);
        CHECK(bytes_equal(
            browser.current_url.bytes,
            browser.current_url.length,
            url_home,
            sizeof(url_home) - 1u));
    }

    {
        union {
            rivet_surface surface;
            unsigned char pixels[
                20u * 10u * RIVET_GFX_PIXEL_BYTES
            ];
        } self_aliased;
        rivet_rect self_bounds =
            {0L,0L,20ul,10ul};

        CHECK(rivet_browser_init(
            &browser,
            &io,
            &config,
            &storage,
            20ul) == RIVET_OK);
        browser.io.fetch = test_fetch;
        CHECK(rivet_browser_home(&browser) == RIVET_OK);
        CHECK(rivet_surface_attach(
            &self_aliased.surface,
            self_aliased.pixels,
            sizeof(self_aliased.pixels),
            20ul,
            10ul,
            20u * RIVET_GFX_PIXEL_BYTES) == RIVET_OK);
        CHECK(rivet_browser_render(
            &self_aliased.surface,
            &browser,
            self_bounds,
            style) == RIVET_ERR_INVALID_ARGUMENT);
        CHECK(browser.loaded);
    }

    return 0;
}

static int test_config_failures(void)
{
    static const unsigned char valid_config[] =
        "RIVET-WEB1 1\n"
        "home=https://site.test/home\n"
        "downloads=0\n"
        "user-css=\n";
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
    union {
        rivet_browser_config config;
        unsigned char bytes[128];
    } aliased;
    size_t cursor;
    size_t path_bytes;
    rivet_browser_config config;

    CHECK(sizeof(valid_config) - 1u <=
          sizeof(aliased.bytes));
    memcpy(
        aliased.bytes,
        valid_config,
        sizeof(valid_config) - 1u
    );
    CHECK(rivet_browser_config_parse(
        &aliased.config,
        aliased.bytes,
        sizeof(valid_config) - 1u) ==
        RIVET_ERR_INVALID_ARGUMENT);
    CHECK(bytes_equal(
        aliased.bytes,
        sizeof(valid_config) - 1u,
        valid_config,
        sizeof(valid_config) - 1u));

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
    CHECK(test_review_regressions() == 0);
    CHECK(test_config_failures() == 0);

    puts("rivet browser tests: ok");
    return 0;
}
