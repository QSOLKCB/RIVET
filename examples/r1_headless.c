/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/rivet.h"

#include <stdio.h>

typedef struct demo {
    rivet_command_registry *commands;
    int count;
} demo;

static rivet_result ping(void *context)
{
    demo *state = (demo *)context;
    ++state->count;
    return RIVET_OK;
}

static rivet_result dispatch_ping(void *context)
{
    demo *state = (demo *)context;
    return rivet_commands_dispatch(state->commands, "demo.ping");
}

int main(void)
{
    const char *const capability_ids[] = {"demo.headless"};
    rivet_capability_set capabilities = {capability_ids, 1u};
    rivet_command_slot command_slots[1];
    rivet_command_registry commands;
    rivet_event event_slots[1];
    rivet_loop loop;
    demo state;
    int did_work = 0;
    rivet_result result;

    state.commands = &commands;
    state.count = 0;

    result = rivet_commands_init(&commands, command_slots, 1u);
    if (result != RIVET_OK) {
        return 1;
    }
    result = rivet_commands_add(&commands, "demo.ping", ping, &state);
    if (result != RIVET_OK) {
        return 1;
    }
    result = rivet_loop_init(&loop, event_slots, 1u);
    if (result != RIVET_OK) {
        return 1;
    }
    result = rivet_loop_post(&loop, dispatch_ping, &state);
    if (result != RIVET_OK) {
        return 1;
    }
    result = rivet_loop_step(&loop, &did_work);
    if (result != RIVET_OK || did_work != 1 || state.count != 1) {
        return 1;
    }
    if (!rivet_capability_has(&capabilities, "demo.headless")) {
        return 1;
    }

    printf("rivet-r1: ok abi=%u commands=%lu events=%lu heap=0\n",
           (unsigned)RIVET_ABI_VERSION,
           (unsigned long)commands.count,
           (unsigned long)state.count);
    return 0;
}
