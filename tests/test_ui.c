/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/ui.h"

#include <stdio.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static rivet_result increment(void *context)
{
    int *value = (int *)context;
    ++(*value);
    return RIVET_OK;
}

static int test_keymap(void)
{
    rivet_command_slot slots[2];
    rivet_command_registry commands;
    rivet_key_binding bindings[2] = {
        {'O', RIVET_MOD_CTRL, "demo.open"},
        {'S', RIVET_MOD_CTRL, "demo.save"}
    };
    rivet_keymap keymap = {bindings, 2u};
    rivet_key_event event = {'O', RIVET_MOD_CTRL, 1};
    int opened = 0;
    int saved = 0;

    CHECK(rivet_commands_init(&commands, slots, 2u) == RIVET_OK);
    CHECK(rivet_commands_add(
        &commands, "demo.open", increment, &opened) == RIVET_OK);
    CHECK(rivet_commands_add(
        &commands, "demo.save", increment, &saved) == RIVET_OK);

    CHECK(rivet_keymap_validate(&keymap) == RIVET_OK);
    CHECK(rivet_keymap_dispatch(&keymap, &commands, event) == RIVET_OK);
    CHECK(opened == 1 && saved == 0);

    event.pressed = 0;
    CHECK(rivet_keymap_dispatch(
        &keymap, &commands, event) == RIVET_ERR_NOT_FOUND);
    CHECK(opened == 1);

    event.pressed = 1;
    event.key = 'Q';
    CHECK(rivet_keymap_dispatch(
        &keymap, &commands, event) == RIVET_ERR_NOT_FOUND);

    bindings[1].key = 'O';
    bindings[1].modifiers = RIVET_MOD_CTRL;
    CHECK(rivet_keymap_validate(&keymap) == RIVET_ERR_DUPLICATE);

    bindings[1].key = 'S';
    event.key = 'O';
    event.modifiers = RIVET_MOD_ALL << 1;
    CHECK(rivet_keymap_dispatch(
        &keymap, &commands, event) == RIVET_ERR_INVALID_ARGUMENT);
    return 0;
}

static int test_menu_projection_independence(void)
{
    rivet_command_slot slots[3];
    rivet_command_registry commands;
    const rivet_menu_item items[3] = {
        {"OPEN", "demo.open"},
        {"SAVE", "demo.save"},
        {"QUIT", "demo.quit"}
    };
    rivet_key_binding bindings[1] = {
        {'O', RIVET_MOD_CTRL, "demo.open"}
    };
    rivet_keymap keymap = {bindings, 1u};
    rivet_menu menu;
    rivet_key_event down = {RIVET_KEY_DOWN, 0u, 1};
    rivet_key_event enter = {RIVET_KEY_ENTER, 0u, 1};
    rivet_key_event escape = {RIVET_KEY_ESCAPE, 0u, 1};
    rivet_key_event ctrl_o = {'O', RIVET_MOD_CTRL, 1};
    int opened = 0;
    int saved = 0;
    int quit = 0;

    CHECK(rivet_commands_init(&commands, slots, 3u) == RIVET_OK);
    CHECK(rivet_commands_add(
        &commands, "demo.open", increment, &opened) == RIVET_OK);
    CHECK(rivet_commands_add(
        &commands, "demo.save", increment, &saved) == RIVET_OK);
    CHECK(rivet_commands_add(
        &commands, "demo.quit", increment, &quit) == RIVET_OK);

    CHECK(rivet_menu_init(&menu, items, 3u) == RIVET_OK);
    CHECK(menu.selected == 0u && menu.visible == 1);

    CHECK(rivet_menu_handle_key(
        &menu, &commands, down) == RIVET_OK);
    CHECK(menu.selected == 1u);

    CHECK(rivet_menu_handle_key(
        &menu, &commands, enter) == RIVET_OK);
    CHECK(saved == 1 && opened == 0 && quit == 0);

    CHECK(rivet_menu_handle_key(
        &menu, &commands, escape) == RIVET_OK);
    CHECK(menu.visible == 0);

    CHECK(rivet_menu_handle_key(
        &menu, &commands, down) == RIVET_ERR_NOT_FOUND);
    CHECK(menu.selected == 1u);

    CHECK(rivet_keymap_dispatch(
        &keymap, &commands, ctrl_o) == RIVET_OK);
    CHECK(opened == 1 && saved == 1 && quit == 0);
    return 0;
}

static int test_menu_validation_and_render(void)
{
    unsigned char pixels[80u * 40u * 4u];
    rivet_surface surface;
    rivet_menu menu;
    const rivet_menu_item items[2] = {
        {"OPEN", "demo.open"},
        {"SAVE", "demo.save"}
    };
    const rivet_menu_item unsupported[1] = {
        {"Open", "demo.open"}
    };
    rivet_rect bounds = {4L, 3L, 50ul, 22ul};
    rivet_menu_style style = {
        {1u,2u,3u,255u},
        {4u,5u,6u,255u},
        {7u,8u,9u,255u},
        {10u,11u,12u,255u}
    };

    CHECK(rivet_surface_attach(
        &surface,
        pixels,
        sizeof(pixels),
        80ul,
        40ul,
        320u) == RIVET_OK);
    CHECK(rivet_menu_init(&menu, items, 2u) == RIVET_OK);
    CHECK(rivet_menu_render(
        &surface, &menu, bounds, style) == RIVET_OK);

    CHECK(pixels[((3u * 80u + 4u) * 4u) + 0u] == 7u);
    CHECK(pixels[((14u * 80u + 4u) * 4u) + 0u] == 1u);

    bounds.width = 8ul;
    CHECK(rivet_menu_render(
        &surface, &menu, bounds, style) == RIVET_ERR_CAPACITY);

    CHECK(rivet_menu_init(
        &menu, unsupported, 1u) == RIVET_ERR_UNSUPPORTED);
    return 0;
}

int main(void)
{
    CHECK(RIVET_UI_ABI_VERSION == 1u);
    CHECK(test_keymap() == 0);
    CHECK(test_menu_projection_independence() == 0);
    CHECK(test_menu_validation_and_render() == 0);

    puts("rivet ui tests: ok");
    return 0;
}
