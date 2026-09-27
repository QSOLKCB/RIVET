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

WINDOWS_9X_VER_PATTERNS = (
    re.compile(
        r"^(?P<identity>.*\bWindows 95\b.*"
        r"\[Version 4\.00"
        r"(?:\.[0-9A-Za-z]+)*\].*)$",
        re.IGNORECASE,
    ),
    re.compile(
        r"^(?P<identity>.*\bWindows 98\b.*"
        r"\[Version 4\.10"
        r"(?:\.[0-9A-Za-z]+)*\].*)$",
        re.IGNORECASE,
    ),
    re.compile(
        r"^(?P<identity>.*\bWindows "
        r"(?:Me|Millennium)\b.*"
        r"\[Version 4\.90"
        r"(?:\.[0-9A-Za-z]+)*\].*)$",
        re.IGNORECASE,
    ),
)

WINDOWS_VERSION_LINE_RE = re.compile(
    r"^.*\bWindows\b.*\[Version [^\]]+\].*$",
    re.IGNORECASE,
)

TARGETS = {
    "windows9x-x86": (32, "little"),
    "classic-mac-m68k": (32, "big"),
    "classic-mac-powerpc": (32, "big"),
    "amiga-m68k": (32, "big"),
}

ROM_REQUIRED_TARGETS = {
    "classic-mac-m68k",
    "amiga-m68k",
}

def require_windows_9x_startup(text: str) -> None:
    if "startup_stage=winstart" not in {
        line.strip() for line in text.splitlines()
    }:
        raise ValueError(
            "R9 E3 receipt: Windows proof was not recorded "
            "from WINSTART startup stage"
        )

def require_windows_9x_proof_exit(text: str) -> None:
    markers = [
        line.strip()
        for line in text.splitlines()
        if line.strip().startswith("proof_exit=")
    ]
    if markers != ["proof_exit=0"]:
        raise ValueError(
            "R9 E3 receipt: Windows guest proof exit "
            "must be exactly proof_exit=0"
        )

def windows_9x_identity(text: str) -> str:
    identity_lines = [
        line.strip()
        for line in text.splitlines()
        if WINDOWS_VERSION_LINE_RE.match(line.strip())
    ]
    if len(identity_lines) != 1:
        raise ValueError(
            "R9 E3 receipt: Windows proof must contain exactly "
            "one Windows [Version ...] identity line"
        )

    identity = identity_lines[0]
    for pattern in WINDOWS_9X_VER_PATTERNS:
        match = pattern.match(identity)
        if match:
            return match.group("identity")

    raise ValueError(
        "R9 E3 receipt: Windows identity is not "
        "consumer Windows 95/98/Me"
    )

def require_sha256(value: str, field: str) -> str:
    if not re.fullmatch(r"[0-9a-f]{64}", value):
        raise ValueError(
            f"R9 E3 receipt: {field} SHA-256 must be "
            "64 lowercase hex characters"
        )
    if value == "0" * 64:
        raise ValueError(
            f"R9 E3 receipt: {field} SHA-256 must not be "
            "the all-zero placeholder digest"
        )
    return value

def validate_identity(
    value: str,
    field: str,
    maximum: int = 240,
) -> str:
    value = value.strip()
    if not value or len(value) > maximum:
        raise ValueError(
            f"R9 E3 receipt: {field} must be 1..{maximum} "
            "non-whitespace characters"
        )
    if any(ord(ch) < 0x20 or ord(ch) == 0x7f for ch in value):
        raise ValueError(
            f"R9 E3 receipt: {field} contains control characters"
        )
    return value

def parse_proof_text(text: str) -> dict:
    proof_lines = [
        line.strip()
        for line in text.splitlines()
        if line.strip().startswith("rivet-r9-guest:")
    ]
    if len(proof_lines) != 1:
        raise ValueError(
            "R9 guest proof must contain exactly one "
            "rivet-r9-guest proof line"
        )

    match = PROOF_RE.match(proof_lines[0])
    if match is None:
        raise ValueError(
            "R9 guest proof line is malformed"
        )

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
        raise SystemExit(
            "R9 E3 receipt: source revision must be "
            "40 lowercase hex characters"
        )
    if args.source_revision == "0" * 40:
        raise SystemExit(
            "R9 E3 receipt: source revision must not be "
            "the Git null OID"
        )
    try:
        require_sha256(
            args.guest_media_sha256,
            "guest media",
        )
        require_sha256(
            args.payload_sha256,
            "payload",
        )
    except ValueError as exc:
        raise SystemExit(str(exc))

    if args.target_profile in ROM_REQUIRED_TARGETS and not args.rom_sha256:
        raise SystemExit(
            "R9 E3 receipt: ROM SHA-256 is required for "
            f"{args.target_profile}"
        )
    if args.rom_sha256:
        try:
            require_sha256(args.rom_sha256, "ROM")
        except ValueError as exc:
            raise SystemExit(str(exc))

    try:
        proof_path = Path(args.proof)
        proof_text = proof_path.read_text(
            encoding="utf-8",
            errors="strict",
        )
        proof = parse_proof_text(proof_text)
        media_label = validate_identity(
            args.guest_media_label,
            "guest media label",
            160,
        )
        emulator_identity = validate_identity(
            args.emulator,
            "emulator identity",
        )
        if args.target_profile == "windows9x-x86":
            require_windows_9x_startup(proof_text)
            require_windows_9x_proof_exit(proof_text)
            guest_os_identity = windows_9x_identity(
                proof_text
            )
        else:
            guest_os_identity = None
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
        "emulator": emulator_identity,
        "guest_media": {
            "label": media_label,
            "sha256": args.guest_media_sha256,
            "redistributed_by_rivet": False,
        },
        "payload_sha256": args.payload_sha256,
        "rom_sha256": args.rom_sha256 or None,
        "guest_os_identity": guest_os_identity,
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
