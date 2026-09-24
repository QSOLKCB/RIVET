# RIVET Historical Stress Contract v1

## Purpose

R6 stress-tests already-frozen RIVET semantics under constrained and non-native execution.

R6 does not add a framework ABI. It reuses the frozen Core v1 and Platform v1 proof surfaces and records how those same semantics behave under:

- 32-bit x86;
- big-endian PowerPC under user-mode emulation;
- a full-system historical Windows target.

Machine-readable identity: machine/historical-v1.json.

## Evidence rule

R6 follows the evidence ladder in PORTABILITY-v2.md.

    compiled != executed
    container != target OS
    qemu-user != full-system guest
    full-system emulation != physical hardware

Each receipt records the exact checked-out Git HEAD that produced the binary, plus CPU, pointer width, byte order, compiler, emulator when applicable, execution class, and the unchanged Platform v1 proof identity.

The semantic proof remains:

    bytes=237
    empty=0
    fnv1a64=36aaff7f4aaa99ab
    monotonic=nondecreasing

## 32-bit x86 gate

Automated PR execution builds the frozen Platform v1 POSIX proof as a static 32-bit x86 ELF with an explicit GCC `-march=i686` baseline. Ubuntu's multilib runtime is i686-oriented, so R6 does not claim an i386 instruction floor.

The same binary is executed twice:

1. directly as a 32-bit process on the Ubuntu x86-64 runner;
2. through qemu-i686 user-mode CPU emulation.

Required proof identity:

    backend=posix-v1
    os=POSIX
    pointer_bits=32
    endian=little

The direct execution is 32-bit process evidence. It is not physical historical x86 hardware.

The qemu-i686 execution is E2 user-mode CPU-emulation evidence. It is not a full historical operating-system image.

## Big-endian gate

Automated PR execution cross-compiles the frozen Platform v1 POSIX proof for 32-bit PowerPC Linux and runs it through qemu-ppc.

Required proof identity:

    backend=posix-v1
    os=POSIX
    pointer_bits=32
    endian=big

Evidence class: E2 user-mode CPU emulation.

This lane exists to catch byte-order, pointer-width, C ABI, and platform assumptions. It does not claim classic Macintosh, AmigaOS, AIX, or another PowerPC/68k operating system.

    POWERPC BIG-ENDIAN EXECUTION != CLASSIC MAC OS
    POWERPC BIG-ENDIAN EXECUTION != AMIGAOS

## Historical Windows E3 target

R6 selects:

    Windows Server 2012 R2 Datacenter Evaluation x64

as the first full-system historical Windows gate.

The source media is Microsoft's Evaluation Center. RIVET does not redistribute Microsoft guest media.

The E3 workflow is:

    .github/workflows/r6-windows2012r2.yml

and is manual because obtaining the evaluation VHD/VHDX requires Microsoft registration and the download media must be supplied by an authorized user.

The workflow requires:

- repository secret RIVET_WIN2012R2_VHD_URL;
- caller-supplied SHA-256 for the downloaded evaluation media;
- official evaluation media retained outside the repository;
- Ubuntu's explicit rhsrvany Windows first-boot helper package.

Before boot evidence is accepted, libguestfs inspection must identify Windows 6.3 on x86_64 with product variant Server and exact product name Windows Server 2012 R2 Datacenter Evaluation (with only the optional Microsoft prefix accepted).

The harness:

1. verifies the supplied media SHA-256;
2. converts the VHD/VHDX into a working QCOW2 copy;
3. cross-builds the frozen Win32 Platform v1 proof;
4. injects proof files with libguestfs;
5. installs a Windows first-boot script;
6. boots the guest under qemu-system-x86_64;
7. runs the proof inside the guest;
8. shuts the guest down;
9. extracts and validates the receipt from the guest disk.

Required guest proof identity:

    backend=win32-v1
    os=Win32
    pointer_bits=64
    endian=little
    bytes=237
    empty=0
    fnv1a64=36aaff7f4aaa99ab
    monotonic=nondecreasing

Evidence class: E3 full-system emulation/VM.

The workflow additionally records the supplied guest-media SHA-256, QEMU identity, compiler identity, selected QEMU accelerator, selected CPU model, and libguestfs inspector output. TCG/Nehalem and KVM/host therefore produce distinguishable E3 receipts.

## Why Windows Server 2012 R2

The first E3 target is chosen for evidence quality and legal/reproducible media sourcing rather than nostalgia value.

Microsoft still provides Windows Server 2012 R2 evaluation ISO/VHD media through the Evaluation Center, but requires registration and limits the evaluation period.

R6 therefore keeps media acquisition outside the repository and makes the full-system gate explicit rather than silently downloading or redistributing a Windows image.

A Windows 9x-class target remains a later historical-expansion candidate once a lawful reproducible media path and suitable toolchain evidence are available.

## Receipt contract

scripts/r6_receipt.py validates the unchanged Platform v1 proof line and writes:

    rivet.historical-receipt/v1

Receipts separate semantic result identity from execution environment identity.

A receipt records:

- source revision;
- target profile;
- evidence class;
- execution mode;
- CPU identity;
- compiler;
- emulator when present;
- accelerator when applicable;
- CPU model when applicable;
- backend/OS identity;
- pointer width;
- byte order;
- proof bytes/FNV;
- result.

## CI and release-gate split

Every PR runs:

- receipt parser self-test;
- R5 freeze regression gate;
- 32-bit i686 direct-process execution;
- i686 qemu-user E2 execution;
- PowerPC big-endian qemu-user E2 execution;
- Win64 historical-guest payload cross-build;
- historical Windows harness syntax validation.

The Windows Server 2012 R2 full-system E3 workflow is manual/release-gated because the licensed evaluation media is not stored in the repository and because full-system execution is materially more expensive.

R6 is not considered fully evidenced until a passing E3 receipt has been retained for the source revision being released.

## Non-goals

R6 does not introduce:

- new application semantics;
- new platform capabilities;
- a new runtime abstraction;
- a generic emulator framework;
- a Windows image downloader;
- redistribution of proprietary guest media;
- claims of physical historical hardware;
- classic Mac support;
- Amiga support;
- Windows 9x support;
- GPU APIs.

Those target families remain separate evidence claims.
