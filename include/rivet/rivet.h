/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RIVET_RIVET_H
#define RIVET_RIVET_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RIVET_ABI_VERSION 1u

typedef enum rivet_result {
    RIVET_OK = 0,
    RIVET_ERR_INVALID_ARGUMENT = 1,
    RIVET_ERR_CAPACITY = 2,
    RIVET_ERR_NOT_FOUND = 3,
    RIVET_ERR_DUPLICATE = 4,
    RIVET_ERR_STOPPED = 5,
    RIVET_ERR_UNSUPPORTED = 6
} rivet_result;

const char *rivet_result_name(rivet_result result);

typedef struct rivet_capability_set {
    const char *const *ids;
    size_t count;
} rivet_capability_set;

int rivet_capability_has(const rivet_capability_set *set, const char *id);

typedef rivet_result (*rivet_command_fn)(void *context);

typedef struct rivet_command_slot {
    const char *id;
    rivet_command_fn fn;
    void *context;
} rivet_command_slot;

typedef struct rivet_command_registry {
    rivet_command_slot *slots;
    size_t capacity;
    size_t count;
} rivet_command_registry;

rivet_result rivet_commands_init(
    rivet_command_registry *registry,
    rivet_command_slot *slots,
    size_t capacity
);

rivet_result rivet_commands_add(
    rivet_command_registry *registry,
    const char *id,
    rivet_command_fn fn,
    void *context
);

rivet_result rivet_commands_dispatch(
    const rivet_command_registry *registry,
    const char *id
);

typedef rivet_result (*rivet_event_fn)(void *context);

typedef struct rivet_event {
    rivet_event_fn fn;
    void *context;
} rivet_event;

typedef struct rivet_loop {
    rivet_event *events;
    size_t capacity;
    size_t head;
    size_t count;
    int stopped;
} rivet_loop;

rivet_result rivet_loop_init(
    rivet_loop *loop,
    rivet_event *events,
    size_t capacity
);

rivet_result rivet_loop_post(
    rivet_loop *loop,
    rivet_event_fn fn,
    void *context
);

rivet_result rivet_loop_step(
    rivet_loop *loop,
    int *did_work
);

rivet_result rivet_loop_stop(rivet_loop *loop);

#ifdef __cplusplus
}
#endif

#endif
