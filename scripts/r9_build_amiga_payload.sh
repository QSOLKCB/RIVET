#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 SOURCE_REVISION OUT_DIR" >&2
  exit 2
fi

SOURCE_REVISION="$1"
OUT_DIR="$2"
AMIGA_IMAGE="${AMIGA_GCC_IMAGE:-amigadev/m68k-amigaos-gcc:latest}"

[[ "$SOURCE_REVISION" =~ ^[0-9a-f]{40}$ ]] || exit 2
mkdir -p "$OUT_DIR"

docker run --rm   -v "$PWD:/work"   -w /work   "$AMIGA_IMAGE"   m68k-amigaos-gcc     -noixemul -m68020 -std=c99 -O2 -Wall -Wextra     -Iinclude     -DRIVET_R9_TARGET_PROFILE=\"amiga-m68k\"     -DRIVET_R9_SOURCE_REVISION=\"$SOURCE_REVISION\"     core/rivet.c     gfx/raster.c     ui/ui.c     web/stream_url_utf8.c     web/html_css.c     web/layout_image.c     apps/browser/browser.c     evidence/r9_guest_browser_proof.c     evidence/r9_amiga_os_identity.c     -o "$OUT_DIR/RIVETR9"

test -s "$OUT_DIR/RIVETR9"
file "$OUT_DIR/RIVETR9" | tee "$OUT_DIR/payload-file.txt"
sha256sum "$OUT_DIR/RIVETR9" > "$OUT_DIR/payload.sha256"
docker image inspect "$AMIGA_IMAGE"   --format '{{json .RepoDigests}}' > "$OUT_DIR/toolchain-image.json"
