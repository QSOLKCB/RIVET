#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 SOURCE_REVISION OUT_DIR" >&2
  exit 2
fi

SOURCE_REVISION="$1"
OUT_DIR="$2"
CC_WIN="${CC_WIN:-i686-w64-mingw32-gcc}"
OBJDUMP_WIN="${OBJDUMP_WIN:-i686-w64-mingw32-objdump}"

[[ "$SOURCE_REVISION" =~ ^[0-9a-f]{40}$ ]] || {
  echo "source revision must be 40 lowercase hex characters" >&2
  exit 2
}

command -v "$CC_WIN" >/dev/null 2>&1 || {
  echo "missing compiler: $CC_WIN" >&2
  exit 2
}
command -v "$OBJDUMP_WIN" >/dev/null 2>&1 || {
  echo "missing PE inspector: $OBJDUMP_WIN" >&2
  exit 2
}

mkdir -p "$OUT_DIR"

"$CC_WIN"   -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror   -ffreestanding -fno-builtin -nostdlib   -Iinclude   -DRIVET_R9_TARGET_PROFILE=\"windows9x-x86\"   -DRIVET_R9_SOURCE_REVISION=\"$SOURCE_REVISION\"   core/rivet.c   gfx/raster.c   ui/ui.c   web/stream_url_utf8.c   web/html_css.c   web/layout_image.c   apps/browser/browser.c   evidence/r9_win9x_browser_proof.c   -Wl,--entry,_mainCRTStartup   -Wl,--subsystem,console:4.0   -lkernel32 -lgcc   -o "$OUT_DIR/RIVETR9.EXE"

file "$OUT_DIR/RIVETR9.EXE" | tee "$OUT_DIR/payload-file.txt"
grep -E "PE32 executable.*Intel 80386|PE32 executable.*i386"   "$OUT_DIR/payload-file.txt"

"$OBJDUMP_WIN" -p "$OUT_DIR/RIVETR9.EXE"   > "$OUT_DIR/payload-imports.txt"

grep -Fi "DLL Name: KERNEL32.dll"   "$OUT_DIR/payload-imports.txt"

if grep -Eiq   'DLL Name: (MSVCRT|UCRTBASE|api-ms-win-crt)'   "$OUT_DIR/payload-imports.txt"; then
  echo "Windows 9x payload unexpectedly imports a CRT" >&2
  cat "$OUT_DIR/payload-imports.txt" >&2
  exit 1
fi

sha256sum "$OUT_DIR/RIVETR9.EXE" > "$OUT_DIR/payload.sha256"
