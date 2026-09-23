# RIVET Portability and Minimum Execution Substrate v2

## Minimum Execution Substrate (MES)

For an application with required capability set `R`, the MES is the lowest-assumption implemented target profile whose declared capabilities contain `R`.

```text
required(app) subset_of provided(target)
    -> executable

otherwise
    -> explicit unavailable result
```

Optional capabilities may improve the application without becoming requirements.

Capability profiles are not an inheritance hierarchy.

## Initial target direction

| Family | Purpose |
|---|---|
| modern POSIX/x86-64 | development/reference environment |
| Win32/x86-64 | contemporary native Windows reference |
| Win32/x86 | 32-bit portability and lower floor |
| Windows 7–10 | practical older-PC longevity target |
| Windows 9x-class | historical/resource-constrained target |
| m68k | big-endian/reduced-resource/compiler stress |
| PowerPC | big-endian/alternate desktop stress |
| classic Macintosh | m68k/PPC OS target where practical |
| Amiga-family | m68k historical GUI/system target where practical |

This is direction, not a claim of implemented support.

## Evidence ladder

RIVET distinguishes test environments instead of flattening them into "portable".

### E0 — Static/build evidence

Examples: compilation, warnings, source/ABI checks, format validation.

Useful, but not target execution.

### E1 — Containerised toolchain evidence

Containers are the default reproducible build and dependency environment where supported.

Good uses:

- pinned compiler/toolchain versions;
- alternate libc/toolchain matrices;
- cross-compilers;
- clean-environment tests;
- size and artifact checks;
- parser/unit/conformance suites;
- packaging the emulator harness itself.

Containers normally share the host kernel and CPU family. Therefore:

```text
CONTAINER PASS != TARGET-OS PASS
CONTAINER CPU != EMULATED CPU
```

### E2 — User-mode CPU emulation

Useful for executing compatible user-space binaries under a different CPU model where the OS ABI remains provided by the host/emulation layer.

This can test endian/word-size/instruction assumptions without claiming full historical-machine equivalence.

### E3 — Full-system emulation or VM

Use when OS, firmware, drivers, graphics APIs, or period runtime behaviour matter.

This is the preferred automated class for Windows 9x, classic Macintosh, Amiga-family, and similar historical targets.

The exact emulator is test infrastructure, not project authority.

### E4 — Physical historical hardware

Strongest environment-specific evidence when available, but not required for every PR.

Physical execution should preserve exact hardware/software identity in its receipt.

## Retro test strategy

RIVET should use a layered matrix:

```text
source
  |
  +-- native host tests
  |
  +-- containerised compilers/libcs
  |
  +-- cross-compile
  |
  +-- user-mode CPU emulation where meaningful
  |
  +-- full-system emulator / VM
  |
  +-- optional physical hardware
```

The lower-cost layers run often. Historical full-system tests may run on scheduled or release gates if runtime cost is high.

## Containers + emulators together

A useful pattern is:

```text
container
  owns:
    compiler
    build dependencies
    test harness
    emulator version/config
        |
        v
full-system emulator / VM
  owns:
    guest CPU model
    guest OS
    guest APIs
        |
        v
RIVET test artifact
```

This makes the *harness* reproducible without pretending the container itself represents the historical target.

## Windows 9x direction

DOS emulators are useful for DOS and real-mode/protected-mode experiments.

Windows 9x behaviour should eventually be exercised under an environment that actually runs the Windows 9x OS/API contract rather than treating DOS compatibility as equivalent to Win32 compatibility.

A real-mode DOS path on modern x86 hardware could be an interesting separate stress experiment if safely isolated, but it is not required for R0 and would be recorded as its own target identity.

## m68k / PowerPC direction

A classic-Mac emulator can provide excellent CPU/endian/compiler and classic-Mac-OS evidence for m68k or PowerPC targets.

It does **not** prove AmigaOS compatibility merely because both systems use 68k hardware.

Likewise an Amiga emulator proves the declared Amiga-family environment, not generic m68k correctness.

```text
M68K EXECUTION != AMIGAOS EXECUTION
M68K EXECUTION != CLASSIC-MAC-OS EXECUTION
PPC EXECUTION != ONE UNIVERSAL PPC PLATFORM
```

## Portable formats

All canonical cross-target binary formats must declare byte order and field widths. Tests should include synthetic opposite-endian fixtures before physical big-endian targets are available.

## Claim rule

Every portability receipt should identify, where applicable:

- RIVET source revision;
- target profile;
- CPU architecture/model;
- OS/API;
- emulator/VM/container identity;
- compiler and linker;
- pointer width;
- byte order;
- capability set;
- binary size;
- test set;
- result;
- whether execution was native, containerised, emulated, virtualised, or physical.

The purpose is not bureaucracy. It is preventing "works on my machine" from becoming "supports this platform".
