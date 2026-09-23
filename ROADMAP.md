# RIVET Roadmap

The roadmap is deliberately staged so architecture is earned before complexity appears.

## R0 — Constitutional foundation

**Complete and frozen in immutable v0.1.0.**

- freeze project mission and invariants;
- define capability and Minimum Execution Substrate concepts;
- define portability/evidence classes;
- record donor boundaries;
- add compact machine-readable contracts;
- publish a minimal static project page.

No application/runtime implementation belongs in R0.

## R1 — Portable core

**Complete in PR #3.**

R1 follows [RUNTIME-PLAN-v1.md](RUNTIME-PLAN-v1.md):

- the second implementation earns the abstraction;
- no heap proportional to a logical domain when bounded/procedural execution suffices;
- explicit memory budgets where resident state can grow with external input;
- successful-result identity remains separate from workers/chunks/budgets; a budget too small for the minimum representation may fail explicitly with resource exhaustion;
- benchmark observations do not enter correctness identity;
- reuse/cache requires complete effective-input binding.

Implemented R1 core slice:

- [x] explicit result/error model;
- [x] exact capability membership query;
- [x] fixed-capacity command registry;
- [x] single-threaded fixed-capacity FIFO event loop;
- [x] caller-owned bounded storage with no mandatory heap;
- [x] deterministic unit tests;
- [x] tiny headless proof host;
- [x] GCC/Clang C99 CI;
- [x] immutable-v0.1.0 authority regression gate.

The original generic allocator-boundary idea is deliberately deferred. R1 has no dynamic allocation requirement, so a general allocator abstraction would be machinery without a second real allocation policy.

Success criterion: one tiny host program exercises the core with no GUI dependency.

See [CORE-v1.md](CORE-v1.md).
## R2 — Software surface

**Current phase.**

Implemented R2 surface slice:

- [x] caller-owned RGBA8888 byte surface;
- [x] checked width/height/stride/buffer attachment;
- [x] explicit surface validation;
- [x] bounded half-open clip rectangle;
- [x] replace-only rectangle fill;
- [x] MSB-first transparent-zero 1-bit glyph/bitmap blit;
- [x] overlap-safe in-place copy/scroll primitive;
- [x] no-heap canonical raster path;
- [x] headless PPM evidence adapter;
- [x] deterministic pixel FNV + PPM SHA-256 vector;
- [x] GCC/Clang and ASan/UBSan coverage;
- [x] regression gate preserving the R1 Core v1 surface.

The PPM presenter is deliberately an evidence adapter, not a native OS/window backend. Platform presentation remains later work.

No GPU API or GPU rendering path. The CPU/software surface is the rendering architecture.

See [GFX-v1.md](GFX-v1.md).
## R3 — Input + lean UI

Add keyboard-first input and only the widgets required by a demonstration application.

First command projections:

- keyboard binding;
- menu;
- simple command palette if it remains smaller than alternative duplicated UI logic.

Success criterion: capability remains addressable even when a presentation element is removed.

## R4 — Non-browser proof application

Build a small useful application that is **not a browser**.

Candidate: document/text viewer with open, search, save/export where appropriate, keyboard navigation, user configuration, and low-resource evidence.

This proves RIVET is an application substrate rather than a browser project wearing a framework hat.

## R5 — Platform split

Establish at least two materially different platform backends.

Likely early targets:

- modern POSIX;
- Win32.

Run containerised compiler/toolchain matrices and explicit 32/64-bit tests.

## R6 — Historical stress gate

Add reproducible constrained execution:

- 32-bit x86;
- one full-system historical Windows target;
- one big-endian target through emulation.

The exact historical target is chosen based on toolchain feasibility and evidence quality, not nostalgia points.

## R7 — Document engine

Introduce bounded document capabilities:

- URL;
- byte streams;
- text decoding;
- HTML subset;
- CSS subset;
- layout;
- image decode;
- links/forms as separate capabilities.

Parsing hostile data invokes the security/invariant workflow rather than ad-hoc patching.

## R8 — RIVET Browser / WEB1

First browser demonstration.

Required goals:

- useful non-JavaScript browsing;
- history/bookmarks;
- downloads;
- view source;
- user styles;
- keyboard-first operation;
- inspectable configuration;
- measured footprint;
- CPU/software raster rendering with no RIVET GPU path.

## R9 — Retro portability expansion

Target multiple historical environments with honest evidence labels.

Candidate lanes:

- Windows 9x-class full-system emulation;
- classic Macintosh m68k;
- classic Macintosh PowerPC;
- Amiga-family m68k;
- physical systems where available.

A CPU-family pass is not promoted into an OS-family claim.

## R10 — Transport relay

Prototype an optional relay for transport functions that very old targets cannot reasonably implement locally.

The relay may provide modern TLS/HTTP/content-encoding transport.

It must not become a remote pixel renderer for profiles claiming local parsing/layout/rendering.

## R11 — Work elimination and CPU optimisation

R11 may draw from the measured mechanisms catalogued in [RUNTIME-PLAN-v1.md](RUNTIME-PLAN-v1.md) and [DONORS-v1.md](DONORS-v1.md), but none are promoted merely because they worked in a donor project.


Only after stable reference semantics and measurement:

- dirty-region painting;
- incremental layout and precise invalidation;
- bounded caches with explicit ownership;
- compact display/layout representations;
- safe result reuse;
- optional SIMD where it produces a measured benefit;
- optional bounded threading where it produces a measured benefit;
- host-aware CPU-path calibration only when the added machinery pays for itself.

There is no GPU/native-GPU-compositor roadmap rung. RIVET continues to produce software pixel surfaces; host presentation acceleration remains outside the RIVET contract.

Promotion requires conformance, measured benefit, and evidence that simpler work-elimination techniques were considered first.

## R12 — Long-lived compatibility policy

Define:

- stable ABI policy;
- command deprecation rules;
- format compatibility;
- target retirement rules;
- archived conformance vectors;
- release evidence bundle.

## Roadmap rule

A later phase must not be pulled forward merely because it is exciting.

If a simpler earlier implementation exposes a flaw in the architecture, fix the architecture before layering on more code.
