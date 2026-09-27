#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 m68k|powerpc SOURCE_REVISION OUT_DIR" >&2
  exit 2
fi

ARCH="$1"
SOURCE_REVISION="$2"
OUT_DIR="$3"
RETRO68_IMAGE="${RETRO68_IMAGE:-ghcr.io/autc04/retro68:latest}"

case "$ARCH" in
  m68k) TARGET_PROFILE=classic-mac-m68k ;;
  powerpc) TARGET_PROFILE=classic-mac-powerpc ;;
  *) echo "unsupported Classic Mac architecture: $ARCH" >&2; exit 2 ;;
esac

[[ "$SOURCE_REVISION" =~ ^[0-9a-f]{40}$ ]] || exit 2
mkdir -p "$OUT_DIR"

docker run --rm   -v "$PWD:/work"   -w /work   "$RETRO68_IMAGE"   /bin/bash -lc "
set -euo pipefail
TOOL=/Retro68-build/toolchain
mkdir -p '$OUT_DIR'
if [[ '$ARCH' == m68k ]]; then
  CC=\$TOOL/bin/m68k-apple-macos-gcc
  REZ=\$TOOL/bin/Rez
  RINC=\$TOOL/m68k-apple-macos/RIncludes
  \$CC -std=c99 -O2 -Wall -Wextra -Iinclude \
    -DRIVET_R9_TARGET_PROFILE=\\\"$TARGET_PROFILE\\\" \
    -DRIVET_R9_SOURCE_REVISION=\\\"$SOURCE_REVISION\\\" \
    core/rivet.c gfx/raster.c ui/ui.c \
    web/stream_url_utf8.c web/html_css.c web/layout_image.c \
    apps/browser/browser.c evidence/r9_guest_browser_proof.c \
    -Wl,--mac-single -o '$OUT_DIR/RIVETR9.code.bin'
  \$REZ -I\$RINC \
    --copy '$OUT_DIR/RIVETR9.code.bin' \
    \$RINC/Retro68APPL.r \
    -t APPL -c RVT9 \
    -o '$OUT_DIR/RIVETR9.bin' \
    --cc '$OUT_DIR/RIVETR9.APPL' \
    --cc '$OUT_DIR/RIVETR9.dsk'
else
  CC=\$TOOL/bin/powerpc-apple-macos-gcc
  MAKEPEF=\$TOOL/bin/MakePEF
  REZ=\$TOOL/bin/Rez
  RINC=\$TOOL/powerpc-apple-macos/RIncludes
  \$CC -std=c99 -O2 -Wall -Wextra -Iinclude \
    -DRIVET_R9_TARGET_PROFILE=\\\"$TARGET_PROFILE\\\" \
    -DRIVET_R9_SOURCE_REVISION=\\\"$SOURCE_REVISION\\\" \
    core/rivet.c gfx/raster.c ui/ui.c \
    web/stream_url_utf8.c web/html_css.c web/layout_image.c \
    apps/browser/browser.c evidence/r9_guest_browser_proof.c \
    -o '$OUT_DIR/RIVETR9.xcoff'
  \$MAKEPEF -o '$OUT_DIR/RIVETR9.pef' '$OUT_DIR/RIVETR9.xcoff'
  \$REZ \$RINC/RetroPPCAPPL.r -I\$RINC \
    -DCFRAG_NAME=\\\"RIVETR9\\\" \
    -o '$OUT_DIR/RIVETR9.bin'     -t APPL -c RVT9     --data '$OUT_DIR/RIVETR9.pef'
fi
"

test -s "$OUT_DIR/RIVETR9.bin"
sha256sum "$OUT_DIR/RIVETR9.bin" > "$OUT_DIR/payload.sha256"
docker image inspect "$RETRO68_IMAGE"   --format '{{json .RepoDigests}}' > "$OUT_DIR/toolchain-image.json"
