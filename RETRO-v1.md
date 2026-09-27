# RIVET Retro Portability Contract v1

## Purpose

R9 expands RIVET's portability evidence without changing the frozen R1-R8 framework or WEB1 semantics.

R9 separates CPU/ABI evidence from operating-system evidence. A successful cross-build or qemu-user execution may establish instruction-set, pointer-width and byte-order behaviour, but it does **not** establish Classic Mac OS, AmigaOS, Windows 9x, or physical-hardware support.

Machine-readable identity: `machine/retro-v1.json`.

## Frozen semantic surfaces

R9 reuses, without redefining:

- Core v1 through Browser WEB1 v1;
- Platform v1;
- Historical v1 evidence classes;
- the Platform v1 deterministic proof identity;
- the WEB1 deterministic browser proof identity.

No R9 target lane may modify a frozen R1-R8 ABI or silently weaken an earlier conformance test.

## Automated m68k E2 lane

The first R9 automated target profile is:

```text
linux-m68k32-68020-qemu-user
```

It is explicitly:

- Motorola 68020-baseline m68k code generation;
- 32-bit pointers;
- big-endian;
- Linux/POSIX user-space ABI;
- qemu-user execution;
- evidence class E2.

The lane cross-builds static m68k binaries and executes:

1. the frozen Platform v1 proof;
2. the frozen WEB1 browser unit/hostile-input test suite;
3. the frozen WEB1 deterministic raster proof.

Required Platform proof identity:

```text
backend=posix-v1
os=POSIX
pointer_bits=32
endian=big
bytes=237
empty=0
fnv1a64=36aaff7f4aaa99ab
monotonic=nondecreasing
```

Required WEB1 proof identity:

```text
document_fnv1a64=75be6cc92698ac1a
source_fnv1a64=5cf7c63a1fa3d9b4
history=2
bookmarks=1
fetches=4
downloads=1
```

Browser state size and proof-resident bytes remain environment observations rather than architecture-independent correctness identity.

## Claim boundary

A passing m68k E2 receipt permits the narrow claim:

```text
the frozen RIVET Platform v1 and WEB1 proof surfaces execute under
32-bit big-endian Linux/m68k qemu-user with the recorded toolchain
```

It does **not** permit any of these claims:

```text
Classic Mac OS support
AmigaOS support
Windows 9x support
full-system m68k target support
physical m68k hardware support
one universal m68k platform
```

Those claims require target-specific evidence.

## E3 target envelopes

R9 reserves separate full-system E3 receipt identities for:

- `classic-mac-m68k`;
- `classic-mac-powerpc`;
- `amiga-m68k`;
- `windows9x-x86`.

Each remains `unclaimed-receipt-required` until a harness actually boots the named OS/API environment and executes a bounded RIVET proof there.

Guest media or ROM material that is not redistributable must not be committed to the repository. A harness may require user-supplied or separately acquired media with an exact digest, following the same evidence-honesty rule used by R6.

## Receipt

Automated R9 receipts use:

```text
rivet.retro-receipt/v1
```

A receipt records:

- source revision;
- target profile;
- evidence class;
- execution mechanism;
- CPU baseline;
- compiler;
- emulator;
- pointer width and byte order;
- OS/API claim;
- Platform v1 proof;
- WEB1 proof;
- explicit claims and non-claims;
- result.

The source revision is the checked-out Git HEAD used for the build.

## Non-goals

R9 does not introduce:

- a new framework ABI;
- a generic emulator abstraction;
- new browser semantics;
- transport relay implementation;
- caches, incremental layout, SIMD or threading;
- GPU APIs;
- a claim that qemu-user is a historical operating system;
- redistribution of proprietary operating-system media or ROMs.

R10 remains the transport-relay phase.
