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

**Complete in PR #4.**

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

**Complete in PR #5.**

Implemented R3 slice:

- [x] portable logical key events;
- [x] exact key+modifier command bindings;
- [x] one caller-owned menu projection;
- [x] UP/DOWN/ENTER/ESCAPE menu handling;
- [x] deterministic menu rendering through GFX v1;
- [x] tiny uppercase 5x7 built-in glyph set;
- [x] no-heap UI path;
- [x] executable proof that menu removal does not remove command capability;
- [x] deterministic pixel FNV + PPM SHA-256 vector;
- [x] GCC/Clang and ASan/UBSan coverage;
- [x] regression gate preserving Core v1 and GFX v1.

The roadmap's optional command palette is deliberately deferred. Key bindings and menu items already project the same stable commands without duplicating command semantics; text-search state has not yet earned its machinery.

Success criterion: capability remains addressable even when a presentation element is removed.

See [UI-v1.md](UI-v1.md).
## R4 — Non-browser proof application

**Complete in PR #6.**

Implemented R4 proof application:

- [x] bounded open from caller-owned document bytes;
- [x] caller-owned line-index storage with explicit capacity failure;
- [x] numeric-ASCII application byte contract independent of execution character set;
- [x] line and page navigation;
- [x] exact find-next with one wrap and preserved previous match on miss;
- [x] keyboard command projection through frozen UI v1;
- [x] deterministic read-only rendering through frozen GFX v1;
- [x] caller-configurable viewer colors;
- [x] bounded headless file reader for proof only;
- [x] no-heap application state;
- [x] deterministic fixture, pixel FNV and PPM SHA-256 evidence;
- [x] GCC/Clang, ASan/UBSan and IBM1047 coverage;
- [x] regression gate preserving Core v1, GFX v1 and UI v1.

The viewer is intentionally read-only, so document save/export is not applicable to this proof. The headless stdio reader is not promoted into a RIVET filesystem abstraction before R5.

This proves RIVET can host a useful application that is not a browser while keeping application semantics outside the framework contracts.

See [TEXTVIEW-v1.md](TEXTVIEW-v1.md).
## R5 — Platform split

**Current phase.**

Implemented R5 platform slice:

- [x] shared Platform v1 ABI earned by two concrete backends;
- [x] explicit `filesystem.read` and `timer.monotonic` capabilities;
- [x] bounded caller-owned file reads with no heap;
- [x] bounded numeric-ASCII proof path contract;
- [x] POSIX backend using open/read/close + CLOCK_MONOTONIC;
- [x] Win32 backend using CreateFileA/ReadFile/CloseHandle + QueryPerformanceCounter;
- [x] native POSIX execution under GCC and Clang;
- [x] native Win32 x64 execution;
- [x] Win32 x86 32-bit process execution with pointer-width verification;
- [x] GCC 13/14 Bookworm container toolchain evidence;
- [x] identical frozen fixture identity across platform proofs;
- [x] explicit evidence labels separating containers, OS execution and 32-bit process evidence;
- [x] regression gate preserving R1–R4 contracts.

The x86 Windows lane proves a 32-bit process on Windows; on a 64-bit runner it may use WOW64 and is not claimed as physical 32-bit hardware.

Platform v1 intentionally does not add windows, native input, filesystem write, networking, audio, threads, or GPU machinery.

See [PLATFORM-v1.md](PLATFORM-v1.md).
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
