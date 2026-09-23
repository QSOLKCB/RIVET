#!/usr/bin/env bash
set -euo pipefail

CC_I386="${CC_I386:-gcc}"
QEMU_I386="${QEMU_I386:-qemu-i386}"
OUT_DIR="${OUT_DIR:-build/r6-i386}"

mkdir -p "$OUT_DIR"

"$CC_I386"   -m32   -static   -std=c99   -O2   -Wall   -Wextra   -Wpedantic   -Werror   -Iinclude   core/rivet.c   platform/platform.c   platform/posix/platform_posix.c   examples/r5_platform_proof.c   -o "$OUT_DIR/rivet-platform-i386"

file "$OUT_DIR/rivet-platform-i386" | tee "$OUT_DIR/file.txt"
grep -F "ELF 32-bit" "$OUT_DIR/file.txt"
grep -F "Intel 80386" "$OUT_DIR/file.txt"

"$OUT_DIR/rivet-platform-i386"   fixtures/r4_textview.txt   fixtures/r5_empty.txt   | tee "$OUT_DIR/native-proof.txt"

"$QEMU_I386" "$OUT_DIR/rivet-platform-i386"   fixtures/r4_textview.txt   fixtures/r5_empty.txt   | tee "$OUT_DIR/qemu-proof.txt"

for proof in "$OUT_DIR/native-proof.txt" "$OUT_DIR/qemu-proof.txt"; do
  grep -F "backend=posix-v1" "$proof"
  grep -F "os=POSIX" "$proof"
  grep -F "pointer_bits=32" "$proof"
  grep -F "endian=little" "$proof"
  grep -F "bytes=237" "$proof"
  grep -F "empty=0" "$proof"
  grep -F "fnv1a64=36aaff7f4aaa99ab" "$proof"
  grep -F "monotonic=nondecreasing" "$proof"
done
