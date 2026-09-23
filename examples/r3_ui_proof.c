/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/ui.h"
#include "ppm.h"

#include <stdio.h>

#define PROOF_WIDTH 96ul
#define PROOF_HEIGHT 48ul
#define PROOF_BYTES ((size_t)PROOF_WIDTH * (size_t)PROOF_HEIGHT * RIVET_GFX_PIXEL_BYTES)
#define PROOF_FNV1A64 0x8e022a6d842ff8e5ULL

typedef struct proof_state {
    int opened;
    int saved;
    int quit;
} proof_state;

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

static rivet_result open_command(void *context)
{
    proof_state *state = (proof_state *)context;
    ++state->opened;
    return RIVET_OK;
}

static rivet_result save_command(void *context)
{
    proof_state *state = (proof_state *)context;
    ++state->saved;
    return RIVET_OK;
}

static rivet_result quit_command(void *context)
{
    proof_state *state = (proof_state *)context;
    ++state->quit;
    return RIVET_OK;
}

int main(int argc, char **argv)
{
    unsigned char pixels[PROOF_BYTES];
    rivet_surface surface;
    rivet_command_slot command_slots[3];
    rivet_command_registry commands;
    const rivet_menu_item menu_items[3] = {
        {"OPEN", "demo.open"},
        {"SAVE", "demo.save"},
        {"QUIT", "demo.quit"}
    };
    const rivet_key_binding bindings[3] = {
        {'O', RIVET_MOD_CTRL, "demo.open"},
        {'S', RIVET_MOD_CTRL, "demo.save"},
        {'Q', RIVET_MOD_CTRL, "demo.quit"}
    };
    rivet_keymap keymap = {bindings, 3u};
    rivet_menu menu;
    rivet_menu_style style = {
        {0x18u,0x20u,0x28u,0xffu},
        {0xc0u,0xc8u,0xd0u,0xffu},
        {0x48u,0x50u,0x58u,0xffu},
        {0xf0u,0xb4u,0x4du,0xffu}
    };
    rivet_rect full = {0L,0L,PROOF_WIDTH,PROOF_HEIGHT};
    rivet_rect menu_bounds = {8L,7L,64ul,33ul};
    rivet_rgba8 background = {0x08u,0x0cu,0x10u,0xffu};
    rivet_key_event down = {RIVET_KEY_DOWN,0u,1};
    rivet_key_event enter = {RIVET_KEY_ENTER,0u,1};
    rivet_key_event ctrl_o = {'O',RIVET_MOD_CTRL,1};
    proof_state state = {0,0,0};
    const char *output_path =
        argc > 1 ? argv[1] : "build/rivet-ui-proof.ppm";
    unsigned long long hash;

    if (rivet_surface_attach(
            &surface,
            pixels,
            sizeof(pixels),
            PROOF_WIDTH,
            PROOF_HEIGHT,
            (size_t)PROOF_WIDTH * RIVET_GFX_PIXEL_BYTES) != RIVET_OK ||
        rivet_surface_fill_rect(
            &surface,
            full,
            background) != RIVET_OK ||
        rivet_commands_init(
            &commands,
            command_slots,
            3u) != RIVET_OK ||
        rivet_commands_add(
            &commands,
            "demo.open",
            open_command,
            &state) != RIVET_OK ||
        rivet_commands_add(
            &commands,
            "demo.save",
            save_command,
            &state) != RIVET_OK ||
        rivet_commands_add(
            &commands,
            "demo.quit",
            quit_command,
            &state) != RIVET_OK ||
        rivet_keymap_validate(&keymap) != RIVET_OK ||
        rivet_menu_init(
            &menu,
            menu_items,
            3u) != RIVET_OK ||
        rivet_menu_handle_key(
            &menu,
            &commands,
            down) != RIVET_OK ||
        menu.selected != 1u ||
        rivet_menu_render(
            &surface,
            &menu,
            menu_bounds,
            style) != RIVET_OK ||
        rivet_menu_handle_key(
            &menu,
            &commands,
            enter) != RIVET_OK ||
        state.saved != 1 ||
        rivet_menu_set_visible(
            &menu,
            0) != RIVET_OK ||
        rivet_menu_handle_key(
            &menu,
            &commands,
            down) != RIVET_ERR_NOT_FOUND ||
        rivet_keymap_dispatch(
            &keymap,
            &commands,
            ctrl_o) != RIVET_OK ||
        state.opened != 1 ||
        state.quit != 0) {
        return 1;
    }

    hash = fnv1a64(pixels, sizeof(pixels));
    if (hash != PROOF_FNV1A64) {
        fprintf(
            stderr,
            "unexpected UI pixel hash: %016llx\n",
            hash
        );
        return 1;
    }

    if (!rivet_headless_write_ppm(output_path, &surface)) {
        return 1;
    }

    printf(
        "rivet-r3: ok fnv1a64=%016llx open=%d save=%d quit=%d menu=%d ppm=%s\n",
        hash,
        state.opened,
        state.saved,
        state.quit,
        menu.visible,
        output_path
    );
    return 0;
}
