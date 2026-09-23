/* SPDX-License-Identifier: MPL-2.0 */

#include "rivet/rivet.h"

#include <stdio.h>
#include <string.h>

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

typedef struct record_context {
    int value;
    int *output;
    size_t *count;
} record_context;

static rivet_result record_value(void *context)
{
    record_context *record = (record_context *)context;
    record->output[*record->count] = record->value;
    ++(*record->count);
    return RIVET_OK;
}

static rivet_result return_unsupported(void *context)
{
    (void)context;
    return RIVET_ERR_UNSUPPORTED;
}

static int test_result_model(void)
{
    CHECK(RIVET_ABI_VERSION == 1u);
    CHECK(strcmp(rivet_result_name(RIVET_OK), "ok") == 0);
    CHECK(strcmp(rivet_result_name(RIVET_ERR_CAPACITY), "capacity") == 0);
    CHECK(strcmp(rivet_result_name((rivet_result)99), "unknown") == 0);
    return 0;
}

static int test_capabilities(void)
{
    const char *const ids[] = {"alpha", "beta", "timer.monotonic"};
    rivet_capability_set set = {ids, 3u};
    rivet_capability_set empty = {NULL, 0u};

    CHECK(rivet_capability_has(&set, "alpha") == 1);
    CHECK(rivet_capability_has(&set, "timer.monotonic") == 1);
    CHECK(rivet_capability_has(&set, "Alpha") == 0);
    CHECK(rivet_capability_has(&set, "missing") == 0);
    CHECK(rivet_capability_has(&empty, "alpha") == 0);
    CHECK(rivet_capability_has(NULL, "alpha") == 0);
    CHECK(rivet_capability_has(&set, NULL) == 0);
    return 0;
}

static int test_commands(void)
{
    rivet_command_slot slots[2];
    rivet_command_registry registry;
    int first = 0;
    int second = 0;

    CHECK(rivet_commands_init(&registry, slots, 2u) == RIVET_OK);
    CHECK(rivet_commands_add(&registry, "first", increment, &first) == RIVET_OK);
    CHECK(rivet_commands_add(&registry, "first", increment, &first) == RIVET_ERR_DUPLICATE);
    CHECK(rivet_commands_add(&registry, "second", increment, &second) == RIVET_OK);
    CHECK(rivet_commands_add(&registry, "third", increment, &second) == RIVET_ERR_CAPACITY);

    CHECK(rivet_commands_dispatch(&registry, "first") == RIVET_OK);
    CHECK(first == 1);
    CHECK(second == 0);
    CHECK(rivet_commands_dispatch(&registry, "missing") == RIVET_ERR_NOT_FOUND);

    CHECK(rivet_commands_init(NULL, slots, 2u) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_commands_init(&registry, NULL, 2u) == RIVET_ERR_INVALID_ARGUMENT);
    CHECK(rivet_commands_init(&registry, slots, 0u) == RIVET_ERR_INVALID_ARGUMENT);
    return 0;
}

static int test_loop_fifo(void)
{
    rivet_event events[3];
    rivet_loop loop;
    int output[3] = {0, 0, 0};
    size_t count = 0u;
    record_context contexts[3] = {
        {1, output, &count},
        {2, output, &count},
        {3, output, &count}
    };
    int did_work = 0;

    CHECK(rivet_loop_init(&loop, events, 3u) == RIVET_OK);
    CHECK(rivet_loop_post(&loop, record_value, &contexts[0]) == RIVET_OK);
    CHECK(rivet_loop_post(&loop, record_value, &contexts[1]) == RIVET_OK);
    CHECK(rivet_loop_post(&loop, record_value, &contexts[2]) == RIVET_OK);
    CHECK(rivet_loop_post(&loop, record_value, &contexts[0]) == RIVET_ERR_CAPACITY);

    CHECK(rivet_loop_step(&loop, &did_work) == RIVET_OK && did_work == 1);
    CHECK(rivet_loop_step(&loop, &did_work) == RIVET_OK && did_work == 1);
    CHECK(rivet_loop_step(&loop, &did_work) == RIVET_OK && did_work == 1);
    CHECK(count == 3u);
    CHECK(output[0] == 1 && output[1] == 2 && output[2] == 3);

    CHECK(rivet_loop_step(&loop, &did_work) == RIVET_OK && did_work == 0);
    return 0;
}

static int test_loop_failure_and_stop(void)
{
    rivet_event events[1];
    rivet_loop loop;
    int did_work = 0;

    CHECK(rivet_loop_init(&loop, events, 1u) == RIVET_OK);
    CHECK(rivet_loop_post(&loop, return_unsupported, NULL) == RIVET_OK);
    CHECK(rivet_loop_step(&loop, &did_work) == RIVET_ERR_UNSUPPORTED);
    CHECK(did_work == 1);

    CHECK(rivet_loop_stop(&loop) == RIVET_OK);
    CHECK(rivet_loop_post(&loop, return_unsupported, NULL) == RIVET_ERR_STOPPED);
    CHECK(rivet_loop_step(&loop, &did_work) == RIVET_ERR_STOPPED);
    CHECK(did_work == 0);
    return 0;
}

int main(void)
{
    CHECK(test_result_model() == 0);
    CHECK(test_capabilities() == 0);
    CHECK(test_commands() == 0);
    CHECK(test_loop_fifo() == 0);
    CHECK(test_loop_failure_and_stop() == 0);

    puts("rivet core tests: ok");
    return 0;
}
