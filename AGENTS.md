# RIVET Agent and Contributor Contract

Read [CONSTITUTION.md](CONSTITUTION.md) before changing architecture.

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
- mandatory GPU;
- mandatory network;
- mandatory threads;
- cloud service;
- database server.

This does not forbid optional adapters or language bindings. It forbids silently making them the foundation.

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

Keep a reference path. Measure before claiming benefit. Record the environment.

## Documentation

Human prose explains why.

Machine contracts define compact normative identities where automation needs them.

Do not duplicate large rule sets in many files; link to the authority.

## R0 boundary

During R0, do not add framework/runtime implementation code.

The static Pages site is documentation, not runtime implementation.
