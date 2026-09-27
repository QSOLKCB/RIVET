#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

PLATFORM_RE = re.compile(
    r"^rivet-r5: "
    r"backend=(?P<backend>\S+) "
    r"os=(?P<os_api>\S+) "
    r"pointer_bits=(?P<pointer_bits>[0-9]+) "
    r"endian=(?P<endian>little|big) "
    r"bytes=(?P<bytes>[0-9]+) "
    r"empty=(?P<empty>[0-9]+) "
    r"fnv1a64=(?P<fnv1a64>[0-9a-f]{16}) "
    r"monotonic=(?P<monotonic>nondecreasing)$"
)

BROWSER_RE = re.compile(
    r"^rivet-r8: "
    r"document_fnv1a64=(?P<document_fnv1a64>[0-9a-f]{16}) "
    r"source_fnv1a64=(?P<source_fnv1a64>[0-9a-f]{16}) "
    r"history=(?P<history>[0-9]+) "
    r"bookmarks=(?P<bookmarks>[0-9]+) "
    r"fetches=(?P<fetches>[0-9]+) "
    r"downloads=(?P<downloads>[0-9]+) "
    r"browser_state_bytes=(?P<browser_state_bytes>[0-9]+) "
    r"proof_resident_bytes=(?P<proof_resident_bytes>[0-9]+) "
    r"ppm=(?P<ppm>\S+)$"
)

NON_CLAIMS = [
    "classic-mac-os",
    "amigaos",
    "windows-9x",
    "full-system-m68k",
    "physical-m68k-hardware",
]

def parse_line(path: str, pattern: re.Pattern[str], label: str) -> dict:
    text = Path(path).read_text(encoding="utf-8")
    for line in text.splitlines():
        match = pattern.match(line.strip())
        if match:
            return match.groupdict()
    raise SystemExit(f"R9 receipt: {label} proof line not found")

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform-proof", required=True)
    parser.add_argument("--browser-proof", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--emulator", required=True)
    args = parser.parse_args()

    platform = parse_line(
        args.platform_proof,
        PLATFORM_RE,
        "platform",
    )
    browser = parse_line(
        args.browser_proof,
        BROWSER_RE,
        "browser",
    )

    platform["pointer_bits"] = int(platform["pointer_bits"])
    platform["bytes"] = int(platform["bytes"])
    platform["empty"] = int(platform["empty"])

    for key in (
        "history",
        "bookmarks",
        "fetches",
        "downloads",
        "browser_state_bytes",
        "proof_resident_bytes",
    ):
        browser[key] = int(browser[key])

    platform_expected = {
        "backend": "posix-v1",
        "os_api": "POSIX",
        "pointer_bits": 32,
        "endian": "big",
        "bytes": 237,
        "empty": 0,
        "fnv1a64": "36aaff7f4aaa99ab",
        "monotonic": "nondecreasing",
    }
    browser_expected = {
        "document_fnv1a64": "75be6cc92698ac1a",
        "source_fnv1a64": "5cf7c63a1fa3d9b4",
        "history": 2,
        "bookmarks": 1,
        "fetches": 4,
        "downloads": 1,
    }

    for key, expected in platform_expected.items():
        if platform[key] != expected:
            raise SystemExit(
                f"R9 receipt: platform {key} mismatch: "
                f"got {platform[key]!r}, expected {expected!r}"
            )

    for key, expected in browser_expected.items():
        if browser[key] != expected:
            raise SystemExit(
                f"R9 receipt: browser {key} mismatch: "
                f"got {browser[key]!r}, expected {expected!r}"
            )

    receipt = {
        "schema": "rivet.retro-receipt/v1",
        "source_revision": args.source_revision,
        "target_profile": "linux-m68k32-68020-qemu-user",
        "evidence_class": "E2",
        "execution": "qemu-user",
        "cpu": "m68k-68020",
        "os_api": "POSIX",
        "pointer_bits": 32,
        "endian": "big",
        "compiler": args.compiler,
        "emulator": args.emulator,
        "platform_proof": platform,
        "browser_proof": browser,
        "claims": [
            "linux-m68k32-user-space-execution",
            "platform-v1-proof-pass",
            "browser-web1-proof-pass",
        ],
        "non_claims": NON_CLAIMS,
        "result": "pass",
    }

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(receipt, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(output)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
