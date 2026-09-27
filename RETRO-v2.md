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

Every E3 receipt also requires that source revision to resolve to an actual commit in the RIVET checkout **and** resolve every target-specific guest payload source/build entrypoint to its exact frozen Git blob identity. A historical commit, empty replacement, or altered payload source cannot mint E3 evidence merely because the expected path exists.

It must reproduce:

```text
document_fnv1a64=75be6cc92698ac1a
source_fnv1a64=5cf7c63a1fa3d9b4
history=2
bookmarks=1
fetches=4
downloads=1
```

The guest writes exactly one plain-text proof line. The host reads at most 65536 bytes from the guest-controlled proof file and rejects an oversized proof before decoding or parsing it. The host refuses to mint an E3 JSON receipt if the proof file contains zero or multiple lines beginning with `rivet-r9-guest:` — including malformed or contradictory duplicates — if the source revision is the Git null OID, or if any proof field differs.

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
- the operator supplies the DOS drive letter for the selected Windows partition (`C:`, `D:`, etc.);
- payload and startup batch are injected into the selected guest filesystem;
- `WINSTART.BAT` calls the proof on the supplied DOS drive and `RUN-R9.BAT` switches to that drive before execution;
- the proof writes `RECEIPT.TXT` relative to its `\\RIVET-R9` working directory rather than assuming `C:`;
- QEMU full-system x86 executes the guest;
- the guest writes the source-bound proof and Windows `VER` output;
- the proof must contain exactly one Windows `[Version ...]` identity occurrence, and it must identify a published consumer Windows 95, Windows 98 or Windows Me build; multiple identities are invalid even when concatenated onto one physical line; arbitrary suffixes such as `4.10.FAKE` are invalid;
- the guest must record exactly one `proof_exit=0` marker; a missing, duplicate or nonzero proof-exit marker is not evidence;
- guest-media, payload and optional/required ROM SHA-256 values must be real nonzero digests; the all-zero placeholder is invalid;
- the receipt retains media digest, payload digest and a target-compatible full-system emulator identity; the identity must name the actual harness executable family (`qemu-system-i386`, `qemu-system-m68k`, `qemu-system-ppc`, or `fs-uae`) rather than an unrelated emulator or user-mode program;
- the requested receipt output path is cleared before validation and published atomically only after every check passes, so a failed rerun cannot leave stale passing evidence.

A passing receipt proves only the exact pinned media/environment named in that receipt.

## Classic Macintosh E3 envelopes

Target profiles:

```text
classic-mac-m68k
classic-mac-powerpc
```

Payloads are built with the Retro68 cross-toolchain from current RIVET sources.

The m68k payload is a Classic Mac APPL built for 68k. The PowerPC payload is a Classic Mac PEF/APPL.

Both Classic Mac payloads query the running guest with the real Classic Mac `Gestalt(gestaltSystemVersion, ...)` API and append exactly one `rivet-r9-os` identity line. E3 validation requires that runtime version to match the selected Classic Mac target envelope; a CPU-compatible proof without this Toolbox-derived OS witness cannot mint E3 evidence. The m68k harness is specifically a QEMU q800/68040 environment and accepts only the published q800-compatible System 7.1/7.1.1, 7.5/7.5.1/7.5.2/7.5.3/7.5.5, 7.6/7.6.1, 8.0 and 8.1 release identities; System 6 and invented numeric versions are invalid q800 E3 evidence. The PowerPC harness is specifically QEMU mac99 and accepts only Mac OS 8.6 and the published 9.0/9.0.2/9.0.3/9.0.4/9.1/9.2/9.2.1/9.2.2 release identities. Earlier PowerPC-capable releases do not establish execution on the fixed mac99 machine.

Harness:

- user-supplied bootable HFS disk image;
- exact SHA-256 required;
- m68k additionally requires a user-supplied ROM with exact SHA-256;
- payload is copied as MacBinary into `System Folder:Startup Items` using explicit HFS operands (for example `:RIVETR9`);
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
- frozen WEB1 proof linked locally;
- runtime OS witness reads the live ExecBase version and opens `dos.library` in the guest, retaining Exec/DOS versions in a `rivet-r9-os` identity line; the fixed FS-UAE A1200 harness accepts only Exec/DOS major versions 39 through 45, excluding pre-A1200 runtimes.

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

- exact source revision that resolves to a RIVET commit;
- exact target profile;
- E3 evidence class;
- emulator identity;
- guest media label and SHA-256;
- optional/required ROM SHA-256;
- payload SHA-256;
- exact frozen WEB1 proof;
- target-specific runtime guest OS identity;
- result.

A harness file existing in the repository is **not** an E3 pass. Only a retained passing receipt from guest execution is E3 evidence. Windows requires its validated `VER` identity, Classic Mac requires a Gestalt-derived system version, and Amiga requires live Exec/DOS library versions. CPU-family execution alone is never promoted to an OS-family E3 claim.

## Physical hardware E4

Physical target evidence uses:

```text
rivet.retro-e4-receipt/v1
```

The receipt must include:

- exact non-null Git source revision that resolves to a commit in the RIVET repository and contains the exact frozen Git blob identities for the WEB1 proof entrypoint, browser implementation, and public browser header;
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
- one or more retained evidence attachments whose relative paths resolve to regular files inside the receipt directory tree and whose actual bytes match the declared SHA-256 digests;
- result `pass`.

The validator reads at most 65536 bytes before UTF-8 decoding/JSON parsing, requires the JSON root to be an object, rejects non-JSON numeric constants such as NaN and infinities, rejects control characters in identity/evidence-name strings before whitespace normalization, and rejects receipts with duplicate JSON object keys at any nesting level, a Git null source OID, a source revision that does not resolve to a RIVET commit, shipped placeholder attachment or hardware identity values such as `REPLACE-WITH-*`, missing hardware identity, a CPU outside the target architecture family, mixed host/emulator CPU descriptions, missing or target-incompatible software identity, an OS version outside the target's historical version envelope, mismatched proof identity, unhashed attachments, or the all-zero SHA-256 placeholder digest. The **entire** CPU identity must match one physical target-compatible CPU description; a compatible token embedded inside incompatible host or emulator prose is not sufficient. Canonical Motorola forms such as `MC68040`, `Motorola MC68040`, the Classic Mac `Motorola 68LC040`, and the Amiga 1200 stock `Motorola 68EC020` are valid m68k identities. Canonical PowerPC identities such as `PowerPC 603e`, `PowerPC 604e`, `PowerPC 7400`, and the `G4` alias are valid PowerPC identities. Classic Mac physical evidence is restricted to explicitly published releases rather than arbitrary numeric version strings. The m68k set is System 6.0/6.0.1/6.0.2/6.0.3/6.0.4/6.0.5/6.0.7/6.0.8; 7.0/7.0.1/7.1/7.1.1; 7.5/7.5.1/7.5.2/7.5.3/7.5.5; 7.6/7.6.1; and 8.0/8.1. The PowerPC set is 7.1.2; 7.5/7.5.1/7.5.2/7.5.3/7.5.5; 7.6/7.6.1; 8.0/8.1/8.5/8.5.1/8.6; and 9.0/9.0.2/9.0.3/9.0.4/9.1/9.2/9.2.1/9.2.2. Impossible values such as 6.99.99 or 9.99.99 are invalid. AMD x86 identities such as `AMD Am486DX4` are valid Windows 9x CPU identities. Windows x86 CPU identity must be established by explicit x86-family model tokens rather than vendor names alone; e.g. Intel Itanium is not x86 evidence. Physical receipts enforce both CPU and RAM floors for the selected software environment. Classic Mac m68k requires at least a 68030 for 7.6.x and a 68040 for 8.0–8.1; Classic Mac PowerPC 9.2.x requires G3/750-class or newer. Windows 95/98/Me retain their 386/486/Pentium-class CPU floors and 4/16/32 MiB RAM floors. Classic Mac m68k uses 1 MiB for System 6, 2 MiB for System 7.0–7.1.1, 4 MiB for System 7.5.x, 8 MiB for 7.6.x and 12 MiB for 8.0–8.1; Classic Mac PowerPC uses 8 MiB for 7.x, 12 MiB for 8.0–8.1, 24 MiB for 8.5–8.6 and 32 MiB for 9.x. Amiga m68k accepts only explicit published 1.x–3.x releases; AmigaOS 3.1.4 and 3.2.x require at least 2 MiB RAM, AmigaOS 3.5/3.9 require at least a 68020 and 4 MiB RAM, and earlier accepted releases retain the 512 KiB baseline. Physical-memory values are also bounded above by the target envelope: 2 GiB for Windows 9x and Classic Mac PowerPC, 256 MiB for Classic Mac m68k, and 512 MiB for Amiga m68k. For example, a `classic-mac-m68k` physical receipt must identify an m68k CPU meeting that release's generation floor, one of the explicit Classic Mac OS m68k releases, enough RAM for that release, and the Mac OS Toolbox API; another operating system or CPU family is different evidence and requires its own target profile.

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
- merged R1-R9-v1 source is frozen by CI using the complete baseline-to-HEAD changed-path set; newly added frozen-surface paths are rejected unless explicitly allowlisted as R9-v2 evidence machinery.

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


## V2 authority freeze

After this R9 v2 authority set is published, the authoritative immutability check runs from the trusted base branch via `pull_request_target`, never from the candidate checkout. That base-branch guard pins the exact Git blob identity of each authority file, owns the fixed R9-v2 changed-path allowlist, and verifies that its own workflow file is byte-identical in the candidate tree. The candidate PR cannot update the lock together with the authority it protects.

- `RETRO-v2.md`;
- `machine/retro-v2.json`;
- `machine/project-v12.json`.

The older R9-v1 baseline continues to protect the frozen implementation surface. The trusted guard is stored at `.github/workflows/r9-trust-anchor.yml` on `main`; its baseline-to-HEAD allowlist scan applies to every non-maintenance pull request, including pull requests opened after R9 v2 merges. PR-local workflows perform candidate tests only and are not treated as an authority boundary.
