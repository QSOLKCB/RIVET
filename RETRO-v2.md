# RIVET Retro Portability Contract v2

## Purpose

R9 v2 completes the target-specific harness layer for the historical operating-system families named by the roadmap while preserving the evidence boundary established by Retro v1.

Retro v2 does **not** rewrite Retro v1. It supersedes it for the current R9 phase and keeps all merged R1-R8 semantics frozen.

Machine-readable identity: `machine/retro-v2.json`.

## Frozen source boundary

R1 through the merged R9 v1 slice are frozen at:

```text
7d260e0671c5d089b25d6075ab1b66fb0886c99e
```

R9 v2 adds only:

- source-bound historical guest payloads;
- target-specific full-system emulator harnesses;
- media/ROM digest validation;
- E3 receipt validation;
- E4 physical-hardware receipt validation;
- CI that proves payloads still build from current source;
- documentation and evidence retention rules.

No frozen framework ABI or WEB1 semantic is redefined.

## Common guest proof

Every E3 payload runs the same bounded WEB1 proof locally inside the guest OS.

The payload embeds:

- target profile;
- exact Git source revision;
- pointer width;
- byte order.

It must reproduce:

```text
document_fnv1a64=75be6cc92698ac1a
source_fnv1a64=5cf7c63a1fa3d9b4
history=2
bookmarks=1
fetches=4
downloads=1
```

The guest writes a plain-text proof line. The host refuses to mint an E3 JSON receipt if any field differs.

This proves local parsing, layout, browser state transitions and software rendering. It is explicitly not remote pixel rendering.

## Windows 9x E3 envelope

Target profile:

```text
windows9x-x86
```

Payload:

- 32-bit PE/Win32;
- built from the current source revision with the i686 MinGW toolchain;
- freestanding Windows 95-floor entrypoint using Kernel32 file/process APIs only;
- no MSVCRT/UCRT/API-set CRT import is permitted;
- frozen WEB1 proof linked directly into the guest executable.

Harness:

- user-supplied bootable Windows 95/98-family disk image;
- exact SHA-256 required;
- guest image is copied before mutation;
- payload and startup batch are injected into the guest FAT filesystem;
- QEMU full-system x86 executes the guest;
- the guest writes the source-bound proof and Windows `VER` output;
- the guest must record exactly one `proof_exit=0` marker; a missing, duplicate or nonzero proof-exit marker is not evidence;
- the receipt retains media digest, payload digest and emulator identity.

A passing receipt proves only the exact pinned media/environment named in that receipt.

## Classic Macintosh E3 envelopes

Target profiles:

```text
classic-mac-m68k
classic-mac-powerpc
```

Payloads are built with the Retro68 cross-toolchain from current RIVET sources.

The m68k payload is a Classic Mac APPL built for 68k. The PowerPC payload is a Classic Mac PEF/APPL.

Harness:

- user-supplied bootable HFS disk image;
- exact SHA-256 required;
- m68k additionally requires a user-supplied ROM with exact SHA-256;
- payload is copied as MacBinary into `System Folder:Startup Items`;
- QEMU full-system q800 or mac99 executes the guest;
- the guest application writes the source-bound WEB1 proof into the Startup Items directory;
- the host extracts and validates that proof before minting E3 evidence.

No Mac OS media or ROM is redistributed by RIVET.

## Amiga E3 envelope

Target profile:

```text
amiga-m68k
```

Payload:

- m68k AmigaOS executable;
- built from current RIVET sources with the maintained Amiga GCC toolchain;
- 68020 baseline;
- frozen WEB1 proof linked locally.

Harness:

- user-supplied bootable Amiga HDF;
- user-supplied Kickstart ROM;
- exact SHA-256 required for both;
- payload is injected into the selected boot partition;
- the boot startup sequence invokes the proof;
- FS-UAE performs full-system execution;
- the guest-written proof is extracted from the HDF and validated.

No Workbench/AmigaOS media or Kickstart ROM is redistributed by RIVET.

## E3 receipt

Successful target-specific runs produce:

```text
rivet.retro-e3-receipt/v1
```

The receipt binds:

- exact source revision;
- exact target profile;
- E3 evidence class;
- emulator identity;
- guest media label and SHA-256;
- optional/required ROM SHA-256;
- payload SHA-256;
- exact frozen WEB1 proof;
- result.

A harness file existing in the repository is **not** an E3 pass. Only a retained passing receipt from guest execution is E3 evidence.

## Physical hardware E4

Physical target evidence uses:

```text
rivet.retro-e4-receipt/v1
```

The receipt must include:

- exact non-null Git source revision;
- one of the named historical target profiles;
- `execution=physical-hardware`;
- manufacturer and model;
- CPU identity compatible with the target architecture;
- installed memory;
- exact software environment:
  - OS name;
  - OS version compatible with the selected target family;
  - API identity;
- exact frozen WEB1 proof;
- one or more SHA-256-bound evidence attachments with non-placeholder digests;
- result `pass`.

The validator rejects receipts with a Git null source OID, missing hardware identity, a CPU outside the target architecture family, mixed host/emulator CPU descriptions, missing or target-incompatible software identity, an OS version outside the target's historical version envelope, mismatched proof identity, unhashed attachments, or the all-zero SHA-256 placeholder digest. The **entire** CPU identity must match one physical target-compatible CPU description; a compatible token embedded inside incompatible host or emulator prose is not sufficient. Canonical Motorola forms such as `MC68040` and `Motorola MC68040` are valid m68k identities. Windows x86 CPU identity must be established by explicit x86-family model tokens rather than vendor names alone; e.g. Intel Itanium is not x86 evidence. For example, a `classic-mac-m68k` physical receipt must identify an m68k CPU, Classic Mac OS in the supported 6.x/7.x/8.0–8.1 envelope, and the Mac OS Toolbox API; another operating system or CPU family is different evidence and requires its own target profile.

Physical evidence is intentionally retained by explicit PR/artifact rather than fabricated automatically.

## Media and redistribution rule

RIVET does not commit or redistribute proprietary Windows, Macintosh, AmigaOS/Workbench or ROM media.

Manual workflows obtain media only from repository secrets controlled by the user/operator and require the caller to provide the expected digest separately.

A successful download without a matching digest is a failure.

Full-system harnesses may continue to proof extraction after GNU `timeout` status 124 or kill-after status 137. Those statuses are not passes by themselves: only a fresh, target/source-bound guest proof can mint evidence.

## Completion rule

R9 implementation machinery is complete when:

- m68k E2 lane is retained;
- Windows 9x E3 harness is source-bound and validated;
- Classic Mac m68k E3 harness is source-bound and validated;
- Classic Mac PowerPC E3 harness is source-bound and validated;
- Amiga m68k E3 harness is source-bound and validated;
- physical E4 receipt validation exists;
- merged R1-R9-v1 source is frozen by CI.

Platform-support claims remain evidence-dependent. A target stays unclaimed until its passing E3 or E4 receipt is actually retained.

## Non-goals

R9 v2 does not add:

- transport relay implementation;
- browser feature expansion;
- JavaScript;
- cookies;
- tabs;
- GPU APIs;
- caches;
- incremental layout;
- SIMD;
- threading;
- a generic emulator framework;
- bundled proprietary guest media.

R10 remains the transport-relay phase after R9 implementation machinery is merged.
