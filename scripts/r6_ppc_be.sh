#!/usr/bin/env bash
set -euo pipefail

CC_PPC="${CC_PPC:-powerpc-linux-gnu-gcc}"
QEMU_PPC="${QEMU_PPC:-qemu-ppc}"
OUT_DIR="${OUT_DIR:-build/r6-ppc-be}"

mkdir -p "$OUT_DIR"

"$CC_PPC"   -static   -std=c99   -O2   -Wall   -Wextra   -Wpedantic   -Werror   -Iinclude   core/rivet.c   platform/platform.c   platform/posix/platform_posix.c   examples/r5_platform_proof.c   -o "$OUT_DIR/rivet-platform-ppc"

file "$OUT_DIR/rivet-platform-ppc" | tee "$OUT_DIR/file.txt"
grep -F "ELF 32-bit MSB" "$OUT_DIR/file.txt"
grep -F "PowerPC" "$OUT_DIR/file.txt"

"$QEMU_PPC" "$OUT_DIR/rivet-platform-ppc"   fixtures/r4_textview.txt   fixtures/r5_empty.txt   | tee "$OUT_DIR/qemu-proof.txt"

grep -F "backend=posix-v1" "$OUT_DIR/qemu-proof.txt"
grep -F "os=POSIX" "$OUT_DIR/qemu-proof.txt"
grep -F "pointer_bits=32" "$OUT_DIR/qemu-proof.txt"
grep -F "endian=big" "$OUT_DIR/qemu-proof.txt"
grep -F "bytes=237" "$OUT_DIR/qemu-proof.txt"
grep -F "empty=0" "$OUT_DIR/qemu-proof.txt"
grep -F "fnv1a64=36aaff7f4aaa99ab" "$OUT_DIR/qemu-proof.txt"
grep -F "monotonic=nondecreasing" "$OUT_DIR/qemu-proof.txt"
