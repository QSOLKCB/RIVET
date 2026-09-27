#!/usr/bin/env bash
set -euo pipefail

CC_M68K="${CC_M68K:-m68k-linux-gnu-gcc}"
QEMU_M68K="${QEMU_M68K:-qemu-m68k}"
OUT_DIR="${OUT_DIR:-build/r9-m68k}"

mkdir -p "$OUT_DIR"

COMMON_FLAGS=(
  -static
  -std=c99
  -O2
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -m68020
)

"$CC_M68K" "${COMMON_FLAGS[@]}" -Iinclude \
  core/rivet.c \
  platform/platform.c \
  platform/posix/platform_posix.c \
  examples/r5_platform_proof.c \
  -o "$OUT_DIR/rivet-platform-m68k"

"$CC_M68K" "${COMMON_FLAGS[@]}" -Iinclude \
  core/rivet.c \
  gfx/raster.c \
  ui/ui.c \
  web/stream_url_utf8.c \
  web/html_css.c \
  web/layout_image.c \
  apps/browser/browser.c \
  tests/test_browser.c \
  -o "$OUT_DIR/test-browser-m68k"

"$CC_M68K" "${COMMON_FLAGS[@]}" -Iinclude -Iplatform/headless \
  core/rivet.c \
  gfx/raster.c \
  ui/ui.c \
  web/stream_url_utf8.c \
  web/html_css.c \
  web/layout_image.c \
  apps/browser/browser.c \
  platform/headless/ppm.c \
  examples/r8_browser_proof.c \
  -o "$OUT_DIR/rivet-web1-proof-m68k"

file \
  "$OUT_DIR/rivet-platform-m68k" \
  "$OUT_DIR/test-browser-m68k" \
  "$OUT_DIR/rivet-web1-proof-m68k" \
  | tee "$OUT_DIR/file.txt"

grep -F "ELF 32-bit MSB" "$OUT_DIR/file.txt"
grep -F "Motorola m68k" "$OUT_DIR/file.txt"

"$QEMU_M68K" "$OUT_DIR/rivet-platform-m68k" \
  fixtures/r4_textview.txt \
  fixtures/r5_empty.txt \
  | tee "$OUT_DIR/platform-proof.txt"

grep -F "backend=posix-v1" "$OUT_DIR/platform-proof.txt"
grep -F "os=POSIX" "$OUT_DIR/platform-proof.txt"
grep -F "pointer_bits=32" "$OUT_DIR/platform-proof.txt"
grep -F "endian=big" "$OUT_DIR/platform-proof.txt"
grep -F "bytes=237" "$OUT_DIR/platform-proof.txt"
grep -F "empty=0" "$OUT_DIR/platform-proof.txt"
grep -F "fnv1a64=36aaff7f4aaa99ab" "$OUT_DIR/platform-proof.txt"
grep -F "monotonic=nondecreasing" "$OUT_DIR/platform-proof.txt"

"$QEMU_M68K" "$OUT_DIR/test-browser-m68k" \
  | tee "$OUT_DIR/browser-tests.txt"
grep -F "rivet browser tests: ok" "$OUT_DIR/browser-tests.txt"

"$QEMU_M68K" "$OUT_DIR/rivet-web1-proof-m68k" \
  "$OUT_DIR/rivet-web1-proof.ppm" \
  | tee "$OUT_DIR/browser-proof.txt"

grep -F "document_fnv1a64=75be6cc92698ac1a" "$OUT_DIR/browser-proof.txt"
grep -F "source_fnv1a64=5cf7c63a1fa3d9b4" "$OUT_DIR/browser-proof.txt"
grep -F "history=2" "$OUT_DIR/browser-proof.txt"
grep -F "bookmarks=1" "$OUT_DIR/browser-proof.txt"
grep -F "fetches=4" "$OUT_DIR/browser-proof.txt"
grep -F "downloads=1" "$OUT_DIR/browser-proof.txt"

sha256sum \
  "$OUT_DIR/rivet-platform-m68k" \
  "$OUT_DIR/test-browser-m68k" \
  "$OUT_DIR/rivet-web1-proof-m68k" \
  "$OUT_DIR/rivet-web1-proof.ppm" \
  > "$OUT_DIR/SHA256SUMS"
