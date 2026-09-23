# RIVET Constitution

RIVET is allowed to evolve. These invariants exist to stop evolution from quietly turning it into the class of software it was created to avoid.

The current machine-readable counterpart is `machine/invariants-v2.json`. The merged R0 v1 record remains preserved at `machine/invariants.json` and is not rewritten in place.

## RIVET-INV-001 — Minimal sufficient implementation

For two implementations that satisfy the same required behaviour with equivalent correctness, safety, readability, portability, and evidence, the materially smaller implementation wins.

Reduction targets include code size, dependency count, abstraction layers, state, allocations, runtime requirements, binary size, and working set.

This is **not code golf**. A shorter implementation that obscures correctness, weakens security, duplicates semantics, or harms portability is not preferred.

**Every abstraction must pay rent.**

## RIVET-INV-002 — Meaning is independent of machinery

Application semantics may not depend on a particular renderer, widget set, thread model, CPU family, browser engine, or operating system unless the application explicitly declares that dependency as a capability requirement.

GPU rendering is not an optional RIVET capability. It is excluded separately by RIVET-INV-019.

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

Software rasterisation is the normative and canonical graphics architecture.

RIVET does not implement or target GPU rendering APIs. A host operating system or window system may internally accelerate final presentation of a completed RIVET pixel surface, but that mechanism is below the RIVET contract and may not redefine pixels or application semantics.

Optional CPU-side optimisations such as SIMD or bounded multithreading require conformance against declared reference semantics.

Optimisation may change CPU machinery. It may not redefine meaning.

## RIVET-INV-006 — No mandatory heavyweight runtime

The RIVET core must not require Chromium, Gecko, WebKit, Electron, an OS WebView, Node.js, React, a network connection, a package manager, a cloud service, or any GPU API.

Applications may explicitly depend on optional capabilities. The core may not.

## RIVET-INV-007 — Portable C baseline and stable ABI

The portable implementation baseline is C99 with a deliberately conservative subset where broader compiler support justifies it.

Public runtime boundaries should prefer a stable C ABI.

Language bindings are welcome. No higher-level language runtime becomes semantic authority merely because a binding is convenient.

## RIVET-INV-008 — Single-threaded execution is valid

The baseline event/runtime model must remain usable without mandatory threading.

Threads, worker pools, and SIMD are optional CPU-side optimisations or explicit capabilities. GPU execution is outside the RIVET architecture.

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

## RIVET-INV-019 — RIVET targets pixels, not GPUs

RIVET's rendering contract ends at a CPU-produced software pixel surface.

RIVET does not target OpenGL, Vulkan, Direct3D, Metal, WebGL, WebGPU, CUDA, GPU compute, shader languages, or equivalent GPU APIs as required **or optional** framework rendering machinery.

A host OS, window server, driver, emulator, or compositor may internally use a GPU when presenting the completed surface. That is outside RIVET's authority and does not create a GPU dependency.

A specialised application may use external GPU code outside the RIVET rendering contract, but such a path is not a RIVET capability and must not become required by the core, UI, document engine, or RIVET Browser.

```text
RIVET TARGETS PIXELS != RIVET TARGETS GPU
GPU PRESENT          != GPU REQUIRED
HOST COMPOSITING     != RIVET RENDERING
```

## RIVET-INV-020 — Reduce work before accelerating it

Before adding parallelism, SIMD, caching, specialised CPU paths, or other acceleration, first determine whether the work can be eliminated, bounded, deferred, reused, invalidated more precisely, or represented more compactly.

Examples include dirty-region painting instead of full repaint, incremental layout instead of complete relayout, bounded image decode, glyph reuse, compact display data, and avoiding work when state has not changed.

The preferred optimisation order is:

```text
DO LESS
  -> STORE LESS
  -> MOVE LESS
  -> REUSE SAFE RESULTS
  -> MEASURE
  -> THEN ACCELERATE THE REMAINDER IF JUSTIFIED
```

Hardware power is not permission to waste work.

## Constitutional change rule

Changing an invariant requires a dedicated pull request that names the invariant, explains the concrete failure in the existing rule, states the replacement, updates the machine-readable contract, and identifies compatibility consequences.

A feature PR must not quietly rewrite the constitution as a side effect.
