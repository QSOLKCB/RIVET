/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_BROWSER_H
#define RIVET_BROWSER_H

#include <stddef.h>

#include "rivet/document.h"
#include "rivet/gfx.h"
#include "rivet/rivet.h"
#include "rivet/ui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RIVET_BROWSER_ABI_VERSION 1u
#define RIVET_BROWSER_URL_MAX 512u
#define RIVET_BROWSER_CHROME_HEIGHT 10ul

#define RIVET_BROWSER_CMD_BACK "browser.back"
#define RIVET_BROWSER_CMD_FORWARD "browser.forward"
#define RIVET_BROWSER_CMD_RELOAD "browser.reload"
#define RIVET_BROWSER_CMD_HOME "browser.home"
#define RIVET_BROWSER_CMD_LINK_NEXT "browser.link.next"
#define RIVET_BROWSER_CMD_LINK_OPEN "browser.link.open"
#define RIVET_BROWSER_CMD_BOOKMARK_ADD "browser.bookmark.add"
#define RIVET_BROWSER_CMD_DOWNLOAD "browser.download"
#define RIVET_BROWSER_CMD_VIEW_SOURCE "view.source"
#define RIVET_BROWSER_CMD_SCROLL_UP "view.scroll.up"
#define RIVET_BROWSER_CMD_SCROLL_DOWN "view.scroll.down"

typedef struct rivet_browser_url {
    unsigned char bytes[RIVET_BROWSER_URL_MAX];
    size_t length;
} rivet_browser_url;

typedef rivet_result (*rivet_browser_fetch_fn)(
    void *context,
    const unsigned char *url,
    size_t url_length,
    unsigned char *buffer,
    size_t capacity,
    size_t *byte_count
);

typedef rivet_result (*rivet_browser_download_fn)(
    void *context,
    const unsigned char *url,
    size_t url_length,
    const unsigned char *bytes,
    size_t byte_count
);

typedef struct rivet_browser_io {
    void *context;
    rivet_browser_fetch_fn fetch;
    rivet_browser_download_fn download;
} rivet_browser_io;

typedef struct rivet_browser_config {
    const unsigned char *source;
    size_t source_bytes;
    rivet_doc_slice home_url;
    rivet_doc_slice user_css;
    int downloads_enabled;
} rivet_browser_config;

typedef struct rivet_browser_storage {
    unsigned char *document_bytes;
    size_t document_capacity;
    rivet_doc_node *nodes;
    size_t node_capacity;
    rivet_css_rule *rules;
    size_t rule_capacity;
    rivet_layout_box *boxes;
    size_t box_capacity;
    rivet_browser_url *history;
    size_t history_capacity;
    rivet_browser_url *bookmarks;
    size_t bookmark_capacity;
    unsigned char *scratch_bytes;
    size_t scratch_capacity;
} rivet_browser_storage;

typedef struct rivet_browser {
    rivet_browser_io io;
    rivet_browser_config config;
    rivet_browser_storage storage;
    rivet_document document;
    size_t document_bytes;
    size_t rule_count;
    size_t box_count;
    unsigned long document_height;
    unsigned long viewport_width;
    unsigned long scroll_y;
    rivet_browser_url current_url;
    size_t history_count;
    size_t history_index;
    size_t bookmark_count;
    size_t selected_link_node;
    int loaded;
    int source_mode;
} rivet_browser;

typedef struct rivet_browser_style {
    rivet_rgba8 chrome_background;
    rivet_rgba8 chrome_foreground;
    rivet_rgba8 page_background;
    rivet_rgba8 source_foreground;
    rivet_rgba8 link_focus_background;
    rivet_rgba8 image_placeholder;
    rivet_rgba8 input_background;
} rivet_browser_style;

rivet_result rivet_browser_config_parse(
    rivet_browser_config *config,
    const unsigned char *bytes,
    size_t byte_count
);

rivet_result rivet_browser_init(
    rivet_browser *browser,
    const rivet_browser_io *io,
    const rivet_browser_config *config,
    const rivet_browser_storage *storage,
    unsigned long viewport_width
);

rivet_result rivet_browser_open(
    rivet_browser *browser,
    const unsigned char *url,
    size_t url_length
);

rivet_result rivet_browser_home(rivet_browser *browser);
rivet_result rivet_browser_back(rivet_browser *browser);
rivet_result rivet_browser_forward(rivet_browser *browser);
rivet_result rivet_browser_reload(rivet_browser *browser);
rivet_result rivet_browser_next_link(rivet_browser *browser);
rivet_result rivet_browser_open_selected_link(rivet_browser *browser);
rivet_result rivet_browser_bookmark_current(rivet_browser *browser);
rivet_result rivet_browser_download_selected(rivet_browser *browser);
rivet_result rivet_browser_toggle_source(rivet_browser *browser);
rivet_result rivet_browser_scroll_up(rivet_browser *browser);
rivet_result rivet_browser_scroll_down(rivet_browser *browser);

rivet_result rivet_browser_register_commands(
    rivet_browser *browser,
    rivet_command_registry *commands
);

const rivet_keymap *rivet_browser_default_keymap(void);

rivet_result rivet_browser_render(
    rivet_surface *surface,
    const rivet_browser *browser,
    rivet_rect bounds,
    rivet_browser_style style
);

#ifdef __cplusplus
}
#endif

#endif
