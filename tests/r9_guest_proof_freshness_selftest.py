#!/usr/bin/env python3
from pathlib import Path

WINDOWS = Path("scripts/r9_windows9x_full_system.sh")
MAC = Path("scripts/r9_classic_mac_full_system.sh")
AMIGA = Path("scripts/r9_amiga_full_system.sh")

def require_before(text: str, first: str, second: str, label: str):
    first_at = text.find(first)
    second_at = text.find(second)
    if first_at < 0:
        raise SystemExit(f"{label}: missing {first!r}")
    if second_at < 0:
        raise SystemExit(f"{label}: missing {second!r}")
    if first_at >= second_at:
        raise SystemExit(
            f"{label}: {first!r} must occur before {second!r}"
        )

def main() -> int:
    windows = WINDOWS.read_text(encoding="utf-8")
    require_before(
        windows,
        "rm-f /RIVET-R9/RECEIPT.TXT",
        'timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"',
        "Windows freshness",
    )
    require_before(
        windows,
        "CALL C:\\\\RIVET-R9\\\\RUN-R9.BAT",
        'cat "$WINSTART_ORIGINAL" >> "$WINSTART"',
        "Windows startup ordering",
    )
    if '>> "$WINSTART"' in windows and (
        "CALL C:\\\\RIVET-R9\\\\RUN-R9.BAT" in windows[
            windows.find('cat "$WINSTART_ORIGINAL" >> "$WINSTART"') :
        ]
    ):
        raise SystemExit(
            "Windows startup proof call must not be appended after "
            "the original WINSTART contents"
        )

    mac = MAC.read_text(encoding="utf-8")
    require_before(
        mac,
        'hdel "RIVET-R9-RECEIPT.TXT"',
        'timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"',
        "Classic Mac freshness",
    )

    amiga = AMIGA.read_text(encoding="utf-8")
    require_before(
        amiga,
        "delete RIVET-R9-RECEIPT.TXT",
        'timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"',
        "Amiga freshness",
    )

    print("r9 guest proof freshness self-test: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
