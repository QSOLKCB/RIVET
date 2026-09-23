# RIVET Core Contract v1

## Claim boundary

RIVET Core v1 is the first executable R1 substrate. It provides only:

- an explicit result/error vocabulary;
- exact string-membership capability queries;
- a fixed-capacity command registry;
- a fixed-capacity single-threaded FIFO event loop.

It does **not** claim a GUI, renderer, platform backend, filesystem, network stack, threading runtime, scheduler, cache system, SIMD path, or performance advantage.

Machine-readable identity: `machine/core-v1.json`.

## ABI

The public C header is `include/rivet/rivet.h`.

```text
RIVET_ABI_VERSION = 1
language baseline  = C99
```

The public structs are intentionally plain and inspectable. `rivet_result` is an explicit C `int`; result constants are integer constants rather than an implementation-sized enum type. A future incompatible public type/layout change requires a new ABI identity rather than silently changing ABI v1.

## Memory boundary

Core v1 requires no heap allocation.

The caller owns:

- command-slot storage;
- event-queue storage;
- command ID strings;
- callback contexts.

RIVET stores references to caller-owned IDs/contexts but does not copy or free them. Those objects must remain valid for as long as the corresponding registry entries/events may be used.

Fixed-capacity exhaustion returns `RIVET_ERR_CAPACITY`.

This deliberately replaces the R0 roadmap's generic "allocator boundary" idea with the smaller boundary actually required by R1: caller-owned bounded storage. A general allocator abstraction is deferred until a real second allocation policy earns it.

## Result model

Core v1 exposes these stable values:

```text
0 RIVET_OK
1 RIVET_ERR_INVALID_ARGUMENT
2 RIVET_ERR_CAPACITY
3 RIVET_ERR_NOT_FOUND
4 RIVET_ERR_DUPLICATE
5 RIVET_ERR_STOPPED
6 RIVET_ERR_UNSUPPORTED
```

`rivet_result_name()` provides stable diagnostic names for those values.

## Capability query

`rivet_capability_has()` performs exact, case-sensitive membership against a caller-supplied capability set.

The function returns a `rivet_result` and writes membership through an `int *has` output only after the complete supplied set has been validated.

- present capability -> `RIVET_OK`, `has = 1`;
- absent capability -> `RIVET_OK`, `has = 0`;
- null/empty requested ID, null output pointer, impossible set storage, or null/empty set entry -> `RIVET_ERR_INVALID_ARGUMENT`.

On invalid input the output value is left unchanged. Malformed input is therefore never collapsed into the valid "capability absent" state.

It does not infer capabilities from OS, CPU, target age, or naming conventions. It also does not prove that a declaration is truthful; platform adapters and tests must establish that evidence.

Canonical capability names remain governed by the current capability contract rather than this primitive.

## Command registry

The command registry:

- uses caller-owned fixed-capacity slots;
- rejects empty/null command IDs;
- rejects null callbacks;
- rejects duplicate command IDs;
- returns explicit capacity exhaustion;
- dispatches by exact command identity;
- rejects malformed or duplicate caller-mutated active slots;
- preserves callback return values.

Registration order does not redefine command identity.

## Event loop

The R1 event loop is deliberately single-threaded.

It:

- uses caller-owned fixed-capacity FIFO storage;
- performs no hidden allocation;
- processes at most one queued event per `rivet_loop_step()`;
- reports an empty queue through `RIVET_OK` with `did_work = 0`;
- rejects a malformed queued null callback without consuming it;
- consumes a valid event before invoking its callback;
- propagates the callback result;
- rejects new work after `rivet_loop_stop()`.

There is no worker pool, async runtime, work stealing, background thread, timer implementation, I/O multiplexer, or platform message pump in Core v1.

## Determinism

For the same registry contents, posted event sequence and callback behavior, the core's own dispatch/queue ordering is deterministic.

External callback behavior remains application/platform responsibility.

## Reference implementation

```text
include/rivet/rivet.h
core/rivet.c
```

The headless R1 proof is `examples/r1_headless.c`.

The deterministic unit suite is `tests/test_core.c`.

## Non-goals

Core v1 does not introduce:

- dynamic allocation;
- generic allocator vtables;
- GUI/windowing;
- rendering;
- networking;
- filesystem services;
- threads;
- SIMD;
- GPU APIs;
- scripting;
- plugin loading;
- executor/scheduler abstractions;
- automatic capability detection;
- performance calibration.
