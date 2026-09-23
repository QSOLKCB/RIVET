/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/ui.h"
#include "textview.h"
#include "ppm.h"
#include "text_file.h"

#include <stdio.h>

#define PROOF_WIDTH 192ul
#define PROOF_HEIGHT 48ul
#define PROOF_BYTES ((size_t)PROOF_WIDTH * (size_t)PROOF_HEIGHT * RIVET_GFX_PIXEL_BYTES)
#define PROOF_FNV1A64 0ULL

typedef struct proof_context {
    rivet_textview *viewer;
    const unsigned char *query;
    size_t query_length;
} proof_context;

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

static rivet_result command_down(void *context)
{
    proof_context *state = (proof_context *)context;
    return rivet_textview_line_down(state->viewer);
}

static rivet_result command_up(void *context)
{
    proof_context *state = (proof_context *)context;
    return rivet_textview_line_up(state->viewer);
}

static rivet_result command_find(void *context)
{
    proof_context *state = (proof_context *)context;
    return rivet_textview_find_next(
        state->viewer,
        state->query,
        state->query_length
    );
}

int main(int argc, char **argv)
{
    unsigned char file_bytes[1024];
    size_t file_count = 0u;
    size_t line_offsets[32];
    unsigned char pixels[PROOF_BYTES];
    rivet_surface surface;
    rivet_textview viewer;
    rivet_command_slot slots[3];
    rivet_command_registry commands;
    const rivet_key_binding bindings[3] = {
        {RIVET_KEY_DOWN, 0u, "view.down"},
        {RIVET_KEY_UP, 0u, "view.up"},
        {0x46u, RIVET_MOD_CTRL, "view.find"}
    };
    rivet_keymap keymap = {bindings, 3u};
    static const unsigned char query_pixels[] = {
        0x50u,0x49u,0x58u,0x45u,0x4cu,0x53u
    };
    proof_context context;
    rivet_key_event down = {RIVET_KEY_DOWN,0u,1};
    rivet_key_event find = {0x46u,RIVET_MOD_CTRL,1};
    rivet_rect full = {0L,0L,PROOF_WIDTH,PROOF_HEIGHT};
    rivet_textview_style style = {
        {0x08u,0x0cu,0x10u,0xffu},
        {0xc0u,0xc8u,0xd0u,0xffu},
        {0x38u,0x40u,0x48u,0xffu},
        {0xf0u,0xb4u,0x4du,0xffu}
    };
    rivet_rgba8 clear = {0x08u,0x0cu,0x10u,0xffu};
    const char *input_path =
        argc > 1 ? argv[1] : "fixtures/r4_textview.txt";
    const char *output_path =
        argc > 2 ? argv[2] : "build/rivet-textview-proof.ppm";
    unsigned long long hash;

    if (!rivet_headless_read_text_file(
            input_path,
            file_bytes,
            sizeof(file_bytes),
            &file_count) ||
        rivet_textview_open(
            &viewer,
            file_bytes,
            file_count,
            line_offsets,
            sizeof(line_offsets) / sizeof(line_offsets[0])) != RIVET_OK ||
        rivet_surface_attach(
            &surface,
            pixels,
            sizeof(pixels),
            PROOF_WIDTH,
            PROOF_HEIGHT,
            (size_t)PROOF_WIDTH * RIVET_GFX_PIXEL_BYTES) != RIVET_OK ||
        rivet_surface_fill_rect(
            &surface,
            full,
            clear) != RIVET_OK ||
        rivet_commands_init(
            &commands,
            slots,
            3u) != RIVET_OK) {
        return 1;
    }

    context.viewer = &viewer;
    context.query = query_pixels;
    context.query_length = sizeof(query_pixels);

    if (rivet_commands_add(
            &commands,
            "view.down",
            command_down,
            &context) != RIVET_OK ||
        rivet_commands_add(
            &commands,
            "view.up",
            command_up,
            &context) != RIVET_OK ||
        rivet_commands_add(
            &commands,
            "view.find",
            command_find,
            &context) != RIVET_OK ||
        rivet_keymap_validate(&keymap) != RIVET_OK ||
        rivet_keymap_dispatch(
            &keymap,
            &commands,
            down) != RIVET_OK ||
        rivet_keymap_dispatch(
            &keymap,
            &commands,
            down) != RIVET_OK ||
        viewer.top_line != 2u ||
        rivet_keymap_dispatch(
            &keymap,
            &commands,
            find) != RIVET_OK ||
        viewer.top_line != 4u ||
        !viewer.has_match ||
        rivet_textview_render(
            &surface,
            &viewer,
            full,
            style) != RIVET_OK) {
        return 1;
    }

    hash = fnv1a64(pixels, sizeof(pixels));

    if (!rivet_headless_write_ppm(output_path, &surface)) {
        return 1;
    }

    printf(
        "rivet-r4: fnv1a64=%016llx bytes=%lu lines=%lu top=%lu match=%lu ppm=%s\n",
        hash,
        (unsigned long)file_count,
        (unsigned long)viewer.line_count,
        (unsigned long)viewer.top_line,
        (unsigned long)viewer.match_offset,
        output_path
    );

    if (PROOF_FNV1A64 != 0ULL &&
        hash != PROOF_FNV1A64) {
        return 1;
    }

    return 0;
}
