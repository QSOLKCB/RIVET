/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_UI_H
#define RIVET_UI_H

#include <stddef.h>

#include "rivet/gfx.h"
#include "rivet/rivet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RIVET_UI_ABI_VERSION 1u

enum {
    RIVET_KEY_UP = 0x100u,
    RIVET_KEY_DOWN = 0x101u,
    RIVET_KEY_ENTER = 0x102u,
    RIVET_KEY_ESCAPE = 0x103u
};

enum {
    RIVET_MOD_SHIFT = 1u << 0,
    RIVET_MOD_CTRL = 1u << 1,
    RIVET_MOD_ALT = 1u << 2,
    RIVET_MOD_META = 1u << 3,
    RIVET_MOD_ALL = RIVET_MOD_SHIFT |
                    RIVET_MOD_CTRL |
                    RIVET_MOD_ALT |
                    RIVET_MOD_META
};

typedef struct rivet_key_event {
    unsigned int key;
    unsigned int modifiers;
    int pressed;
} rivet_key_event;

typedef struct rivet_key_binding {
    unsigned int key;
    unsigned int modifiers;
    const char *command_id;
} rivet_key_binding;

typedef struct rivet_keymap {
    const rivet_key_binding *bindings;
    size_t count;
} rivet_keymap;

rivet_result rivet_keymap_validate(const rivet_keymap *keymap);

rivet_result rivet_keymap_dispatch(
    const rivet_keymap *keymap,
    const rivet_command_registry *commands,
    rivet_key_event event
);

typedef struct rivet_menu_item {
    const char *label;
    const char *command_id;
} rivet_menu_item;

typedef struct rivet_menu {
    const rivet_menu_item *items;
    size_t count;
    size_t selected;
    int visible;
} rivet_menu;

typedef struct rivet_menu_style {
    rivet_rgba8 background;
    rivet_rgba8 foreground;
    rivet_rgba8 selected_background;
    rivet_rgba8 selected_foreground;
} rivet_menu_style;

rivet_result rivet_menu_init(
    rivet_menu *menu,
    const rivet_menu_item *items,
    size_t count
);

rivet_result rivet_menu_validate(const rivet_menu *menu);

rivet_result rivet_menu_set_visible(
    rivet_menu *menu,
    int visible
);

rivet_result rivet_menu_handle_key(
    rivet_menu *menu,
    const rivet_command_registry *commands,
    rivet_key_event event
);

rivet_result rivet_menu_render(
    rivet_surface *surface,
    const rivet_menu *menu,
    rivet_rect bounds,
    rivet_menu_style style
);

#ifdef __cplusplus
}
#endif

#endif
