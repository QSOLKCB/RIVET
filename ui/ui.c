/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/ui.h"

#include <limits.h>

#define RIVET_UI_GLYPH_WIDTH 5ul
#define RIVET_UI_GLYPH_HEIGHT 7ul
#define RIVET_UI_GLYPH_PITCH 6ul
#define RIVET_UI_MENU_PAD_X 2ul
#define RIVET_UI_MENU_PAD_Y 2ul
#define RIVET_UI_MENU_ROW_HEIGHT 11ul

static int rivet_ui_cstr_valid(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static int rivet_ui_key_valid(unsigned int key)
{
    return (key >= 0x20u && key <= 0x7eu) ||
           key == RIVET_KEY_UP ||
           key == RIVET_KEY_DOWN ||
           key == RIVET_KEY_ENTER ||
           key == RIVET_KEY_ESCAPE;
}

static int rivet_ui_event_valid(rivet_key_event event)
{
    return rivet_ui_key_valid(event.key) &&
           (event.modifiers & ~RIVET_MOD_ALL) == 0u &&
           (event.pressed == 0 || event.pressed == 1);
}

static int rivet_ui_glyph_rows(
    unsigned char code,
    unsigned char rows[7]
)
{
    static const unsigned char glyphs[26][7] = {
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
    size_t i;

    if (rows == NULL) {
        return 0;
    }

    if (code == 0x20u) {
        for (i = 0u; i < 7u; ++i) {
            rows[i] = 0u;
        }
        return 1;
    }

    if (code < 0x41u || code > 0x5au) {
        return 0;
    }

    for (i = 0u; i < 7u; ++i) {
        rows[i] = (unsigned char)(
            glyphs[(unsigned int)(code - 0x41u)][i] << 3
        );
    }
    return 1;
}

static rivet_result rivet_ui_label_measure(
    const char *label,
    unsigned long *pixel_width
)
{
    size_t count = 0u;
    unsigned char rows[7];

    if (!rivet_ui_cstr_valid(label) || pixel_width == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    while (label[count] != '\0') {
        if (!rivet_ui_glyph_rows((unsigned char)label[count], rows)) {
            return RIVET_ERR_UNSUPPORTED;
        }
        if (count == (size_t)-1) {
            return RIVET_ERR_CAPACITY;
        }
        ++count;
    }

    if (count > (size_t)ULONG_MAX ||
        (unsigned long)count > ULONG_MAX / RIVET_UI_GLYPH_PITCH) {
        return RIVET_ERR_CAPACITY;
    }

    *pixel_width =
        (unsigned long)count * RIVET_UI_GLYPH_PITCH - 1ul;
    return RIVET_OK;
}

static rivet_result rivet_ui_draw_label(
    rivet_surface *surface,
    long x,
    long y,
    const char *label,
    rivet_rgba8 color
)
{
    size_t index = 0u;
    unsigned char rows[7];

    while (label[index] != '\0') {
        if (!rivet_ui_glyph_rows((unsigned char)label[index], rows)) {
            return RIVET_ERR_UNSUPPORTED;
        }

        if (rivet_surface_blit_mono1(
                surface,
                x,
                y,
                rows,
                sizeof(rows),
                1u,
                RIVET_UI_GLYPH_WIDTH,
                RIVET_UI_GLYPH_HEIGHT,
                color) != RIVET_OK) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }

        ++index;
        if (label[index] != '\0') {
            if (x > LONG_MAX - (long)RIVET_UI_GLYPH_PITCH) {
                return RIVET_ERR_CAPACITY;
            }
            x += (long)RIVET_UI_GLYPH_PITCH;
        }
    }

    return RIVET_OK;
}

rivet_result rivet_keymap_validate(const rivet_keymap *keymap)
{
    size_t i;
    size_t j;

    if (keymap == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (keymap->count != 0u && keymap->bindings == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < keymap->count; ++i) {
        const rivet_key_binding *binding = &keymap->bindings[i];

        if (!rivet_ui_key_valid(binding->key) ||
            (binding->modifiers & ~RIVET_MOD_ALL) != 0u ||
            !rivet_ui_cstr_valid(binding->command_id)) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }

        for (j = i + 1u; j < keymap->count; ++j) {
            if (binding->key == keymap->bindings[j].key &&
                binding->modifiers == keymap->bindings[j].modifiers) {
                return RIVET_ERR_DUPLICATE;
            }
        }
    }

    return RIVET_OK;
}

rivet_result rivet_keymap_dispatch(
    const rivet_keymap *keymap,
    const rivet_command_registry *commands,
    rivet_key_event event
)
{
    size_t i;
    rivet_result result;

    result = rivet_keymap_validate(keymap);
    if (result != RIVET_OK) {
        return result;
    }
    if (commands == NULL || !rivet_ui_event_valid(event)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (!event.pressed) {
        return RIVET_ERR_NOT_FOUND;
    }

    for (i = 0u; i < keymap->count; ++i) {
        const rivet_key_binding *binding = &keymap->bindings[i];

        if (binding->key == event.key &&
            binding->modifiers == event.modifiers) {
            return rivet_commands_dispatch(
                commands,
                binding->command_id
            );
        }
    }

    return RIVET_ERR_NOT_FOUND;
}

rivet_result rivet_menu_validate(const rivet_menu *menu)
{
    size_t i;

    if (menu == NULL ||
        menu->items == NULL ||
        menu->count == 0u ||
        menu->selected >= menu->count ||
        (menu->visible != 0 && menu->visible != 1)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < menu->count; ++i) {
        unsigned long width;
        rivet_result result;

        if (!rivet_ui_cstr_valid(menu->items[i].command_id)) {
            return RIVET_ERR_INVALID_ARGUMENT;
        }

        result = rivet_ui_label_measure(
            menu->items[i].label,
            &width
        );
        if (result != RIVET_OK) {
            return result;
        }
    }

    return RIVET_OK;
}

rivet_result rivet_menu_init(
    rivet_menu *menu,
    const rivet_menu_item *items,
    size_t count
)
{
    rivet_menu candidate;
    rivet_result result;

    if (menu == NULL || items == NULL || count == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    candidate.items = items;
    candidate.count = count;
    candidate.selected = 0u;
    candidate.visible = 1;

    result = rivet_menu_validate(&candidate);
    if (result != RIVET_OK) {
        return result;
    }

    *menu = candidate;
    return RIVET_OK;
}

rivet_result rivet_menu_set_visible(
    rivet_menu *menu,
    int visible
)
{
    rivet_result result = rivet_menu_validate(menu);

    if (result != RIVET_OK) {
        return result;
    }
    if (visible != 0 && visible != 1) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    menu->visible = visible;
    return RIVET_OK;
}

rivet_result rivet_menu_handle_key(
    rivet_menu *menu,
    const rivet_command_registry *commands,
    rivet_key_event event
)
{
    rivet_result result;

    result = rivet_menu_validate(menu);
    if (result != RIVET_OK) {
        return result;
    }
    if (commands == NULL || !rivet_ui_event_valid(event)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    if (!menu->visible || !event.pressed || event.modifiers != 0u) {
        return RIVET_ERR_NOT_FOUND;
    }

    if (event.key == RIVET_KEY_UP) {
        menu->selected =
            menu->selected == 0u ?
            menu->count - 1u :
            menu->selected - 1u;
        return RIVET_OK;
    }

    if (event.key == RIVET_KEY_DOWN) {
        menu->selected =
            menu->selected + 1u == menu->count ?
            0u :
            menu->selected + 1u;
        return RIVET_OK;
    }

    if (event.key == RIVET_KEY_ESCAPE) {
        menu->visible = 0;
        return RIVET_OK;
    }

    if (event.key == RIVET_KEY_ENTER) {
        return rivet_commands_dispatch(
            commands,
            menu->items[menu->selected].command_id
        );
    }

    return RIVET_ERR_NOT_FOUND;
}

rivet_result rivet_menu_render(
    rivet_surface *surface,
    const rivet_menu *menu,
    rivet_rect bounds,
    rivet_menu_style style
)
{
    size_t i;
    unsigned long required_height;
    rivet_result result;

    if (rivet_surface_validate(surface) != RIVET_OK) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    result = rivet_menu_validate(menu);
    if (result != RIVET_OK) {
        return result;
    }

    if (!menu->visible) {
        return RIVET_OK;
    }

    if (menu->count > (size_t)ULONG_MAX ||
        (unsigned long)menu->count >
            ULONG_MAX / RIVET_UI_MENU_ROW_HEIGHT) {
        return RIVET_ERR_CAPACITY;
    }

    required_height =
        (unsigned long)menu->count *
        RIVET_UI_MENU_ROW_HEIGHT;

    if (bounds.width < 2ul * RIVET_UI_MENU_PAD_X + RIVET_UI_GLYPH_WIDTH ||
        bounds.height < required_height) {
        return RIVET_ERR_CAPACITY;
    }

    for (i = 0u; i < menu->count; ++i) {
        unsigned long label_width;
        unsigned long y_offset;
        rivet_rect row;
        rivet_rgba8 background;
        rivet_rgba8 foreground;

        result = rivet_ui_label_measure(
            menu->items[i].label,
            &label_width
        );
        if (result != RIVET_OK) {
            return result;
        }
        if (label_width >
            bounds.width - 2ul * RIVET_UI_MENU_PAD_X) {
            return RIVET_ERR_CAPACITY;
        }

        y_offset =
            (unsigned long)i *
            RIVET_UI_MENU_ROW_HEIGHT;

        if (y_offset > (unsigned long)LONG_MAX ||
            bounds.y > LONG_MAX - (long)y_offset) {
            return RIVET_ERR_CAPACITY;
        }

        row.x = bounds.x;
        row.y = bounds.y + (long)y_offset;
        row.width = bounds.width;
        row.height = RIVET_UI_MENU_ROW_HEIGHT;

        if (i == menu->selected) {
            background = style.selected_background;
            foreground = style.selected_foreground;
        } else {
            background = style.background;
            foreground = style.foreground;
        }

        result = rivet_surface_fill_rect(
            surface,
            row,
            background
        );
        if (result != RIVET_OK) {
            return result;
        }

        if (row.x > LONG_MAX - (long)RIVET_UI_MENU_PAD_X ||
            row.y > LONG_MAX - (long)RIVET_UI_MENU_PAD_Y) {
            return RIVET_ERR_CAPACITY;
        }

        result = rivet_ui_draw_label(
            surface,
            row.x + (long)RIVET_UI_MENU_PAD_X,
            row.y + (long)RIVET_UI_MENU_PAD_Y,
            menu->items[i].label,
            foreground
        );
        if (result != RIVET_OK) {
            return result;
        }
    }

    return RIVET_OK;
}
