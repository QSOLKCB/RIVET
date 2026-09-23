# RIVET Platform Contract v1

## Purpose

Platform v1 is the first shared RIVET platform ABI earned by two materially different implementations:

- modern POSIX;
- Win32.

It supplies OS machinery without redefining application semantics.

Machine-readable identity: machine/platform-v1.json.

## Claim boundary

Platform v1 exposes only two capabilities already named by the frozen capability contract:

    filesystem.read
    timer.monotonic

It does not claim a complete operating-system abstraction.

There is no Platform v1 API for windows, native event pumps, native keyboard/pointer translation, filesystem write, directory enumeration, clipboard, sockets/network, audio, threads, or GPU APIs.

Those capabilities must earn their own implementation surface later.

## ABI

Public header:

    include/rivet/platform.h

Identity:

    RIVET_PLATFORM_ABI_VERSION = 1
    language baseline          = C99

A rivet_platform provides immutable backend metadata, a bounded file-read operation, and a monotonic-nanoseconds operation.

Each linked host executable contains exactly one concrete rivet_platform_current() backend.

## Backend metadata

Each backend declares a stable backend ID, OS/API identity, process pointer width, observed byte order, and explicit RIVET capability set.

Platform validation requires metadata pointer width to equal the executing process's real sizeof(void*) * CHAR_BIT.

No capability is inferred from OS age, CPU name, or backend name.

## Path boundary

Platform v1 file paths are deliberately narrow.

A path must be a NUL-terminated C string, non-empty, at most 255 bytes before the terminator, and composed only of numeric 7-bit ASCII bytes 0x20..0x7e.

This is not RIVET's final text/path encoding policy.

    CreateFileA != final Windows UTF-8 filesystem design

The bounded ASCII path contract exists only to prove the R5 platform split without pulling later encoding/filesystem machinery forward.

## Bounded file read

rivet_platform_read_file() reads into caller-owned storage.

Rules:

- no heap allocation;
- caller declares buffer capacity;
- exact-size files may fill the complete buffer and still succeed;
- a file larger than capacity returns RIVET_ERR_CAPACITY;
- a missing path returns RIVET_ERR_NOT_FOUND;
- other backend/OS failures return RIVET_ERR_UNSUPPORTED;
- the public byte_count output is modified only on success.

The common wrapper validates the backend and path before invoking OS machinery.

## Monotonic time

rivet_platform_monotonic_ns() returns an unsigned 64-bit-or-wider nanosecond count through unsigned long long.

Rules:

- absolute epoch/origin is unspecified;
- only monotonic ordering is meaningful;
- the public output is modified only on success;
- conversion overflow fails explicitly with RIVET_ERR_CAPACITY.

No wall-clock/calendar API is introduced.

## POSIX backend

Implementation:

    platform/posix/platform_posix.c

Native mechanisms:

    filesystem.read  -> open / read / close
    timer.monotonic  -> clock_gettime(CLOCK_MONOTONIC)

Reads are chunked so no single POSIX read() request exceeds SSIZE_MAX.

## Win32 backend

Implementation:

    platform/win32/platform_win32.c

Native mechanisms:

    filesystem.read  -> CreateFileA / ReadFile / CloseHandle
    timer.monotonic  -> QueryPerformanceCounter / QueryPerformanceFrequency

The Win32 proof path remains within Platform v1's bounded numeric-ASCII path contract.

## Shared semantic proof

Both backends read the already-frozen R4 fixture:

    fixtures/r4_textview.txt
    bytes       = 237
    SHA-256     = 8d4a34353106071386727b776fa3089801563708dc521e0d363a447e12bb791e
    FNV-1a64    = 36aaff7f4aaa99ab

The native proof also calls the monotonic clock twice and requires the second observation to be greater than or equal to the first.

Correctness identity is the fixture bytes and operation semantics, not the timer's absolute value.

## R5 evidence matrix

R5 CI is required to execute native POSIX under GCC and Clang, native Win32 x64, a Win32 x86 process, and POSIX proofs in GCC 13 and GCC 14 Bookworm containers.

Native POSIX is native host execution.

Native Win32 x64 is native Windows OS/API execution.

The Win32 x86 lane is explicit 32-bit process execution. On a 64-bit Windows runner the process may execute through WOW64; it is not a claim of physical 32-bit hardware.

The GCC container lanes are E1 containerised toolchain evidence.

    CONTAINER PASS != TARGET-OS PASS
    WIN32 x86 PROCESS != PHYSICAL 32-BIT HARDWARE

## Reference proof

Program:

    examples/r5_platform_proof.c

Required semantic output includes:

    bytes=237
    fnv1a64=36aaff7f4aaa99ab
    monotonic=nondecreasing

Backend identity, OS/API, pointer width and byte order are printed separately and are expected to vary by target.

## Non-goals

Platform v1 does not introduce generic UTF-8/native path conversion, filesystem write, directory APIs, window creation, software-surface presentation to native windows, native input/event translation, clipboard, networking, audio, threading, SIMD, GPU APIs, dynamic backend loading, backend registries, or runtime backend selection.

The executable links exactly one backend. A dynamic backend framework has not earned its complexity.
