/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/rivet.h"

static int rivet_id_valid(const char *id)
{
    return id != NULL && id[0] != '\0';
}

static int rivet_streq(const char *left, const char *right)
{
    if (left == NULL || right == NULL) {
        return 0;
    }

    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == *right;
}

static int rivet_registry_valid(const rivet_command_registry *registry)
{
    return registry != NULL &&
           registry->slots != NULL &&
           registry->capacity != 0u &&
           registry->count <= registry->capacity;
}

static int rivet_loop_valid(const rivet_loop *loop)
{
    return loop != NULL &&
           loop->events != NULL &&
           loop->capacity != 0u &&
           loop->head < loop->capacity &&
           loop->count <= loop->capacity;
}

const char *rivet_result_name(rivet_result result)
{
    switch (result) {
    case RIVET_OK:
        return "ok";
    case RIVET_ERR_INVALID_ARGUMENT:
        return "invalid-argument";
    case RIVET_ERR_CAPACITY:
        return "capacity";
    case RIVET_ERR_NOT_FOUND:
        return "not-found";
    case RIVET_ERR_DUPLICATE:
        return "duplicate";
    case RIVET_ERR_STOPPED:
        return "stopped";
    case RIVET_ERR_UNSUPPORTED:
        return "unsupported";
    default:
        return "unknown";
    }
}

int rivet_capability_has(const rivet_capability_set *set, const char *id)
{
    size_t i;

    if (set == NULL || !rivet_id_valid(id)) {
        return 0;
    }
    if (set->count != 0u && set->ids == NULL) {
        return 0;
    }

    for (i = 0u; i < set->count; ++i) {
        if (rivet_streq(set->ids[i], id)) {
            return 1;
        }
    }

    return 0;
}

rivet_result rivet_commands_init(
    rivet_command_registry *registry,
    rivet_command_slot *slots,
    size_t capacity
)
{
    if (registry == NULL || slots == NULL || capacity == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    registry->slots = slots;
    registry->capacity = capacity;
    registry->count = 0u;
    return RIVET_OK;
}

rivet_result rivet_commands_add(
    rivet_command_registry *registry,
    const char *id,
    rivet_command_fn fn,
    void *context
)
{
    size_t i;

    if (!rivet_registry_valid(registry) || !rivet_id_valid(id) || fn == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < registry->count; ++i) {
        if (rivet_streq(registry->slots[i].id, id)) {
            return RIVET_ERR_DUPLICATE;
        }
    }

    if (registry->count == registry->capacity) {
        return RIVET_ERR_CAPACITY;
    }

    registry->slots[registry->count].id = id;
    registry->slots[registry->count].fn = fn;
    registry->slots[registry->count].context = context;
    ++registry->count;

    return RIVET_OK;
}

rivet_result rivet_commands_dispatch(
    const rivet_command_registry *registry,
    const char *id
)
{
    size_t i;

    if (!rivet_registry_valid(registry) || !rivet_id_valid(id)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    for (i = 0u; i < registry->count; ++i) {
        if (rivet_streq(registry->slots[i].id, id)) {
            return registry->slots[i].fn(registry->slots[i].context);
        }
    }

    return RIVET_ERR_NOT_FOUND;
}

rivet_result rivet_loop_init(
    rivet_loop *loop,
    rivet_event *events,
    size_t capacity
)
{
    if (loop == NULL || events == NULL || capacity == 0u) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    loop->events = events;
    loop->capacity = capacity;
    loop->head = 0u;
    loop->count = 0u;
    loop->stopped = 0;
    return RIVET_OK;
}

rivet_result rivet_loop_post(
    rivet_loop *loop,
    rivet_event_fn fn,
    void *context
)
{
    size_t tail;

    if (!rivet_loop_valid(loop) || fn == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }
    if (loop->stopped) {
        return RIVET_ERR_STOPPED;
    }
    if (loop->count == loop->capacity) {
        return RIVET_ERR_CAPACITY;
    }

    if (loop->count >= loop->capacity - loop->head) {
        tail = loop->count - (loop->capacity - loop->head);
    } else {
        tail = loop->head + loop->count;
    }
    loop->events[tail].fn = fn;
    loop->events[tail].context = context;
    ++loop->count;

    return RIVET_OK;
}

rivet_result rivet_loop_step(
    rivet_loop *loop,
    int *did_work
)
{
    rivet_event event;

    if (!rivet_loop_valid(loop) || did_work == NULL) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    *did_work = 0;

    if (loop->stopped) {
        return RIVET_ERR_STOPPED;
    }
    if (loop->count == 0u) {
        return RIVET_OK;
    }

    event = loop->events[loop->head];
    loop->head = (loop->head + 1u) % loop->capacity;
    --loop->count;
    *did_work = 1;

    return event.fn(event.context);
}

rivet_result rivet_loop_stop(rivet_loop *loop)
{
    if (!rivet_loop_valid(loop)) {
        return RIVET_ERR_INVALID_ARGUMENT;
    }

    loop->stopped = 1;
    return RIVET_OK;
}
