/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/browser.h"

#include <limits.h>
#include <string.h>

#define BROWSER_GLYPH_WIDTH 5ul
#define BROWSER_GLYPH_HEIGHT 7ul
#define BROWSER_GLYPH_ADVANCE 6ul
#define BROWSER_LINE_HEIGHT 8ul
#define BROWSER_SCROLL_STEP 8ul

static rivet_result browser_utf8_one(
    const unsigned char *bytes,
    size_t byte_count,
    size_t offset,
    unsigned int *codepoint,
    size_t *used
);

static int browser_space(unsigned int codepoint)
{
    return codepoint == 0x09u ||
           codepoint == 0x0au ||
           codepoint == 0x0cu ||
           codepoint == 0x0du ||
           codepoint == 0x20u;
}

static int browser_bytes_equal(
    const unsigned char *left,
    size_t left_count,
    const unsigned char *right,
    size_t right_count
)
{
    size_t i;

    if (left_count != right_count) {
        return 0;
    }
    for (i = 0u; i < left_count; ++i) {
        if (left[i] != right[i]) {
            return 0;
        }
    }
    return 1;
}

static int browser_prefix(
    const unsigned char *bytes,
    size_t byte_count,
    size_t offset,
    const unsigned char *prefix,
    size_t prefix_count
)
{
    if (offset > byte_count ||
        prefix_count > byte_count - offset) {
        return 0;
    }

    return browser_bytes_equal(
        bytes + offset,
        prefix_count,
        prefix,
        prefix_count
    );
}

static rivet_result browser_line(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *cursor,
    size_t *start,
    size_t *length
)
{
    size_t begin;
    size_t pos;

    if (bytes == NULL || cursor == NULL ||
        start == NULL || length == NULL ||
        *cursor > byte_count) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    begin = *cursor;
    pos = begin;
    while (pos < byte_count &&
           bytes[pos] != 0x0au) {
        if (bytes[pos] == 0u ||
            bytes[pos] == 0x0du) {
            return RIVET_ERR_UNSUPPORTED;
        }
        ++pos;
    }

    if (pos >= byte_count) {
        return RIVET_ERR_UNSUPPORTED;
    }

    *start = begin;
    *length = pos - begin;
    *cursor = pos + 1u;
    return RIVET_OK;
}

static rivet_result browser_validate_css(
    const unsigned char *bytes,
    size_t byte_count,
    size_t *total_rule_count
)
{
    size_t start = 0u;
    size_t i;
    size_t rule_count = 0u;
    size_t total = 0u;
    rivet_css_rule rule;
    rivet_result result;

    if (byte_count == 0u) {
        if (total_rule_count != NULL) {
            *total_rule_count = 0u;
        }
        return RIVET_OK;
    }
    if (bytes == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    /*
     * Document v1 CSS has no strings, comments, nested rules or
     * declaration values containing '}'. Validate one complete rule
     * at a time through the frozen R7 parser so WEB1 does not duplicate
     * the CSS grammar or require unbounded temporary rule storage.
     */
    for (i = 0u; i < byte_count; ++i) {
        if (bytes[i] != 0x7du) {
            continue;
        }

        result = rivet_css_parse(
            bytes + start,
            i + 1u - start,
            &rule,
            1u,
            &rule_count
        );
        if (result != RIVET_OK) {
            return result;
        }
        if (rule_count != 1u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        if (total == (size_t)-1) {
            return RIVET_ERR_CAPACITY;
        }
        ++total;
        start = i + 1u;
    }

    result = rivet_css_parse(
        bytes + start,
        byte_count - start,
        NULL,
        0u,
        &rule_count
    );
    if (result != RIVET_OK) {
        return result;
    }
    if (rule_count != 0u) {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (total_rule_count != NULL) {
        *total_rule_count = total;
    }
    return RIVET_OK;
}

rivet_result rivet_browser_config_parse(
    rivet_browser_config *config,
    const unsigned char *bytes,
    size_t byte_count
)
{
    static const unsigned char magic[] = {
        0x52u,0x49u,0x56u,0x45u,0x54u,0x2du,0x57u,0x45u,
        0x42u,0x31u,0x20u,0x31u
    };
    static const unsigned char home[] = {
        0x68u,0x6fu,0x6du,0x65u,0x3du
    };
    static const unsigned char downloads[] = {
        0x64u,0x6fu,0x77u,0x6eu,0x6cu,0x6fu,0x61u,0x64u,
        0x73u,0x3du
    };
    static const unsigned char user_css[] = {
        0x75u,0x73u,0x65u,0x72u,0x2du,0x63u,0x73u,0x73u,
        0x3du
    };
    size_t cursor = 0u;
    size_t start;
    size_t length;
    rivet_browser_config candidate;
    rivet_url parsed;
    rivet_result result;

    if (config == NULL || bytes == NULL ||
        byte_count == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    result = browser_line(
        bytes, byte_count, &cursor,
        &start, &length
    );
    if (result != RIVET_OK ||
        !browser_bytes_equal(
            bytes + start,
            length,
            magic,
            sizeof(magic))) {
        return RIVET_ERR_UNSUPPORTED;
    }

    result = browser_line(
        bytes, byte_count, &cursor,
        &start, &length
    );
    if (result != RIVET_OK ||
        length <= sizeof(home) ||
        !browser_prefix(
            bytes, byte_count, start,
            home, sizeof(home))) {
        return RIVET_ERR_UNSUPPORTED;
    }
    candidate.home_url.offset =
        start + sizeof(home);
    candidate.home_url.length =
        length - sizeof(home);
    if (candidate.home_url.length >
        RIVET_BROWSER_URL_MAX) {
        return RIVET_ERR_CAPACITY;
    }

    result = rivet_url_parse(
        &parsed,
        bytes + candidate.home_url.offset,
        candidate.home_url.length
    );
    if (result != RIVET_OK) {
        return result;
    }

    result = browser_line(
        bytes, byte_count, &cursor,
        &start, &length
    );
    if (result != RIVET_OK ||
        length != sizeof(downloads) + 1u ||
        !browser_prefix(
            bytes, byte_count, start,
            downloads, sizeof(downloads)) ||
        (bytes[start + sizeof(downloads)] != 0x30u &&
         bytes[start + sizeof(downloads)] != 0x31u)) {
        return RIVET_ERR_UNSUPPORTED;
    }
    candidate.downloads_enabled =
        bytes[start + sizeof(downloads)] == 0x31u;

    result = browser_line(
        bytes, byte_count, &cursor,
        &start, &length
    );
    if (result != RIVET_OK ||
        length < sizeof(user_css) ||
        !browser_prefix(
            bytes, byte_count, start,
            user_css, sizeof(user_css))) {
        return RIVET_ERR_UNSUPPORTED;
    }
    candidate.user_css.offset =
        start + sizeof(user_css);
    candidate.user_css.length =
        length - sizeof(user_css);

    result = browser_validate_css(
        bytes + candidate.user_css.offset,
        candidate.user_css.length,
        NULL
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (cursor != byte_count) {
        return RIVET_ERR_UNSUPPORTED;
    }

    candidate.source = bytes;
    candidate.source_bytes = byte_count;
    *config = candidate;
    return RIVET_OK;
}

static rivet_result browser_copy_url(
    rivet_browser_url *target,
    const unsigned char *url,
    size_t url_length
)
{
    rivet_url parsed;

    if (target == NULL || url == NULL ||
        url_length == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (url_length > RIVET_BROWSER_URL_MAX) {
        return RIVET_ERR_CAPACITY;
    }

    if (rivet_url_parse(
            &parsed,
            url,
            url_length) != RIVET_OK) {
        return RIVET_ERR_UNSUPPORTED;
    }

    memcpy(target->bytes, url, url_length);
    target->length = url_length;
    return RIVET_OK;
}

static int browser_url_equal(
    const rivet_browser_url *entry,
    const unsigned char *url,
    size_t url_length
)
{
    return entry != NULL &&
           entry->length == url_length &&
           browser_bytes_equal(
               entry->bytes,
               entry->length,
               url,
               url_length
           );
}

static rivet_result browser_storage_validate(
    const rivet_browser_storage *storage
)
{
    if (storage == NULL ||
        storage->document_bytes == NULL ||
        storage->document_capacity == 0u ||
        storage->nodes == NULL ||
        storage->node_capacity == 0u ||
        storage->rules == NULL ||
        storage->rule_capacity == 0u ||
        storage->boxes == NULL ||
        storage->box_capacity == 0u ||
        storage->history == NULL ||
        storage->history_capacity == 0u ||
        storage->bookmarks == NULL ||
        storage->bookmark_capacity == 0u ||
        storage->scratch_bytes == NULL ||
        storage->scratch_capacity == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    return RIVET_OK;
}

static rivet_result browser_config_validate(
    const rivet_browser_config *config
)
{
    rivet_url parsed;
    rivet_result result;

    if (config == NULL ||
        config->source == NULL ||
        config->source_bytes == 0u ||
        config->home_url.offset >
            config->source_bytes ||
        config->home_url.length >
            config->source_bytes -
            config->home_url.offset ||
        config->user_css.offset >
            config->source_bytes ||
        config->user_css.length >
            config->source_bytes -
            config->user_css.offset ||
        (config->downloads_enabled != 0 &&
         config->downloads_enabled != 1)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (config->home_url.length >
        RIVET_BROWSER_URL_MAX) {
        return RIVET_ERR_CAPACITY;
    }

    result = rivet_url_parse(
        &parsed,
        config->source + config->home_url.offset,
        config->home_url.length
    );
    if (result != RIVET_OK) {
        return result;
    }

    return browser_validate_css(
        config->source + config->user_css.offset,
        config->user_css.length,
        NULL
    );
}

rivet_result rivet_browser_init(
    rivet_browser *browser,
    const rivet_browser_io *io,
    const rivet_browser_config *config,
    const rivet_browser_storage *storage,
    unsigned long viewport_width
)
{
    size_t user_rule_count = 0u;
    rivet_result result;

    if (browser == NULL || io == NULL ||
        io->fetch == NULL ||
        viewport_width == 0ul) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (viewport_width >
        (unsigned long)LONG_MAX) {
        return RIVET_ERR_CAPACITY;
    }

    result = browser_config_validate(config);
    if (result != RIVET_OK) {
        return result;
    }
    if (config->downloads_enabled &&
        io->download == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    result = browser_storage_validate(storage);
    if (result != RIVET_OK) {
        return result;
    }
    result = browser_validate_css(
        config->source + config->user_css.offset,
        config->user_css.length,
        &user_rule_count
    );
    if (result != RIVET_OK) {
        return result;
    }
    if (user_rule_count >
        storage->rule_capacity) {
        return RIVET_ERR_CAPACITY;
    }

    memset(browser, 0, sizeof(*browser));
    browser->io = *io;
    browser->config = *config;
    browser->storage = *storage;
    browser->viewport_width = viewport_width;
    browser->selected_link_node =
        RIVET_DOCUMENT_NO_PARENT;
    return RIVET_OK;
}

static rivet_result browser_append_css(
    rivet_browser *browser,
    const unsigned char *bytes,
    size_t byte_count
)
{
    size_t count = 0u;
    size_t remaining;
    rivet_result result;

    if (byte_count == 0u) {
        return RIVET_OK;
    }
    if (bytes == NULL ||
        browser->rule_count >
            browser->storage.rule_capacity) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    remaining =
        browser->storage.rule_capacity -
        browser->rule_count;
    result = rivet_css_parse(
        bytes,
        byte_count,
        browser->storage.rules +
            browser->rule_count,
        remaining,
        &count
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (count >
        browser->storage.rule_capacity -
        browser->rule_count) {
        return RIVET_ERR_CAPACITY;
    }
    browser->rule_count += count;
    return RIVET_OK;
}

static rivet_result browser_collect_css(
    rivet_browser *browser
)
{
    size_t i;

    browser->rule_count = 0u;

    for (i = 0u;
         i < browser->document.node_count;
         ++i) {
        const rivet_doc_node *node =
            &browser->document.nodes[i];
        size_t j;

        if (node->kind !=
            RIVET_DOC_NODE_STYLE) {
            continue;
        }

        for (j = i + 1u;
             j < browser->document.node_count;
             ++j) {
            const rivet_doc_node *child =
                &browser->document.nodes[j];

            if (child->parent != i) {
                if (child->parent < i) {
                    break;
                }
                continue;
            }
            if (child->kind !=
                RIVET_DOC_NODE_TEXT) {
                return RIVET_ERR_UNSUPPORTED;
            }
            {
                rivet_result result =
                    browser_append_css(
                        browser,
                        browser->document.source +
                            child->text.offset,
                        child->text.length
                    );
                if (result != RIVET_OK) {
                    return result;
                }
            }
        }
    }

    return browser_append_css(
        browser,
        browser->config.source +
            browser->config.user_css.offset,
        browser->config.user_css.length
    );
}

static size_t browser_first_link(
    const rivet_browser *browser
)
{
    size_t i;

    for (i = 0u;
         i < browser->document.node_count;
         ++i) {
        const rivet_doc_node *node =
            &browser->document.nodes[i];
        if (node->kind == RIVET_DOC_NODE_A &&
            node->href.length != 0u) {
            return i;
        }
    }
    return RIVET_DOCUMENT_NO_PARENT;
}

static rivet_result browser_layout(
    rivet_browser *browser
)
{
    rivet_result result;

    result = browser_collect_css(browser);
    if (result != RIVET_OK) {
        return result;
    }

    result = rivet_document_layout(
        &browser->document,
        browser->storage.rules,
        browser->rule_count,
        browser->viewport_width,
        browser->storage.boxes,
        browser->storage.box_capacity,
        &browser->box_count,
        &browser->document_height
    );
    if (result != RIVET_OK) {
        return result;
    }

    browser->scroll_y = 0ul;
    browser->selected_link_node =
        browser_first_link(browser);
    return RIVET_OK;
}

static void browser_invalidate_loaded(
    rivet_browser *browser
)
{
    browser->loaded = 0;
    browser->document_bytes = 0u;
    browser->rule_count = 0u;
    browser->box_count = 0u;
    browser->document_height = 0ul;
    browser->scroll_y = 0ul;
    browser->selected_link_node =
        RIVET_DOCUMENT_NO_PARENT;
}

static rivet_result browser_load(
    rivet_browser *browser,
    const unsigned char *url,
    size_t url_length,
    int record_history
)
{
    rivet_browser_url candidate_url;
    rivet_document candidate_document;
    size_t byte_count = 0u;
    size_t next_history_count =
        browser->history_count;
    size_t next_history_index =
        browser->history_index;
    rivet_result result;

    result = browser_copy_url(
        &candidate_url,
        url,
        url_length
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (record_history) {
        if (browser->history_count != 0u) {
            next_history_count =
                browser->history_index + 1u;
        }
        if (next_history_count >=
            browser->storage.history_capacity) {
            return RIVET_ERR_CAPACITY;
        }
        next_history_index =
            next_history_count;
    }

    result = browser->io.fetch(
        browser->io.context,
        candidate_url.bytes,
        candidate_url.length,
        browser->storage.document_bytes,
        browser->storage.document_capacity,
        &byte_count
    );
    if (result != RIVET_OK) {
        browser_invalidate_loaded(browser);
        return result;
    }
    if (byte_count == 0u) {
        browser_invalidate_loaded(browser);
        return RIVET_ERR_UNSUPPORTED;
    }
    if (byte_count >
        browser->storage.document_capacity) {
        browser_invalidate_loaded(browser);
        return RIVET_ERR_CAPACITY;
    }

    result = rivet_html_parse(
        &candidate_document,
        browser->storage.document_bytes,
        byte_count,
        browser->storage.nodes,
        browser->storage.node_capacity
    );
    if (result != RIVET_OK) {
        browser_invalidate_loaded(browser);
        return result;
    }

    browser->document = candidate_document;
    browser->document_bytes = byte_count;

    result = browser_layout(browser);
    if (result != RIVET_OK) {
        browser_invalidate_loaded(browser);
        return result;
    }

    browser->current_url = candidate_url;
    browser->source_mode = 0;
    browser->loaded = 1;

    if (record_history) {
        browser->storage.history[
            next_history_index] =
            candidate_url;
        browser->history_index =
            next_history_index;
        browser->history_count =
            next_history_index + 1u;
    }

    return RIVET_OK;
}

rivet_result rivet_browser_open(
    rivet_browser *browser,
    const unsigned char *url,
    size_t url_length
)
{
    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    return browser_load(
        browser,
        url,
        url_length,
        1
    );
}

rivet_result rivet_browser_home(rivet_browser *browser)
{
    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    return rivet_browser_open(
        browser,
        browser->config.source +
            browser->config.home_url.offset,
        browser->config.home_url.length
    );
}

rivet_result rivet_browser_back(rivet_browser *browser)
{
    size_t target;
    rivet_result result;

    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (browser->history_count == 0u ||
        browser->history_index == 0u) {
        return RIVET_ERR_NOT_FOUND;
    }

    target = browser->history_index - 1u;
    result = browser_load(
        browser,
        browser->storage.history[target].bytes,
        browser->storage.history[target].length,
        0
    );
    if (result == RIVET_OK) {
        browser->history_index = target;
    }
    return result;
}

rivet_result rivet_browser_forward(rivet_browser *browser)
{
    size_t target;
    rivet_result result;

    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (browser->history_count == 0u ||
        browser->history_index + 1u >=
            browser->history_count) {
        return RIVET_ERR_NOT_FOUND;
    }

    target = browser->history_index + 1u;
    result = browser_load(
        browser,
        browser->storage.history[target].bytes,
        browser->storage.history[target].length,
        0
    );
    if (result == RIVET_OK) {
        browser->history_index = target;
    }
    return result;
}

rivet_result rivet_browser_reload(rivet_browser *browser)
{
    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!browser->loaded) {
        return RIVET_ERR_NOT_FOUND;
    }
    return browser_load(
        browser,
        browser->current_url.bytes,
        browser->current_url.length,
        0
    );
}

rivet_result rivet_browser_next_link(rivet_browser *browser)
{
    size_t start;
    size_t i;

    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!browser->loaded) {
        return RIVET_ERR_NOT_FOUND;
    }

    start =
        browser->selected_link_node ==
            RIVET_DOCUMENT_NO_PARENT ?
        0u :
        browser->selected_link_node + 1u;

    for (i = start;
         i < browser->document.node_count;
         ++i) {
        const rivet_doc_node *node =
            &browser->document.nodes[i];
        if (node->kind == RIVET_DOC_NODE_A &&
            node->href.length != 0u) {
            browser->selected_link_node = i;
            return RIVET_OK;
        }
    }

    for (i = 0u;
         i < start &&
         i < browser->document.node_count;
         ++i) {
        const rivet_doc_node *node =
            &browser->document.nodes[i];
        if (node->kind == RIVET_DOC_NODE_A &&
            node->href.length != 0u) {
            browser->selected_link_node = i;
            return RIVET_OK;
        }
    }

    browser->selected_link_node =
        RIVET_DOCUMENT_NO_PARENT;
    return RIVET_ERR_NOT_FOUND;
}

static const rivet_doc_node *browser_selected_link(
    const rivet_browser *browser
)
{
    if (browser == NULL ||
        !browser->loaded ||
        browser->selected_link_node ==
            RIVET_DOCUMENT_NO_PARENT ||
        browser->selected_link_node >=
            browser->document.node_count ||
        browser->document.nodes[
            browser->selected_link_node].kind !=
            RIVET_DOC_NODE_A) {
        return NULL;
    }

    return &browser->document.nodes[
        browser->selected_link_node];
}

rivet_result rivet_browser_open_selected_link(
    rivet_browser *browser
)
{
    const rivet_doc_node *link;
    unsigned char url[RIVET_BROWSER_URL_MAX];
    size_t length;

    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    link = browser_selected_link(browser);
    if (link == NULL) {
        return RIVET_ERR_NOT_FOUND;
    }
    if (link->href.length == 0u) {
        return RIVET_ERR_UNSUPPORTED;
    }
    if (link->href.length > sizeof(url)) {
        return RIVET_ERR_CAPACITY;
    }

    length = link->href.length;
    memcpy(
        url,
        browser->document.source +
            link->href.offset,
        length
    );

    return rivet_browser_open(
        browser,
        url,
        length
    );
}

rivet_result rivet_browser_bookmark_current(
    rivet_browser *browser
)
{
    size_t i;

    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!browser->loaded) {
        return RIVET_ERR_NOT_FOUND;
    }

    for (i = 0u;
         i < browser->bookmark_count;
         ++i) {
        if (browser_url_equal(
                &browser->storage.bookmarks[i],
                browser->current_url.bytes,
                browser->current_url.length)) {
            return RIVET_ERR_DUPLICATE;
        }
    }

    if (browser->bookmark_count >=
        browser->storage.bookmark_capacity) {
        return RIVET_ERR_CAPACITY;
    }

    browser->storage.bookmarks[
        browser->bookmark_count] =
        browser->current_url;
    ++browser->bookmark_count;
    return RIVET_OK;
}

rivet_result rivet_browser_download_selected(
    rivet_browser *browser
)
{
    const rivet_doc_node *link =
        browser_selected_link(browser);
    const unsigned char *url;
    size_t byte_count = 0u;
    rivet_url parsed;
    rivet_result result;

    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!browser->config.downloads_enabled ||
        browser->io.download == NULL) {
        return RIVET_ERR_UNSUPPORTED;
    }
    if (link == NULL || link->href.length == 0u) {
        return RIVET_ERR_NOT_FOUND;
    }

    url = browser->document.source +
          link->href.offset;

    result = rivet_url_parse(
        &parsed,
        url,
        link->href.length
    );
    if (result != RIVET_OK) {
        return result;
    }

    result = browser->io.fetch(
        browser->io.context,
        url,
        link->href.length,
        browser->storage.scratch_bytes,
        browser->storage.scratch_capacity,
        &byte_count
    );
    if (result != RIVET_OK) {
        return result;
    }
    if (byte_count >
        browser->storage.scratch_capacity) {
        return RIVET_ERR_CAPACITY;
    }

    return browser->io.download(
        browser->io.context,
        url,
        link->href.length,
        browser->storage.scratch_bytes,
        byte_count
    );
}

rivet_result rivet_browser_toggle_source(
    rivet_browser *browser
)
{
    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!browser->loaded) {
        return RIVET_ERR_NOT_FOUND;
    }
    browser->source_mode =
        browser->source_mode ? 0 : 1;
    browser->scroll_y = 0ul;
    return RIVET_OK;
}

static unsigned long browser_source_height(
    const rivet_browser *browser
)
{
    size_t offset = 0u;
    unsigned long column = 0ul;
    unsigned long row = 0ul;
    unsigned long rows_used = 0ul;
    unsigned long columns;

    columns =
        browser->viewport_width /
        BROWSER_GLYPH_ADVANCE;
    if (columns == 0ul) {
        columns = 1ul;
    }

    while (offset <
           browser->document.source_bytes) {
        unsigned int codepoint;
        size_t used;
        rivet_result result =
            browser_utf8_one(
                browser->document.source,
                browser->document.source_bytes,
                offset,
                &codepoint,
                &used
            );

        if (result != RIVET_OK) {
            return ULONG_MAX;
        }

        if (codepoint == 0x0au ||
            codepoint == 0x0du) {
            column = 0ul;
            if (row == ULONG_MAX) {
                return ULONG_MAX;
            }
            ++row;
            offset += used;
            continue;
        }

        if (column == columns) {
            column = 0ul;
            if (row == ULONG_MAX) {
                return ULONG_MAX;
            }
            ++row;
        }

        if (row == ULONG_MAX) {
            return ULONG_MAX;
        }
        if (row + 1ul > rows_used) {
            rows_used = row + 1ul;
        }

        ++column;
        offset += used;
    }

    if (rows_used >
        ULONG_MAX / BROWSER_LINE_HEIGHT) {
        return ULONG_MAX;
    }
    return rows_used * BROWSER_LINE_HEIGHT;
}

static unsigned long browser_scroll_limit(
    const rivet_browser *browser
)
{
    if (browser->source_mode) {
        return browser_source_height(browser);
    }
    return browser->document_height;
}

rivet_result rivet_browser_scroll_up(
    rivet_browser *browser
)
{
    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!browser->loaded) {
        return RIVET_ERR_NOT_FOUND;
    }

    if (browser->scroll_y <=
        BROWSER_SCROLL_STEP) {
        browser->scroll_y = 0ul;
    } else {
        browser->scroll_y -=
            BROWSER_SCROLL_STEP;
    }
    return RIVET_OK;
}

rivet_result rivet_browser_scroll_down(
    rivet_browser *browser
)
{
    unsigned long limit;

    if (browser == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (!browser->loaded) {
        return RIVET_ERR_NOT_FOUND;
    }

    limit = browser_scroll_limit(browser);
    if (browser->scroll_y >= limit) {
        return RIVET_ERR_NOT_FOUND;
    }
    if (limit - browser->scroll_y <
        BROWSER_SCROLL_STEP) {
        browser->scroll_y = limit;
    } else {
        browser->scroll_y +=
            BROWSER_SCROLL_STEP;
    }
    return RIVET_OK;
}

static rivet_result command_back(void *context)
{
    return rivet_browser_back(
        (rivet_browser *)context);
}

static rivet_result command_forward(void *context)
{
    return rivet_browser_forward(
        (rivet_browser *)context);
}

static rivet_result command_reload(void *context)
{
    return rivet_browser_reload(
        (rivet_browser *)context);
}

static rivet_result command_home(void *context)
{
    return rivet_browser_home(
        (rivet_browser *)context);
}

static rivet_result command_link_next(void *context)
{
    return rivet_browser_next_link(
        (rivet_browser *)context);
}

static rivet_result command_link_open(void *context)
{
    return rivet_browser_open_selected_link(
        (rivet_browser *)context);
}

static rivet_result command_bookmark_add(void *context)
{
    return rivet_browser_bookmark_current(
        (rivet_browser *)context);
}

static rivet_result command_download(void *context)
{
    return rivet_browser_download_selected(
        (rivet_browser *)context);
}

static rivet_result command_source(void *context)
{
    return rivet_browser_toggle_source(
        (rivet_browser *)context);
}

static rivet_result command_scroll_up(void *context)
{
    return rivet_browser_scroll_up(
        (rivet_browser *)context);
}

static rivet_result command_scroll_down(void *context)
{
    return rivet_browser_scroll_down(
        (rivet_browser *)context);
}

rivet_result rivet_browser_register_commands(
    rivet_browser *browser,
    rivet_command_registry *commands
)
{
    static const struct command_definition {
        const char *id;
        rivet_command_fn fn;
    } definitions[] = {
        {RIVET_BROWSER_CMD_BACK, command_back},
        {RIVET_BROWSER_CMD_FORWARD, command_forward},
        {RIVET_BROWSER_CMD_RELOAD, command_reload},
        {RIVET_BROWSER_CMD_HOME, command_home},
        {RIVET_BROWSER_CMD_LINK_NEXT, command_link_next},
        {RIVET_BROWSER_CMD_LINK_OPEN, command_link_open},
        {RIVET_BROWSER_CMD_BOOKMARK_ADD, command_bookmark_add},
        {RIVET_BROWSER_CMD_DOWNLOAD, command_download},
        {RIVET_BROWSER_CMD_VIEW_SOURCE, command_source},
        {RIVET_BROWSER_CMD_SCROLL_UP, command_scroll_up},
        {RIVET_BROWSER_CMD_SCROLL_DOWN, command_scroll_down}
    };
    size_t i;

    if (browser == NULL || commands == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u;
         i < sizeof(definitions) /
             sizeof(definitions[0]);
         ++i) {
        rivet_result result =
            rivet_commands_add(
                commands,
                definitions[i].id,
                definitions[i].fn,
                browser
            );
        if (result != RIVET_OK) {
            return result;
        }
    }
    return RIVET_OK;
}

const rivet_keymap *rivet_browser_default_keymap(void)
{
    static const rivet_key_binding bindings[] = {
        {0x42u, RIVET_MOD_ALT, RIVET_BROWSER_CMD_BACK},
        {0x46u, RIVET_MOD_ALT, RIVET_BROWSER_CMD_FORWARD},
        {0x52u, RIVET_MOD_CTRL, RIVET_BROWSER_CMD_RELOAD},
        {0x48u, RIVET_MOD_ALT, RIVET_BROWSER_CMD_HOME},
        {0x4eu, 0u, RIVET_BROWSER_CMD_LINK_NEXT},
        {RIVET_KEY_ENTER, 0u, RIVET_BROWSER_CMD_LINK_OPEN},
        {0x4bu, RIVET_MOD_CTRL, RIVET_BROWSER_CMD_BOOKMARK_ADD},
        {0x44u, RIVET_MOD_CTRL, RIVET_BROWSER_CMD_DOWNLOAD},
        {0x53u, RIVET_MOD_CTRL, RIVET_BROWSER_CMD_VIEW_SOURCE},
        {RIVET_KEY_UP, 0u, RIVET_BROWSER_CMD_SCROLL_UP},
        {RIVET_KEY_DOWN, 0u, RIVET_BROWSER_CMD_SCROLL_DOWN}
    };
    static const rivet_keymap keymap = {
        bindings,
        sizeof(bindings) / sizeof(bindings[0])
    };

    return &keymap;
}

static rivet_result browser_utf8_one(
    const unsigned char *bytes,
    size_t byte_count,
    size_t offset,
    unsigned int *codepoint,
    size_t *used
)
{
    unsigned char first;
    unsigned int value;
    size_t need;
    size_t i;

    if (bytes == NULL ||
        codepoint == NULL ||
        used == NULL ||
        offset >= byte_count) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    first = bytes[offset];
    if (first <= 0x7fu) {
        *codepoint = (unsigned int)first;
        *used = 1u;
        return RIVET_OK;
    }

    if (first >= 0xc2u && first <= 0xdfu) {
        value = (unsigned int)(first & 0x1fu);
        need = 2u;
    } else if (first >= 0xe0u && first <= 0xefu) {
        value = (unsigned int)(first & 0x0fu);
        need = 3u;
    } else if (first >= 0xf0u && first <= 0xf4u) {
        value = (unsigned int)(first & 0x07u);
        need = 4u;
    } else {
        return RIVET_ERR_UNSUPPORTED;
    }

    if (need > byte_count - offset) {
        return RIVET_ERR_UNSUPPORTED;
    }

    for (i = 1u; i < need; ++i) {
        unsigned char continuation =
            bytes[offset + i];
        if ((continuation & 0xc0u) != 0x80u) {
            return RIVET_ERR_UNSUPPORTED;
        }
        value = (value << 6u) |
                (unsigned int)
                    (continuation & 0x3fu);
    }

    *codepoint = value;
    *used = need;
    return RIVET_OK;
}

static int browser_glyph_rows(
    unsigned int codepoint,
    unsigned char rows[7]
)
{
    static const unsigned char letters[26][7] = {
        {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11},
        {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
        {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e},
        {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
        {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f},
        {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
        {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f},
        {0x11,0x11,0x11,0x1f,0x11,0x11,0x11},
        {0x1f,0x04,0x04,0x04,0x04,0x04,0x1f},
        {0x07,0x02,0x02,0x02,0x12,0x12,0x0c},
        {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
        {0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
        {0x11,0x1b,0x15,0x15,0x11,0x11,0x11},
        {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
        {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e},
        {0x1e,0x11,0x11,0x1e,0x10,0x10,0x10},
        {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d},
        {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11},
        {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e},
        {0x1f,0x04,0x04,0x04,0x04,0x04,0x04},
        {0x11,0x11,0x11,0x11,0x11,0x11,0x0e},
        {0x11,0x11,0x11,0x11,0x11,0x0a,0x04},
        {0x11,0x11,0x11,0x15,0x15,0x1b,0x11},
        {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11},
        {0x11,0x11,0x0a,0x04,0x04,0x04,0x04},
        {0x1f,0x01,0x02,0x04,0x08,0x10,0x1f}
    };
    static const unsigned char digits[10][7] = {
        {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e},
        {0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
        {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f},
        {0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e},
        {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02},
        {0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e},
        {0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e},
        {0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
        {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e},
        {0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e}
    };
    size_t i;

    if (rows == NULL) {
        return 0;
    }
    for (i = 0u; i < 7u; ++i) {
        rows[i] = 0u;
    }

    if (codepoint >= 0x61u &&
        codepoint <= 0x7au) {
        codepoint -= 0x20u;
    }
    if (codepoint == 0x20u) {
        return 1;
    }
    if (codepoint >= 0x41u &&
        codepoint <= 0x5au) {
        for (i = 0u; i < 7u; ++i) {
            rows[i] = (unsigned char)
                (letters[codepoint - 0x41u][i] << 3);
        }
        return 1;
    }
    if (codepoint >= 0x30u &&
        codepoint <= 0x39u) {
        for (i = 0u; i < 7u; ++i) {
            rows[i] = (unsigned char)
                (digits[codepoint - 0x30u][i] << 3);
        }
        return 1;
    }

    switch (codepoint) {
    case 0x2eu:
        rows[6] = 0x04u << 3;
        break;
    case 0x2du:
        rows[3] = 0x0eu << 3;
        break;
    case 0x2fu:
        rows[0] = 0x01u << 3;
        rows[1] = 0x02u << 3;
        rows[2] = 0x04u << 3;
        rows[3] = 0x04u << 3;
        rows[4] = 0x08u << 3;
        rows[5] = 0x10u << 3;
        break;
    case 0x3au:
        rows[2] = 0x04u << 3;
        rows[5] = 0x04u << 3;
        break;
    case 0x3cu:
        rows[1] = 0x02u << 3;
        rows[2] = 0x04u << 3;
        rows[3] = 0x08u << 3;
        rows[4] = 0x04u << 3;
        rows[5] = 0x02u << 3;
        break;
    case 0x3eu:
        rows[1] = 0x08u << 3;
        rows[2] = 0x04u << 3;
        rows[3] = 0x02u << 3;
        rows[4] = 0x04u << 3;
        rows[5] = 0x08u << 3;
        break;
    case 0x3du:
        rows[2] = 0x0eu << 3;
        rows[4] = 0x0eu << 3;
        break;
    case 0x22u:
        rows[0] = 0x0au << 3;
        rows[1] = 0x0au << 3;
        break;
    case 0x27u:
        rows[0] = 0x04u << 3;
        rows[1] = 0x04u << 3;
        break;
    case 0x23u:
        rows[1] = 0x0au << 3;
        rows[2] = 0x1fu << 3;
        rows[3] = 0x0au << 3;
        rows[4] = 0x1fu << 3;
        rows[5] = 0x0au << 3;
        break;
    case 0x3fu:
        rows[0] = 0x0eu << 3;
        rows[1] = 0x11u << 3;
        rows[2] = 0x02u << 3;
        rows[3] = 0x04u << 3;
        rows[5] = 0x04u << 3;
        break;
    case 0x21u:
        rows[0] = 0x04u << 3;
        rows[1] = 0x04u << 3;
        rows[2] = 0x04u << 3;
        rows[3] = 0x04u << 3;
        rows[5] = 0x04u << 3;
        break;
    case 0x5fu:
        rows[6] = 0x1fu << 3;
        break;
    case 0x7bu:
        rows[0] = 0x06u << 3;
        rows[1] = 0x04u << 3;
        rows[2] = 0x08u << 3;
        rows[3] = 0x04u << 3;
        rows[4] = 0x08u << 3;
        rows[5] = 0x04u << 3;
        rows[6] = 0x06u << 3;
        break;
    case 0x7du:
        rows[0] = 0x0cu << 3;
        rows[1] = 0x04u << 3;
        rows[2] = 0x02u << 3;
        rows[3] = 0x04u << 3;
        rows[4] = 0x02u << 3;
        rows[5] = 0x04u << 3;
        rows[6] = 0x0cu << 3;
        break;
    case 0x3bu:
        rows[2] = 0x04u << 3;
        rows[5] = 0x04u << 3;
        rows[6] = 0x08u << 3;
        break;
    case 0x2cu:
        rows[5] = 0x04u << 3;
        rows[6] = 0x08u << 3;
        break;
    default:
        rows[0] = 0x0eu << 3;
        rows[1] = 0x11u << 3;
        rows[2] = 0x02u << 3;
        rows[3] = 0x04u << 3;
        rows[5] = 0x04u << 3;
        break;
    }

    return 1;
}

static rivet_result browser_draw_glyph(
    rivet_surface *surface,
    long x,
    long y,
    unsigned int codepoint,
    rivet_rgba8 color
)
{
    unsigned char rows[7];

    if (!browser_glyph_rows(
            codepoint,
            rows)) {
        return RIVET_ERR_UNSUPPORTED;
    }

    return rivet_surface_blit_mono1(
        surface,
        x,
        y,
        rows,
        sizeof(rows),
        1u,
        BROWSER_GLYPH_WIDTH,
        BROWSER_GLYPH_HEIGHT,
        color
    );
}

static int browser_box_selected_link(
    const rivet_browser *browser,
    const rivet_layout_box *box
)
{
    size_t node_index;
    size_t remaining;

    if (browser->selected_link_node ==
            RIVET_DOCUMENT_NO_PARENT ||
        browser->selected_link_node >=
            browser->document.node_count ||
        box->node_index >=
            browser->document.node_count) {
        return 0;
    }

    node_index = box->node_index;
    remaining = browser->document.node_count;
    while (remaining != 0u) {
        if (node_index ==
            browser->selected_link_node) {
            return 1;
        }
        node_index = browser->document.nodes[
            node_index].parent;
        if (node_index ==
            RIVET_DOCUMENT_NO_PARENT ||
            node_index >=
                browser->document.node_count) {
            return 0;
        }
        --remaining;
    }

    return 0;
}

static rivet_result browser_scrolled_y(
    long origin_y,
    unsigned long content_y,
    unsigned long scroll_y,
    long *translated_y
)
{
    unsigned long delta;

    if (translated_y == NULL ||
        origin_y < 0L) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (content_y >= scroll_y) {
        delta = content_y - scroll_y;
        if (delta >
            (unsigned long)LONG_MAX -
            (unsigned long)origin_y) {
            return RIVET_ERR_CAPACITY;
        }
        *translated_y =
            origin_y + (long)delta;
    } else {
        unsigned long magnitude;

        delta = scroll_y - content_y;
        if (delta <= (unsigned long)origin_y) {
            *translated_y =
                origin_y - (long)delta;
            return RIVET_OK;
        }

        magnitude =
            delta - (unsigned long)origin_y;
        if (magnitude >
            (unsigned long)LONG_MAX + 1ul) {
            return RIVET_ERR_CAPACITY;
        }
        *translated_y =
            -((long)(magnitude - 1ul)) - 1L;
    }

    return RIVET_OK;
}

static rivet_result browser_render_text_box(
    rivet_surface *surface,
    const rivet_browser *browser,
    const rivet_layout_box *box,
    long origin_x,
    long origin_y,
    rivet_browser_style style
)
{
    size_t offset = box->source.offset;
    size_t end;
    unsigned long target_glyphs;
    unsigned long emitted = 0ul;
    int pending_space = 0;
    long x;
    long y;
    rivet_rgba8 foreground = {
        box->foreground.r,
        box->foreground.g,
        box->foreground.b,
        box->foreground.a
    };
    rivet_rgba8 background = {
        box->background.r,
        box->background.g,
        box->background.b,
        box->background.a
    };
    int has_background =
        box->has_background;
    rivet_rect rect;

    if (box->source.offset >
            browser->document.source_bytes ||
        box->source.length >
            browser->document.source_bytes -
            box->source.offset ||
        box->width % BROWSER_GLYPH_ADVANCE != 0ul ||
        box->x > (unsigned long)LONG_MAX ||
        box->x >
            (unsigned long)LONG_MAX -
            (unsigned long)origin_x) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    x = origin_x + (long)box->x;
    if (browser_scrolled_y(
            origin_y,
            box->y,
            browser->scroll_y,
            &y) != RIVET_OK) {
        return RIVET_ERR_CAPACITY;
    }
    target_glyphs =
        box->width / BROWSER_GLYPH_ADVANCE;
    end = box->source.offset +
          box->source.length;

    if (browser_box_selected_link(
            browser,
            box)) {
        background =
            style.link_focus_background;
        has_background = 1;
    }

    if (has_background) {
        rect.x = x;
        rect.y = y;
        rect.width = box->width;
        rect.height = box->height;
        if (rivet_surface_fill_rect(
                surface,
                rect,
                background) != RIVET_OK) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }
    }

    while (offset < end &&
           emitted < target_glyphs) {
        unsigned int codepoint;
        size_t used;
        rivet_result result =
            browser_utf8_one(
                browser->document.source,
                end,
                offset,
                &codepoint,
                &used
            );

        if (result != RIVET_OK) {
            return result;
        }

        if (browser_space(codepoint)) {
            pending_space = 1;
            offset += used;
            continue;
        }

        if (pending_space &&
            emitted < target_glyphs &&
            (emitted != 0ul ||
             box->x != 0ul)) {
            result = browser_draw_glyph(
                surface,
                x + (long)
                    (emitted *
                     BROWSER_GLYPH_ADVANCE),
                y,
                0x20u,
                foreground
            );
            if (result != RIVET_OK) {
                return result;
            }
            ++emitted;
        }
        pending_space = 0;

        if (emitted >= target_glyphs) {
            break;
        }
        result = browser_draw_glyph(
            surface,
            x + (long)
                (emitted *
                 BROWSER_GLYPH_ADVANCE),
            y,
            codepoint,
            foreground
        );
        if (result != RIVET_OK) {
            return result;
        }
        ++emitted;
        offset += used;
    }

    if (pending_space &&
        emitted < target_glyphs) {
        rivet_result result =
            browser_draw_glyph(
                surface,
                x + (long)
                    (emitted *
                     BROWSER_GLYPH_ADVANCE),
                y,
                0x20u,
                foreground
            );
        if (result != RIVET_OK) {
            return result;
        }
        ++emitted;
    }

    return emitted == target_glyphs ?
        RIVET_OK :
        RIVET_ERR_INVALID_ARGUMENT;
}

static rivet_result browser_render_document(
    rivet_surface *surface,
    const rivet_browser *browser,
    rivet_rect bounds,
    rivet_browser_style style
)
{
    size_t i;
    long origin_y;

    if (bounds.y >
        LONG_MAX -
        (long)RIVET_BROWSER_CHROME_HEIGHT) {
        return RIVET_ERR_CAPACITY;
    }
    origin_y = bounds.y +
               (long)RIVET_BROWSER_CHROME_HEIGHT;

    for (i = 0u;
         i < browser->box_count;
         ++i) {
        const rivet_layout_box *box =
            &browser->storage.boxes[i];
        rivet_rect rect;
        long translated_y;
        rivet_result translated_result;

        translated_result = browser_scrolled_y(
            origin_y,
            box->y,
            browser->scroll_y,
            &translated_y
        );
        if (translated_result == RIVET_ERR_CAPACITY) {
            continue;
        }
        if (translated_result != RIVET_OK) {
            return translated_result;
        }

        if (box->x >
            (unsigned long)LONG_MAX) {
            return RIVET_ERR_CAPACITY;
        }

        if (box->kind == RIVET_LAYOUT_TEXT) {
            rivet_result result =
                browser_render_text_box(
                    surface,
                    browser,
                    box,
                    bounds.x,
                    origin_y,
                    style
                );
            if (result != RIVET_OK) {
                return result;
            }
            continue;
        }

        if (box->x >
            (unsigned long)LONG_MAX -
            (unsigned long)bounds.x) {
            return RIVET_ERR_CAPACITY;
        }
        rect.x = bounds.x +
                 (long)box->x;
        rect.y = translated_y;
        rect.width = box->width;
        rect.height = box->height;

        if (browser_box_selected_link(
                browser,
                box)) {
            if (rivet_surface_fill_rect(
                    surface,
                    rect,
                    style.link_focus_background) !=
                RIVET_OK) {
                return RIVET_ERR_INVALID_ARGUMENT;
            }
        } else if (box->kind ==
                   RIVET_LAYOUT_IMAGE) {
            rivet_rgba8 fill =
                style.image_placeholder;
            if (box->has_background) {
                fill.r = box->background.r;
                fill.g = box->background.g;
                fill.b = box->background.b;
                fill.a = box->background.a;
            }
            if (rivet_surface_fill_rect(
                    surface,
                    rect,
                    fill) != RIVET_OK) {
                return RIVET_ERR_INVALID_ARGUMENT;
            }
        } else if (box->kind ==
                   RIVET_LAYOUT_INPUT) {
            rivet_rgba8 fill =
                style.input_background;
            if (box->has_background) {
                fill.r = box->background.r;
                fill.g = box->background.g;
                fill.b = box->background.b;
                fill.a = box->background.a;
            }
            if (rivet_surface_fill_rect(
                    surface,
                    rect,
                    fill) != RIVET_OK) {
                return RIVET_ERR_INVALID_ARGUMENT;
            }
        }
    }

    return RIVET_OK;
}

static rivet_result browser_render_source(
    rivet_surface *surface,
    const rivet_browser *browser,
    rivet_rect bounds,
    rivet_browser_style style
)
{
    size_t offset = 0u;
    unsigned long column = 0ul;
    unsigned long row = 0ul;
    unsigned long columns;
    long origin_y;

    columns =
        bounds.width /
        BROWSER_GLYPH_ADVANCE;
    if (columns == 0ul) {
        columns = 1ul;
    }

    if (bounds.y >
        LONG_MAX -
        (long)RIVET_BROWSER_CHROME_HEIGHT) {
        return RIVET_ERR_CAPACITY;
    }
    origin_y = bounds.y +
               (long)RIVET_BROWSER_CHROME_HEIGHT;

    while (offset <
           browser->document.source_bytes) {
        unsigned int codepoint;
        size_t used;
        rivet_result result =
            browser_utf8_one(
                browser->document.source,
                browser->document.source_bytes,
                offset,
                &codepoint,
                &used
            );

        if (result != RIVET_OK) {
            return result;
        }

        if (codepoint == 0x0au ||
            codepoint == 0x0du) {
            column = 0ul;
            ++row;
            offset += used;
            continue;
        }
        if (codepoint == 0x09u ||
            codepoint == 0x0cu) {
            codepoint = 0x20u;
        }

        if (column == columns) {
            column = 0ul;
            ++row;
        }

        if (row <=
            (unsigned long)LONG_MAX /
            BROWSER_LINE_HEIGHT) {
            long y;
            unsigned long content_y =
                row * BROWSER_LINE_HEIGHT;

            result = browser_scrolled_y(
                origin_y,
                content_y,
                browser->scroll_y,
                &y
            );
            if (result == RIVET_OK) {
                result = browser_draw_glyph(
                    surface,
                    bounds.x +
                        (long)(column *
                               BROWSER_GLYPH_ADVANCE),
                    y,
                    codepoint,
                    style.source_foreground
                );
                if (result != RIVET_OK) {
                    return result;
                }
            } else if (result != RIVET_ERR_CAPACITY) {
                return result;
            }
        }

        ++column;
        offset += used;
    }

    return RIVET_OK;
}

static rivet_result browser_draw_word(
    rivet_surface *surface,
    long x,
    long y,
    const char *word,
    rivet_rgba8 color
)
{
    size_t i = 0u;

    while (word[i] != '\0') {
        rivet_result result =
            browser_draw_glyph(
                surface,
                x,
                y,
                (unsigned int)
                    (unsigned char)word[i],
                color
            );
        if (result != RIVET_OK) {
            return result;
        }
        ++i;
        x += (long)BROWSER_GLYPH_ADVANCE;
    }
    return RIVET_OK;
}

static rivet_result browser_surface_clip_to(
    rivet_surface *target,
    const rivet_surface *source,
    rivet_rect requested
)
{
    rivet_rect current;
    rivet_rect clip;
    unsigned long left;
    unsigned long top;
    unsigned long right;
    unsigned long bottom;
    unsigned long requested_right;
    unsigned long requested_bottom;
    unsigned long current_right;
    unsigned long current_bottom;

    if (target == NULL || source == NULL ||
        requested.x < 0L ||
        requested.y < 0L) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    current = source->clip;
    left = (unsigned long)requested.x >
           (unsigned long)current.x ?
        (unsigned long)requested.x :
        (unsigned long)current.x;
    top = (unsigned long)requested.y >
          (unsigned long)current.y ?
        (unsigned long)requested.y :
        (unsigned long)current.y;

    requested_right =
        (unsigned long)requested.x +
        requested.width;
    requested_bottom =
        (unsigned long)requested.y +
        requested.height;
    current_right =
        (unsigned long)current.x +
        current.width;
    current_bottom =
        (unsigned long)current.y +
        current.height;

    right = requested_right < current_right ?
        requested_right : current_right;
    bottom = requested_bottom < current_bottom ?
        requested_bottom : current_bottom;

    clip.x = (long)left;
    clip.y = (long)top;
    clip.width =
        right > left ? right - left : 0ul;
    clip.height =
        bottom > top ? bottom - top : 0ul;

    *target = *source;
    return rivet_surface_set_clip(
        target,
        clip
    );
}

rivet_result rivet_browser_render(
    rivet_surface *surface,
    const rivet_browser *browser,
    rivet_rect bounds,
    rivet_browser_style style
)
{
    rivet_rect chrome;
    rivet_rect content;
    rivet_surface chrome_surface;
    rivet_surface content_surface;
    rivet_result result;

    if (surface == NULL || browser == NULL ||
        rivet_surface_validate(surface) !=
            RIVET_OK ||
        !browser->loaded ||
        bounds.x < 0L ||
        bounds.y < 0L ||
        bounds.width !=
            browser->viewport_width ||
        bounds.height <
            RIVET_BROWSER_CHROME_HEIGHT ||
        (unsigned long)bounds.x >
            surface->width ||
        bounds.width >
            surface->width -
            (unsigned long)bounds.x ||
        (unsigned long)bounds.y >
            surface->height ||
        bounds.height >
            surface->height -
            (unsigned long)bounds.y) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    result = rivet_surface_fill_rect(
        surface,
        bounds,
        style.page_background
    );
    if (result != RIVET_OK) {
        return result;
    }

    chrome = bounds;
    chrome.height =
        RIVET_BROWSER_CHROME_HEIGHT;
    result = rivet_surface_fill_rect(
        surface,
        chrome,
        style.chrome_background
    );
    if (result != RIVET_OK) {
        return result;
    }

    result = browser_surface_clip_to(
        &chrome_surface,
        surface,
        chrome
    );
    if (result != RIVET_OK) {
        return result;
    }

    result = browser_draw_word(
        &chrome_surface,
        bounds.x + 2L,
        bounds.y + 1L,
        browser->source_mode ?
            "SOURCE" :
            "WEB1",
        style.chrome_foreground
    );
    if (result != RIVET_OK) {
        return result;
    }

    content = bounds;
    content.y +=
        (long)RIVET_BROWSER_CHROME_HEIGHT;
    content.height -=
        RIVET_BROWSER_CHROME_HEIGHT;

    result = browser_surface_clip_to(
        &content_surface,
        surface,
        content
    );
    if (result != RIVET_OK) {
        return result;
    }

    if (browser->source_mode) {
        return browser_render_source(
            &content_surface,
            browser,
            bounds,
            style
        );
    }

    return browser_render_document(
        &content_surface,
        browser,
        bounds,
        style
    );
}
