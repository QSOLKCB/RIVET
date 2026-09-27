#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 6 || $# -gt 7 ]]; then
  echo "usage: $0 BOOT_HDF PARTITION PAYLOAD KICKSTART_ROM SOURCE_REVISION TIMEOUT_SECONDS [OUT_DIR]" >&2
  exit 2
fi

SOURCE_IMAGE="$1"
PARTITION="$2"
PAYLOAD="$3"
KICKSTART_ROM="$4"
SOURCE_REVISION="$5"
TIMEOUT_SECONDS="$6"
OUT_DIR="${7:-build/r9-amiga}"
GUEST="$OUT_DIR/amiga.hdf"
STARTUP="$OUT_DIR/Startup-Sequence"
PROOF="$OUT_DIR/guest-proof.txt"

for command in xdftool fs-uae timeout; do
  command -v "$command" >/dev/null 2>&1 || {
    echo "missing required command: $command" >&2
    exit 2
  }
done

[[ "$SOURCE_REVISION" =~ ^[0-9a-f]{40}$ ]] || exit 2
mkdir -p "$OUT_DIR"
cp "$SOURCE_IMAGE" "$GUEST"

cat > "$STARTUP" <<'AMIGA'
Stack 131072
SYS:RIVETR9 SYS:RIVET-R9-RECEIPT.TXT
AMIGA

rm -f "$PROOF"

if xdftool "$GUEST" open "part=$PARTITION" + \
    list RIVET-R9-RECEIPT.TXT >/dev/null 2>&1; then
  xdftool "$GUEST" open "part=$PARTITION" + \
    delete RIVET-R9-RECEIPT.TXT
fi

xdftool "$GUEST" open "part=$PARTITION" + \
  write "$PAYLOAD" RIVETR9 + \
  write "$STARTUP" S/Startup-Sequence

set +e
timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"   fs-uae     --amiga_model=A1200     --kickstart_file="$KICKSTART_ROM"     --hard_drive_0="$GUEST"     --sound_output=none     --fullscreen=0
FSUAE_STATUS=$?
set -e
if [[ $FSUAE_STATUS -ne 0 && $FSUAE_STATUS -ne 124 ]]; then
  exit "$FSUAE_STATUS"
fi

xdftool "$GUEST" open "part=$PARTITION" +   read RIVET-R9-RECEIPT.TXT "$PROOF"

cat "$PROOF"
grep -F "rivet-r9-guest: target=amiga-m68k source=$SOURCE_REVISION" "$PROOF"
