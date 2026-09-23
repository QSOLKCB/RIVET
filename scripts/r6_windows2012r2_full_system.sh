#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 3 ]]; then
  echo "usage: $0 WINDOWS_VHD_OR_VHDX [OUT_DIR] [TIMEOUT_SECONDS]" >&2
  exit 2
fi

SOURCE_IMAGE="$1"
OUT_DIR="${2:-build/r6-windows2012r2}"
TIMEOUT_SECONDS="${3:-5400}"
GUEST="$OUT_DIR/windows2012r2.qcow2"
PAYLOAD="$OUT_DIR/rivet-platform-win32.exe"
FIRSTBOOT="$OUT_DIR/r6-firstboot.bat"
RECEIPT="$OUT_DIR/receipt.txt"
INSPECTOR="$OUT_DIR/inspector.xml"

for command in   qemu-img   qemu-system-x86_64   virt-customize   virt-inspector   virt-cat   x86_64-w64-mingw32-gcc   timeout
do
  command -v "$command" >/dev/null 2>&1 || {
    echo "missing required command: $command" >&2
    exit 2
  }
done

if ! find /usr/share -type f \( -iname rhsrvany.exe -o -iname pvvxsvc.exe \) -print -quit | grep -q .; then
  echo "virt-customize Windows firstboot helper not found (rhsrvany.exe or pvvxsvc.exe)" >&2
  exit 2
fi

mkdir -p "$OUT_DIR"

x86_64-w64-mingw32-gcc   -std=c99   -O2   -Wall   -Wextra   -Werror   -static   -Iinclude   core/rivet.c   platform/platform.c   platform/win32/platform_win32.c   examples/r5_platform_proof.c   -o "$PAYLOAD"

file "$PAYLOAD" | tee "$OUT_DIR/payload-file.txt"
grep -F "PE32+" "$OUT_DIR/payload-file.txt"

qemu-img convert -p -O qcow2 "$SOURCE_IMAGE" "$GUEST"

cat > "$FIRSTBOOT" <<'BAT'
@echo off
C:\rivet-r6\rivet-platform-win32.exe C:\rivet-r6\r4_textview.txt C:\rivet-r6\r5_empty.txt > C:\rivet-r6\receipt.txt 2>&1
echo exit_code=%ERRORLEVEL%>> C:\rivet-r6\receipt.txt
shutdown /s /t 5 /f
exit /b 250
BAT

export LIBGUESTFS_BACKEND=direct

virt-customize   -a "$GUEST"   --mkdir /rivet-r6   --upload "$PAYLOAD:/rivet-r6/rivet-platform-win32.exe"   --upload fixtures/r4_textview.txt:/rivet-r6/r4_textview.txt   --upload fixtures/r5_empty.txt:/rivet-r6/r5_empty.txt   --firstboot "$FIRSTBOOT"

virt-inspector -a "$GUEST" > "$INSPECTOR"
grep -qi "<name>windows</name>" "$INSPECTOR"
grep -q "<major_version>6</major_version>" "$INSPECTOR"
grep -q "<minor_version>3</minor_version>" "$INSPECTOR"

ACCEL="tcg"
CPU="Nehalem"
if [[ -r /dev/kvm && -w /dev/kvm ]]; then
  ACCEL="kvm:tcg"
  CPU="host"
fi

set +e
timeout --signal=TERM --kill-after=30 "$TIMEOUT_SECONDS"   qemu-system-x86_64     -machine "pc,accel=$ACCEL"     -cpu "$CPU"     -m 2048     -smp 2     -drive "file=$GUEST,format=qcow2,if=ide"     -boot c     -nic none     -display none     -serial "file:$OUT_DIR/serial.log"     -no-reboot
QEMU_STATUS=$?
set -e

if [[ $QEMU_STATUS -ne 0 ]]; then
  echo "historical Windows guest did not shut down successfully (status $QEMU_STATUS)" >&2
  exit "$QEMU_STATUS"
fi

virt-cat -a "$GUEST" /rivet-r6/receipt.txt > "$RECEIPT"

cat "$RECEIPT"
grep -F "backend=win32-v1" "$RECEIPT"
grep -F "os=Win32" "$RECEIPT"
grep -F "pointer_bits=64" "$RECEIPT"
grep -F "endian=little" "$RECEIPT"
grep -F "bytes=237" "$RECEIPT"
grep -F "empty=0" "$RECEIPT"
grep -F "fnv1a64=36aaff7f4aaa99ab" "$RECEIPT"
grep -F "monotonic=nondecreasing" "$RECEIPT"
grep -F "exit_code=0" "$RECEIPT"
