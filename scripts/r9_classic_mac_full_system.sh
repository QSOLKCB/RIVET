#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 5 || $# -gt 7 ]]; then
  echo "usage: $0 m68k|powerpc BOOT_IMAGE PAYLOAD SOURCE_REVISION TIMEOUT_SECONDS [ROM_IMAGE|-] [OUT_DIR]" >&2
  exit 2
fi

ARCH="$1"
SOURCE_IMAGE="$2"
PAYLOAD="$3"
SOURCE_REVISION="$4"
TIMEOUT_SECONDS="$5"
ROM_IMAGE="${6:--}"
OUT_DIR="${7:-build/r9-classic-mac-$ARCH}"
GUEST="$OUT_DIR/classic-mac.img"
PROOF="$OUT_DIR/guest-proof.txt"

case "$ARCH" in
  m68k)
    QEMU=qemu-system-m68k
    TARGET=classic-mac-m68k
    MACHINE=q800
    MEMORY=64
    ;;
  powerpc)
    QEMU=qemu-system-ppc
    TARGET=classic-mac-powerpc
    MACHINE=mac99
    MEMORY=256
    ;;
  *) exit 2 ;;
esac

for command in "$QEMU" hmount hcopy hcd hdel hls humount timeout; do
  command -v "$command" >/dev/null 2>&1 || {
    echo "missing required command: $command" >&2
    exit 2
  }
done

[[ "$SOURCE_REVISION" =~ ^[0-9a-f]{40}$ ]] || exit 2
mkdir -p "$OUT_DIR"
cp "$SOURCE_IMAGE" "$GUEST"

export HOME="$OUT_DIR/hfs-home"
mkdir -p "$HOME"
rm -f "$PROOF"

hmount "$GUEST"
hcd ":System Folder:Startup Items"
if hls "RIVET-R9-RECEIPT.TXT" >/dev/null 2>&1; then
  hdel "RIVET-R9-RECEIPT.TXT"
fi
hcopy -m "$PAYLOAD" "RIVETR9"
humount

QEMU_ARGS=(
  -M "$MACHINE"
  -m "$MEMORY"
  -drive "file=$GUEST,format=raw,media=disk"
  -boot c
  -nic none
  -display none
  -serial "file:$OUT_DIR/serial.log"
  -no-reboot
)
if [[ "$ARCH" == m68k ]]; then
  [[ "$ROM_IMAGE" != "-" && -f "$ROM_IMAGE" ]] || {
    echo "m68k Classic Mac execution requires a user-supplied ROM image" >&2
    exit 2
  }
  QEMU_ARGS+=( -bios "$ROM_IMAGE" )
elif [[ "$ROM_IMAGE" != "-" ]]; then
  QEMU_ARGS+=( -bios "$ROM_IMAGE" )
fi

set +e
timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"   "$QEMU" "${QEMU_ARGS[@]}"
QEMU_STATUS=$?
set -e
if [[ $QEMU_STATUS -ne 0 && $QEMU_STATUS -ne 124 && $QEMU_STATUS -ne 137 ]]; then
  exit "$QEMU_STATUS"
fi

hmount "$GUEST"
hcd ":System Folder:Startup Items"
hcopy -t "RIVET-R9-RECEIPT.TXT" "$PROOF"
humount

cat "$PROOF"
grep -F "rivet-r9-guest: target=$TARGET source=$SOURCE_REVISION" "$PROOF"
