# RIVET — Resilient Interface, View & Execution Toolkit

**Lean application infrastructure for software that should remain useful after the fashionable stack is gone.**

RIVET is a capability-driven runtime and interface toolkit for small native applications, document viewers, browser-like front ends, and long-lived software.

> **Software should scale downward as deliberately as it scales upward.**

RIVET separates **what an application means**, **what capabilities it requires**, **how those capabilities are presented**, and **which machine provides them**.

The eventual RIVET Browser is a proof application, not the definition of the framework.

## Status

**R0 — constitutional foundation.**

R0 defines the invariants, architecture, portability model, roadmap, donor boundaries, and machine-readable project contracts before implementation begins.

No browser engine, widget toolkit, scripting engine, accelerator, or platform backend is canonical yet.

## Mission

RIVET exists to make it practical to build software that is:

- small enough to inspect;
- explicit about the capabilities it needs;
- usable without Chromium, Electron, Node.js, React, or an OS webview;
- functional without a GPU;
- functional without a network when the application itself does not require one;
- portable across operating systems and CPU families without redefining application semantics;
- friendly to power users rather than hostile to them;
- measurable instead of merely marketed as "lightweight";
- capable of surviving older and constrained hardware.

## First principle: minimal sufficient implementation

If two implementations satisfy the same contract with equivalent correctness, safety, readability, portability, and evidence, **the materially smaller implementation wins**.

Smaller includes fewer lines where clarity is preserved, fewer dependencies, fewer layers and wrappers, fewer allocations, less hidden state, smaller binaries, smaller working sets, and fewer runtime requirements.

This is not code golf. A shorter implementation that is harder to verify, less portable, less safe, or less readable is not an improvement.

See [CONSTITUTION.md](CONSTITUTION.md).

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

See [PORTABILITY.md](PORTABILITY.md).

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

- [CONSTITUTION.md](CONSTITUTION.md) — non-negotiable project invariants.
- [ARCHITECTURE.md](ARCHITECTURE.md) — layer and authority model.
- [PORTABILITY.md](PORTABILITY.md) — MES, target identity, containers, emulation, and evidence.
- [ROADMAP.md](ROADMAP.md) — staged implementation plan.
- [DONORS.md](DONORS.md) — bounded lessons from existing QSOL projects.
- [AGENTS.md](AGENTS.md) — rules for coding agents and contributors.
- [machine/](machine/) — compact machine-readable project contracts.

## Licence

RIVET is licensed under the **Mozilla Public License 2.0**.
