#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 7 || $# -gt 8 ]]; then
  echo "usage: $0 GUEST_IMAGE GUEST_PARTITION DOS_DRIVE WINDOWS_DIRECTORY PAYLOAD SOURCE_REVISION TIMEOUT_SECONDS [OUT_DIR]" >&2
  exit 2
fi

SOURCE_IMAGE="$1"
GUEST_PARTITION="$2"
DOS_DRIVE="$3"
WINDOWS_DIRECTORY="$4"
PAYLOAD="$5"
SOURCE_REVISION="$6"
TIMEOUT_SECONDS="$7"
OUT_DIR="${8:-build/r9-windows9x}"
GUEST="$OUT_DIR/windows9x.qcow2"
RUN_BAT="$OUT_DIR/RUN-R9.BAT"
WINSTART="$OUT_DIR/WINSTART.BAT"
WINSTART_ORIGINAL="$OUT_DIR/WINSTART.ORIGINAL.BAT"
PROOF="$OUT_DIR/guest-proof.txt"

for command in qemu-img qemu-system-i386 guestfish timeout; do
  command -v "$command" >/dev/null 2>&1 || {
    echo "missing required command: $command" >&2
    exit 2
  }
done

[[ "$SOURCE_REVISION" =~ ^[0-9a-f]{40}$ ]] || exit 2
[[ "$TIMEOUT_SECONDS" =~ ^[0-9]+$ ]] || exit 2
[[ "$DOS_DRIVE" =~ ^[A-Za-z]:$ ]] || {
  echo "DOS drive must look like C: or D:" >&2
  exit 2
}
[[ -f "$SOURCE_IMAGE" && -f "$PAYLOAD" ]] || exit 2

DOS_DRIVE="${DOS_DRIVE^^}"

mkdir -p "$OUT_DIR"
qemu-img convert -p -O qcow2 "$SOURCE_IMAGE" "$GUEST"

{
  printf '@ECHO OFF\r\n'
  printf '%s\r\n' "$DOS_DRIVE"
  printf 'CD \\RIVET-R9\r\n'
  printf 'RIVETR9.EXE\r\n'
  printf 'IF ERRORLEVEL 1 GOTO RIVET_FAIL\r\n'
  printf 'ECHO proof_exit=0>>RECEIPT.TXT\r\n'
  printf 'GOTO RIVET_STATUS_DONE\r\n'
  printf ':RIVET_FAIL\r\n'
  printf 'ECHO proof_exit=nonzero>>RECEIPT.TXT\r\n'
  printf ':RIVET_STATUS_DONE\r\n'
  printf 'ECHO startup_stage=winstart>>RECEIPT.TXT\r\n'
  printf 'VER>>RECEIPT.TXT\r\n'
} > "$RUN_BAT"

if guestfish --ro -a "$GUEST" -m "$GUEST_PARTITION" \
    exists "$WINDOWS_DIRECTORY/WINSTART.BAT" | grep -q true; then
  guestfish --ro -a "$GUEST" -m "$GUEST_PARTITION" \
    download "$WINDOWS_DIRECTORY/WINSTART.BAT" "$WINSTART_ORIGINAL"
else
  : > "$WINSTART_ORIGINAL"
fi

printf '@ECHO OFF\r\nCALL C:\\RIVET-R9\\RUN-R9.BAT\r\n' > "$WINSTART"
cat "$WINSTART_ORIGINAL" >> "$WINSTART"

rm -f "$PROOF"

guestfish --rw -a "$GUEST" -m "$GUEST_PARTITION" <<EOF
mkdir-p /RIVET-R9
rm-f /RIVET-R9/RECEIPT.TXT
upload $PAYLOAD /RIVET-R9/RIVETR9.EXE
upload $RUN_BAT /RIVET-R9/RUN-R9.BAT
upload $WINSTART $WINDOWS_DIRECTORY/WINSTART.BAT
EOF

set +e
timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"   qemu-system-i386     -machine pc,accel=tcg     -cpu pentium2     -m 128     -drive "file=$GUEST,format=qcow2,if=ide"     -boot c     -nic none     -display none     -serial "file:$OUT_DIR/serial.log"     -no-reboot
QEMU_STATUS=$?
set -e

if [[ $QEMU_STATUS -ne 0 && $QEMU_STATUS -ne 124 && $QEMU_STATUS -ne 137 ]]; then
  echo "Windows 9x QEMU exited unexpectedly: $QEMU_STATUS" >&2
  exit "$QEMU_STATUS"
fi

guestfish --ro -a "$GUEST" -m "$GUEST_PARTITION"   download /RIVET-R9/RECEIPT.TXT "$PROOF"

cat "$PROOF"
grep -F "rivet-r9-guest: target=windows9x-x86 source=$SOURCE_REVISION" "$PROOF"
grep -F "proof_exit=0" "$PROOF"
