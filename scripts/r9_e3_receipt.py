#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

PROOF_RE = re.compile(
    r"^rivet-r9-guest: "
    r"target=(?P<target>\S+) "
    r"source=(?P<source>[0-9a-f]{40}) "
    r"pointer_bits=(?P<pointer_bits>[0-9]+) "
    r"endian=(?P<endian>little|big) "
    r"document_fnv1a64=(?P<document_fnv1a64>[0-9a-f]{16}) "
    r"source_fnv1a64=(?P<source_fnv1a64>[0-9a-f]{16}) "
    r"history=(?P<history>[0-9]+) "
    r"bookmarks=(?P<bookmarks>[0-9]+) "
    r"fetches=(?P<fetches>[0-9]+) "
    r"downloads=(?P<downloads>[0-9]+)$"
)

TARGETS = {
    "windows9x-x86": (32, "little"),
    "classic-mac-m68k": (32, "big"),
    "classic-mac-powerpc": (32, "big"),
    "amiga-m68k": (32, "big"),
}

def parse_proof(path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="strict")
    for line in text.splitlines():
        match = PROOF_RE.match(line.strip())
        if match:
            data = match.groupdict()
            for key in (
                "pointer_bits",
                "history",
                "bookmarks",
                "fetches",
                "downloads",
            ):
                data[key] = int(data[key])
            return data
    raise ValueError("R9 guest proof line not found")

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--proof", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--target-profile", choices=sorted(TARGETS), required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--emulator", required=True)
    parser.add_argument("--guest-media-sha256", required=True)
    parser.add_argument("--guest-media-label", required=True)
    parser.add_argument("--payload-sha256", required=True)
    parser.add_argument("--rom-sha256", default="")
    parser.add_argument("--notes", default="")
    args = parser.parse_args()

    if not re.fullmatch(r"[0-9a-f]{40}", args.source_revision):
        raise SystemExit("R9 E3 receipt: source revision must be 40 lowercase hex characters")
    for label, value in (
        ("guest media", args.guest_media_sha256),
        ("payload", args.payload_sha256),
    ):
        if not re.fullmatch(r"[0-9a-f]{64}", value):
            raise SystemExit(f"R9 E3 receipt: {label} SHA-256 must be 64 lowercase hex characters")
    if args.rom_sha256 and not re.fullmatch(r"[0-9a-f]{64}", args.rom_sha256):
        raise SystemExit("R9 E3 receipt: ROM SHA-256 must be 64 lowercase hex characters")

    try:
        proof = parse_proof(Path(args.proof))
    except (OSError, UnicodeError, ValueError) as exc:
        raise SystemExit(str(exc))

    pointer_bits, endian = TARGETS[args.target_profile]
    expected = {
        "target": args.target_profile,
        "source": args.source_revision,
        "pointer_bits": pointer_bits,
        "endian": endian,
        "document_fnv1a64": "75be6cc92698ac1a",
        "source_fnv1a64": "5cf7c63a1fa3d9b4",
        "history": 2,
        "bookmarks": 1,
        "fetches": 4,
        "downloads": 1,
    }
    for key, value in expected.items():
        if proof[key] != value:
            raise SystemExit(
                f"R9 E3 receipt: {key} mismatch: "
                f"got {proof[key]!r}, expected {value!r}"
            )

    receipt = {
        "schema": "rivet.retro-e3-receipt/v1",
        "source_revision": args.source_revision,
        "target_profile": args.target_profile,
        "evidence_class": "E3",
        "execution": "full-system-emulation",
        "emulator": args.emulator,
        "guest_media": {
            "label": args.guest_media_label,
            "sha256": args.guest_media_sha256,
            "redistributed_by_rivet": False,
        },
        "payload_sha256": args.payload_sha256,
        "rom_sha256": args.rom_sha256 or None,
        "browser_proof": proof,
        "notes": args.notes or None,
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
