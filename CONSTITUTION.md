# RIVET Constitution

RIVET is allowed to evolve. These invariants exist to stop evolution from quietly turning it into the class of software it was created to avoid.

Machine-readable counterparts live in `machine/invariants.json`.

## RIVET-INV-001 — Minimal sufficient implementation

For two implementations that satisfy the same required behaviour with equivalent correctness, safety, readability, portability, and evidence, the materially smaller implementation wins.

Reduction targets include code size, dependency count, abstraction layers, state, allocations, runtime requirements, binary size, and working set.

This is **not code golf**. A shorter implementation that obscures correctness, weakens security, duplicates semantics, or harms portability is not preferred.

**Every abstraction must pay rent.**

## RIVET-INV-002 — Meaning is independent of machinery

Application semantics may not depend on a particular renderer, widget set, GPU, thread model, CPU family, browser engine, or operating system unless the application explicitly declares that dependency as a capability requirement.

```text
APPLICATION MEANING != EXECUTION MACHINERY
```

## RIVET-INV-003 — Capability is not presentation

A capability is a stable application contract. Menus, buttons, shortcuts, command palettes, CLI bindings, and other surfaces are projections of that capability.

A UI redesign may move or hide a command from its default presentation. It must not silently delete or redefine the underlying capability.

```text
BUTTON != CAPABILITY
MENU ITEM != CAPABILITY
```

## RIVET-INV-004 — Capabilities are explicit

Targets declare capabilities individually. Capabilities are never inherited implicitly from target age, OS name, CPU class, or profile rank.

Unsupported means unsupported. Unknown means unknown.

No requested backend may silently fall back to a different semantic path.

## RIVET-INV-005 — The reference path stays boring

RIVET keeps a simple, inspectable reference implementation wherever practical.

Software rasterisation is the normative graphics baseline. SIMD, GPU, native compositor, multithreaded, or other accelerated paths are optional until they pass conformance against declared reference semantics.

Optimisation may change machinery. It may not redefine meaning.

## RIVET-INV-006 — No mandatory heavyweight runtime

The RIVET core must not require Chromium, Gecko, WebKit, Electron, an OS WebView, Node.js, React, a GPU, a network connection, a package manager, or a cloud service.

Applications may explicitly depend on optional capabilities. The core may not.

## RIVET-INV-007 — Portable C baseline and stable ABI

The portable implementation baseline is C99 with a deliberately conservative subset where broader compiler support justifies it.

Public runtime boundaries should prefer a stable C ABI.

Language bindings are welcome. No higher-level language runtime becomes semantic authority merely because a binding is convenient.

## RIVET-INV-008 — Single-threaded execution is valid

The baseline event/runtime model must remain usable without mandatory threading.

Threads, worker pools, SIMD and GPU work are optimisations or explicit capabilities.

## RIVET-INV-009 — Local software does not worship the network

No telemetry by default. No account requirement by default. No network dependency for local verification, configuration, or ordinary offline operation where avoidable.

## RIVET-INV-010 — The user owns state and capability

Important configuration and persistent state should be inspectable, portable, versioned, and documented.

Power-user controls are not removed merely to simplify a default interface.

## RIVET-INV-011 — Failure is explicit

Unsupported capability, unavailable backend, malformed input, exhausted resource budget, timeout, and compatibility failure must be distinguishable.

A permitted fallback must be explicit in the resulting runtime identity.

```text
FALLBACK != REQUESTED BACKEND
TIMEOUT != SUCCESS
UNAVAILABLE != FALSE
```

## RIVET-INV-012 — Persistent formats are architecture-neutral

Persistent or wire formats must not serialize native C structs as protocol.

Formats require explicit field widths, byte order, version, bounds, and validation.

Big-endian and little-endian hosts must interpret the same canonical bytes consistently.

## RIVET-INV-013 — Resource use is evidence

"Fast", "small", "lightweight", and "low memory" are not sufficient claims.

Footprint/performance claims should bind exact source revision, target identity, toolchain, workload, binary size, memory where available, and timing where material.

## RIVET-INV-014 — Portability claims require execution evidence

A successful cross-compile is useful evidence, but not automatically evidence that the program executed correctly on the claimed target.

Containers, user-mode emulation, full-system emulation, VMs, and physical hardware are distinct evidence classes.

## RIVET-INV-015 — Browser code is downstream

The RIVET Browser is a demonstration application.

Browser requirements do not automatically become RIVET-core requirements.

## RIVET-INV-016 — Dependencies must justify themselves

Dependencies are allowed when they materially improve correctness, security, portability, standards compliance, or maintenance.

"Zero dependencies" is not a religion. "Add a library" is not a reflex.

A dependency must solve a problem better than a small auditable implementation and fit the declared target floor.

## RIVET-INV-017 — Donors provide mechanisms, not authority

Existing QSOL projects and external software may donate ideas, tests, algorithms, or bounded code where licensing permits.

Provenance, licence, semantic boundary, and conformance remain explicit.

## RIVET-INV-018 — Compatibility is a feature

Do not remove working capabilities merely because they are unfashionable.

Deprecation requires a documented reason, compatibility consequence, and migration path where practical.

## Constitutional change rule

Changing an invariant requires a dedicated pull request that names the invariant, explains the concrete failure in the existing rule, states the replacement, updates the machine-readable contract, and identifies compatibility consequences.

A feature PR must not quietly rewrite the constitution as a side effect.
