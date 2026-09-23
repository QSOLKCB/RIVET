# RIVET Runtime and Memory Plan

**Status: planning document for R1 and later. No runtime implementation is established by this file.**

This plan converts proven donor mechanisms into a deliberately small RIVET implementation sequence. Exact donor links and adoption boundaries are recorded in [DONORS.md](DONORS.md).

## Prime runtime rule

> **The second implementation earns the abstraction.**

R1 must not begin with an executor hierarchy, scheduler framework, plugin architecture, generic cache system, worker pool, task graph, or backend factory merely because later phases might need one.

The first implementation should be the smallest explicit code that satisfies the current contract.

Source precedent: [QSOL-MESH PR #3](https://github.com/QSOLKCB/QSOL-MESH/pull/3).

## P1 — Explicit state and bounded ownership

Initial runtime state should be plainly owned, inspectable, bounded where growth depends on external input, free of hidden global services, and valid in single-threaded execution.

No runtime object should exist solely to make a diagram symmetrical.

## P2 — Logical scale is not resident scale

Applications/documents may describe more logical work than should be resident simultaneously.

Where semantics permit, RIVET should use bounded windows/chunks rather than allocate state proportional to the complete logical domain.

Donors: [GALAXY v0.4.0](https://github.com/QSOLKCB/GALAXY/releases/tag/v0.4.0) and [IGM execution campaigns](https://github.com/QSOLKCB/igm/blob/main/docs/EXECUTION_CAMPAIGNS.md).

## P3 — Stream → Consume/Reduce → Discard

Transient state should not earn permanent residency merely because it was convenient to compute.

Candidate applications:

```text
network bytes -> parse -> commit useful semantic state -> discard transport scratch
layout region -> bounded paint state -> rasterise -> discard transient paint state
decoded block -> consume/cache only if justified -> discard decoder scratch
```

Source precedent: [GALAXY v0.6.0](https://github.com/QSOLKCB/GALAXY/releases/tag/v0.6.0).

This rule does not authorize transformations that change HTML/CSS/document semantics. Persistent state remains persistent when later semantics require it.

## P4 — Explicit memory budgets

When externally supplied content can grow resident state, the operation should accept or derive an explicit bounded budget before expensive work begins.

A budget plan should fail closed on zero/invalid limits, arithmetic overflow, minimum-object impossibility, unbounded chunk counts, or a representation that cannot fit the declared target profile.

Initial constants are not copied from donors. RIVET establishes target-specific budgets through its own profiles and evidence.

Source precedent: [IGM execution campaigns](https://github.com/QSOLKCB/igm/blob/main/docs/EXECUTION_CAMPAIGNS.md).

## P5 — Result identity != execution-plan identity

Where semantics are invariant, changing worker count, chunk size/count, resident memory budget, cache allocation, or optional SIMD path must not create a different correctness result merely because execution machinery changed.

```text
RESULT IDENTITY != EXECUTION PLAN IDENTITY
BENCHMARK OBSERVATION != CORRECTNESS IDENTITY
```

Source precedent: [IGM execution campaigns](https://github.com/QSOLKCB/igm/blob/main/docs/EXECUTION_CAMPAIGNS.md).

## P6 — Reference path before promotion

The smallest scalar/single-threaded/reference implementation remains available where practical.

An optimized CPU path is promotable only after reference parity passes, the workload justifies the added code, any declared promotion margin is met, and failure returns to a correct explicit path or fails closed under the contract.

Donors: [GALAXY v0.5.0](https://github.com/QSOLKCB/GALAXY/releases/tag/v0.5.0) and [OPT v1.0.0](https://github.com/QSOLKCB/OPT/releases/tag/v1.0.0).

## P7 — Reduce work before accelerating it

Before adding SIMD, threads, specialized CPU paths or elaborate caches: eliminate unnecessary work, bound materialization, improve invalidation granularity, move safe reductions earlier, reuse equivalent results, measure, then accelerate only justified remainder.

This operationalizes `RIVET-INV-020`.

Useful donor records: [OPT-REDUCE-001](https://github.com/QSOLKCB/OPT/blob/main/optimizations/OPT-REDUCE-001-early-working-set-reduction.md) and [OPT-INC-001](https://github.com/QSOLKCB/OPT/blob/main/optimizations/OPT-INC-001-signature-bound-incremental-execution.md).

## P8 — Reuse/cache only under complete identity

A cache hit or reusable materialization is valid only when its identity covers every effective input capable of changing the consumed result.

Validation and later consumption must refer to the same immutable/versioned artifact or equivalent protected snapshot.

Donors: [OPT-INC-001](https://github.com/QSOLKCB/OPT/blob/main/optimizations/OPT-INC-001-signature-bound-incremental-execution.md) and [OPT-FAN-001](https://github.com/QSOLKCB/OPT/blob/main/optimizations/OPT-FAN-001-shared-materialization-fanout.md).

R1 does not need a generic cache framework. This rule applies when a real cache/reuse case appears.

## P9 — Duplicate work is future-only

Concurrent equivalent work may eventually share an in-flight computation only if cancellation, deadlines, errors, ownership and result-isolation semantics remain correct.

Source: [OPT-COAL-001](https://github.com/QSOLKCB/OPT/blob/main/optimizations/OPT-COAL-001-concurrent-duplicate-work-coalescing.md).

This is a future optimization mechanism, not an R1 requirement.

## P10 — Simple defaults do not remove expert capability

A compact default UI/profile may expose only common choices while stable commands, readable configuration or bounded adapters retain legitimate advanced capability.

Source precedent: [QSOLKCB/C64 architecture](https://github.com/QSOLKCB/C64/blob/main/docs/ARCHITECTURE.md).

## Proposed R1 implementation order

1. smallest C99 error/result vocabulary needed by real calls;
2. capability query over static target declarations;
3. explicit event loop with no mandatory threads;
4. command registry;
5. minimal allocator/budget hooks only where a real bounded allocation exists;
6. one headless host test application;
7. deterministic tests and size evidence.

Do **not** add chunk planners, caches, workers, SIMD, topology calibration or persistent pools until a real later phase demonstrates the need.

## Later phase mapping

| Mechanism | Earliest likely phase | Gate |
|---|---|---|
| explicit allocation budgets | R1/R2 | external-input growth exists |
| dirty-region painting | R2/R3 | full repaint measured waste |
| bounded document streaming | R7 | parser semantics permit it |
| incremental layout | R7/R11 | full relayout measured waste |
| reusable glyph/image state | R2/R7 | complete identity + bounded retention |
| SIMD | R11 | reference parity + meaningful measured gain |
| bounded threading | R11 | single-thread path insufficient + parity |
| host calibration | R11 | at least two real equivalent CPU paths exist |
| duplicate-work coalescing | R11+ | concurrent duplicate work exists |

## Non-goals

This plan does not introduce GPU work, an async runtime, a general scheduler, work stealing, mandatory worker pools, generic dependency injection, an application server, a database, automatic network services, or performance claims inherited from donor repositories.
