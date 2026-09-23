# RIVET — Resilient Interface, View & Execution Toolkit

**Lean application infrastructure for software that should remain useful after the fashionable stack is gone.**

RIVET is a capability-driven runtime and interface toolkit for small native applications, document viewers, browser-like front ends, and long-lived software.

> **Software should scale downward as deliberately as it scales upward.**

RIVET separates **what an application means**, **what capabilities it requires**, **how those capabilities are presented**, and **which machine provides them**.

The eventual RIVET Browser is a proof application, not the definition of the framework.

## Status

**R3 — input + lean UI.**

R2 is merged at `07ed7b1d37f7e09d40d8bd01c27788e205544dfe`. R3 adds portable logical keyboard events plus two command projections: exact key bindings and one lean menu.

The menu uses caller-owned items and a tiny built-in uppercase 5x7 bitmap alphabet. It renders through GFX v1 and requires no heap allocation.

## R3 quick proof

```sh
make test-ui
make ui
./build/rivet-ui-proof build/rivet-ui-proof.ppm
```

The deterministic proof must produce:

```text
FNV-1a64    8e022a6d842ff8e5
PPM SHA-256 c7feae9354c6934df1b199aa869845e0bae2cf46bb0cabce4e78930095fb21fd
open/save   1 / 1
menu final  hidden
```

The proof activates SAVE through the menu, removes the menu presentation, then still activates OPEN through `CTRL+O` using the same command registry.

See [UI-v1.md](UI-v1.md).
## Mission

RIVET exists to make it practical to build software that is:

- small enough to inspect;
- explicit about the capabilities it needs;
- usable without Chromium, Electron, Node.js, React, or an OS webview;
- rendered through CPU/software pixel surfaces without targeting a GPU API;
- functional without a network when the application itself does not require one;
- portable across operating systems and CPU families without redefining application semantics;
- friendly to power users rather than hostile to them;
- measurable instead of merely marketed as "lightweight";
- capable of surviving older and constrained hardware.

## Rendering doctrine

> **RIVET targets pixels, not GPUs.**

The same CPU/software rendering architecture applies on historical and modern systems. RIVET does not grow an OpenGL/Vulkan/Direct3D/Metal/WebGPU path merely because newer machines contain a GPU.

A host OS or compositor may internally accelerate presentation of the completed pixel surface. That is outside RIVET's rendering contract.

Performance work starts by doing less: tighter invalidation, bounded layout/paint, reuse, compact representations, and measured CPU-side optimisation.

See [RENDERING-v1.md](RENDERING-v1.md).

## First principle: minimal sufficient implementation

If two implementations satisfy the same contract with equivalent correctness, safety, readability, portability, and evidence, **the materially smaller implementation wins**.

Smaller includes fewer lines where clarity is preserved, fewer dependencies, fewer layers and wrappers, fewer allocations, less hidden state, smaller binaries, smaller working sets, and fewer runtime requirements.

This is not code golf. A shorter implementation that is harder to verify, less portable, less safe, or less readable is not an improvement.

See [CONSTITUTION-v2.md](CONSTITUTION-v2.md).

## Architectural thesis

```text
APPLICATION SEMANTICS
        |
        v
COMMANDS + CAPABILITY REQUIREMENTS
        |
        v
PORTABLE RIVET CORE
        |
   +----+----+----------------+
   |         |                |
  UI      DOCUMENTS        SERVICES
   |         |                |
   +----+----+----------------+
        |
   PLATFORM ADAPTER
        |
 OS/API + CPU + compiler + declared capabilities
```

Applications target capabilities, not fashionable stacks.

A target advertises what it actually provides. Unsupported capability is explicit. Silent fallback is not portability.

## Minimum Execution Substrate

RIVET adapts QSOL-ARK's Minimum Recoverable Substrate idea into a **Minimum Execution Substrate (MES)**.

An application declares required and optional capabilities. A target declares provided capabilities. The lowest-assumption target satisfying the required set is the application's MES.

Capabilities are never inherited merely because a target sounds newer or more powerful.

See [PORTABILITY-v2.md](PORTABILITY-v2.md).

## Browser as proof

The browser is downstream of RIVET.

A first useful browser profile should prove support for URL handling, HTTP, HTML, CSS, images, links/forms, downloads, history/bookmarks, view-source, user styles, and keyboard-first navigation.

JavaScript is not a requirement for the first browser profile.

## Target direction

The intended stress matrix includes modern POSIX, Win32, Windows 7–10, Windows 9x-class targets, x86/x86-64, PowerPC, Motorola 68k, classic Macintosh and Amiga-family experiments where practical.

Containers provide reproducible build/toolchain environments. Emulators and VMs provide target-execution evidence. Physical historical hardware may later provide stronger environment-specific evidence.

```text
CONTAINER PASS != TARGET-OS PASS
CROSS-COMPILE != EXECUTED TARGET
EMULATION != HISTORICAL HARDWARE
CPU COMPATIBILITY != OS COMPATIBILITY
```

## Project site

The GitHub Pages site is intentionally static HTML/CSS with no framework, analytics, package manager, or build step.

## Repository guide

- [UI-v1.md](UI-v1.md) — R3 logical keyboard, command projection, and lean-menu contract.
- [GFX-v1.md](GFX-v1.md) — frozen R2 software-surface ABI and deterministic raster contract.
- [CORE-v1.md](CORE-v1.md) — frozen R1 core/ABI contract.
- [CONSTITUTION-v2.md](CONSTITUTION-v2.md) — current non-negotiable project invariants.
- [CONSTITUTION.md](CONSTITUTION.md) — frozen v1 authority retained for compatibility.
- [ARCHITECTURE-v2.md](ARCHITECTURE-v2.md) — current layer, runtime, rendering, and authority model.
- [ARCHITECTURE.md](ARCHITECTURE.md) — frozen v1 project authority retained for compatibility.
- [PORTABILITY-v2.md](PORTABILITY-v2.md) — current MES, target identity, containers, emulation, and evidence.
- [PORTABILITY.md](PORTABILITY.md) — frozen v1 project authority retained for compatibility.
- [RENDERING-v1.md](RENDERING-v1.md) — CPU/software rendering contract and GPU exclusion boundary.
- [ROADMAP.md](ROADMAP.md) — staged implementation plan.
- [RUNTIME-PLAN-v1.md](RUNTIME-PLAN-v1.md) — donor-derived runtime and memory plan for R1+.
- [DONORS-v1.md](DONORS-v1.md) — frozen donor/provenance map for runtime-plan v1.
- [AGENTS.md](AGENTS.md) — rules for coding agents and contributors.
- [machine/project-v5.json](machine/project-v5.json) — current machine entrypoint for R3.
- [machine/project-v4.json](machine/project-v4.json) — frozen R2 project contract.
- [machine/project-v3.json](machine/project-v3.json) — frozen R1 project contract.
- [machine/project-v2.json](machine/project-v2.json) — frozen R0 project contract.
- [machine/](machine/) — versioned machine-readable project contracts.

## Licence

RIVET is licensed under the **Mozilla Public License 2.0**.
