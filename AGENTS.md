# RIVET Agent and Contributor Contract

Read [CONSTITUTION-v2.md](CONSTITUTION-v2.md) before changing architecture. `CONSTITUTION.md` is the frozen v1 authority and must not be rewritten.

## Prime directive

**Prefer the smallest implementation that completely satisfies the declared contract without weakening correctness, safety, readability, portability, evidence, or user capability.**

Every abstraction must pay rent.

## Forbidden architectural drift

Do not introduce any of the following as a required RIVET-core dependency without a dedicated constitutional/architecture PR:

- Electron;
- Chromium/Gecko/WebKit embedding;
- OS WebView;
- Node.js;
- React or another JavaScript UI framework;
- mandatory C++ runtime;
- mandatory Rust runtime;
- mandatory network;
- mandatory threads;
- cloud service;
- database server.

This does not forbid optional adapters or language bindings. It forbids silently making them the foundation.

## Absolute GPU prohibition

RIVET contributors and coding agents must not introduce GPU rendering or compute APIs as required **or optional** RIVET framework machinery.

This prohibition applies to the core, UI, document engine, RIVET Browser, framework adapters, and capability registry, including OpenGL, Vulkan, Direct3D, Metal, WebGL, WebGPU, CUDA, GPU compute, and shader-based rendering.

The optional-adapter allowance above does **not** create an exception to this rule.

A specialised application may use external GPU code outside the RIVET rendering contract, but that code is not a RIVET capability or rendering adapter and must not be promoted into framework authority.

Host OS/window-system compositing of a completed RIVET pixel surface remains outside the RIVET contract.

## Runtime planning rule

Before implementing runtime/memory infrastructure, read [RUNTIME-PLAN-v1.md](RUNTIME-PLAN-v1.md). It is a plan, not permission to pre-build abstractions. The second concrete implementation earns an abstraction.

## Before adding code

Ask in order:

1. Does this capability already exist?
2. Can the goal be met by deleting or simplifying code instead?
3. Can an existing primitive be reused without semantic ambiguity?
4. Is a dependency genuinely safer/smaller/more portable than a local implementation?
5. Does this belong in core, or is it an application/platform/adapter concern?
6. What is the minimum test that proves the contract?

## Code shape

Prefer:

- plain data;
- explicit state;
- short call paths;
- boring control flow;
- fixed ownership;
- bounded memory;
- small files with real cohesion;
- stable C ABI at public boundaries;
- compile-time/runtime feature declarations over hidden autodetection.

Avoid:

- wrapper chains;
- speculative genericity;
- class/type hierarchies for hypothetical future use;
- duplicated source-of-truth state;
- magic fallback;
- code generation where a tiny checked-in definition is clearer;
- dynamic plugin systems before a real plugin use case exists.

## Portability

Never equate:

```text
compiled == executed
container == target OS
CPU family == operating system
emulator == physical hardware
fallback == requested backend
```

Every target claim must match the evidence actually produced.

## Tests

Tests should protect invariants, not implementation trivia.

When a bug appears:

1. reduce to a minimal reproduction;
2. identify the violated invariant;
3. fix the invariant/general condition where practical;
4. retain the reproduction as regression evidence;
5. verify legitimate behaviour still works.

## Performance

Do not optimise by weakening semantics.

RIVET targets software pixel surfaces, not GPUs. Do not add a GPU path as an optimisation.

Optimise in this order:

1. eliminate unnecessary work;
2. bound working sets;
3. invalidate/recompute more precisely;
4. reuse safe results;
5. measure;
6. only then consider optional CPU-side SIMD or bounded threading.

Keep a scalar/reference path. Measure before claiming benefit. Record the environment.

## Documentation

Human prose explains why.

Machine contracts define compact normative identities where automation needs them.

Do not duplicate large rule sets in many files; link to the authority.

Published versioned machine contracts and the versioned human authority paths they name are immutable. Frozen legacy authority paths such as `CONSTITUTION.md`, `ARCHITECTURE.md`, and `PORTABILITY.md` must also remain byte-stable for the contract identities that name them. Breaking semantic changes require new contract identities and new versioned authority paths rather than rewriting an existing version in place.

## Phase boundary

R0 remains frozen in immutable `v0.1.0`.

R1 Core v1 through R7 Document v1 are merged and frozen. R6's full-system Windows E3 receipt remains release-gated evidence rather than a reason to rewrite historical contracts.

R8 may add only the downstream WEB1 browser machinery described by [BROWSER-WEB1-v1.md](BROWSER-WEB1-v1.md): bounded browser state, resource-service composition, history/bookmarks, explicit downloads, view source, user styles, keyboard commands, inspectable configuration, software-raster presentation, tests and deterministic evidence.

R8 must not redefine frozen R1-R7 APIs. Treat fetched resources and configuration bytes as hostile input and fail explicitly on malformed syntax, unsupported grammar or caller-capacity exhaustion.

Do not pull R9 historical target claims, R10 relay implementation, R11 caches/incremental-layout/threads/SIMD, JavaScript, cookies, tabs, GPU APIs or speculative browser machinery into WEB1.
