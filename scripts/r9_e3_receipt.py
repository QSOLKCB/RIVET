#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]

MAX_PROOF_BYTES = 64 * 1024

CLASSIC_MAC_Q800_RELEASES = {
    (7, 1, 0),
    (7, 1, 1),
    (7, 5, 0),
    (7, 5, 1),
    (7, 5, 2),
    (7, 5, 3),
    (7, 5, 5),
    (7, 6, 0),
    (7, 6, 1),
    (8, 0, 0),
    (8, 1, 0),
}

CLASSIC_MAC_MAC99_RELEASES = {
    (8, 6, 0),
    (9, 0, 0),
    (9, 0, 2),
    (9, 0, 3),
    (9, 0, 4),
    (9, 1, 0),
    (9, 2, 0),
    (9, 2, 1),
    (9, 2, 2),
}

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

WINDOWS_IDENTITY_RE = re.compile(
    r"(?P<identity>(?:Microsoft\s+)?Windows\b"
    r"[^\[\r\n]*?\[Version [^\]\r\n]+\])",
    re.IGNORECASE,
)

WINDOWS_9X_VER_PATTERNS = (
    re.compile(
        r"^(?P<identity>(?:Microsoft\s+)?Windows 95\.? "
        r"\[Version 4\.00\."
        r"(?:950(?:A|B|C)?|1111|1212|1214)\])$",
        re.IGNORECASE,
    ),
    re.compile(
        r"^(?P<identity>(?:Microsoft\s+)?Windows 98 "
        r"\[Version 4\.10\.(?:1998|2222A?)\])$",
        re.IGNORECASE,
    ),
    re.compile(
        r"^(?P<identity>(?:Microsoft\s+)?Windows "
        r"(?:Me|Millennium) "
        r"\[Version 4\.90\.3000\])$",
        re.IGNORECASE,
    ),
)

CLASSIC_MAC_OS_RE = re.compile(
    r"^rivet-r9-os: "
    r"target=(?P<target>classic-mac-(?:m68k|powerpc)) "
    r"os=classic-mac-os "
    r"version=(?P<major>[0-9]+)\.(?P<minor>[0-9]+)\.(?P<patch>[0-9]+) "
    r"api=toolbox$"
)

AMIGA_OS_RE = re.compile(
    r"^rivet-r9-os: target=amiga-m68k os=amigaos "
    r"exec_version=(?P<exec_version>[0-9]+) "
    r"exec_revision=(?P<exec_revision>[0-9]+) "
    r"dos_version=(?P<dos_version>[0-9]+) "
    r"dos_revision=(?P<dos_revision>[0-9]+) "
    r"api=exec-dos$"
)

TARGETS = {
    "windows9x-x86": (32, "little"),
    "classic-mac-m68k": (32, "big"),
    "classic-mac-powerpc": (32, "big"),
    "amiga-m68k": (32, "big"),
}

TARGET_EMULATOR_PATTERNS = {
    "windows9x-x86": re.compile(
        r"(?i)^qemu-system-i386(?:\s|:|$)"
    ),
    "classic-mac-m68k": re.compile(
        r"(?i)^qemu-system-m68k(?:\s|:|$)"
    ),
    "classic-mac-powerpc": re.compile(
        r"(?i)^qemu-system-ppc(?:\s|:|$)"
    ),
    "amiga-m68k": re.compile(
        r"(?i)^fs-uae(?:\s|:|$)"
    ),
}

ROM_REQUIRED_TARGETS = {
    "classic-mac-m68k",
    "amiga-m68k",
}

TARGET_REQUIRED_SOURCE_BLOBS = {
    "windows9x-x86": {
        "evidence/r9_win9x_browser_proof.c":
            "7758623c70bf686ccecd8ff261d1c1fbbb0259f6",
        "scripts/r9_build_windows9x_payload.sh":
            "f851ee04f410e06528f06bbfae3176f4f14eca36",
    },
    "classic-mac-m68k": {
        "evidence/r9_guest_browser_proof.c":
            "96eca03a3062fab51f85e1d585149cc3cf309dce",
        "evidence/r9_classic_mac_os_identity.c":
            "329c32517937a6a56f899d1f7febf70bedaff3ac",
        "scripts/r9_build_classic_mac_payload.sh":
            "01954eca1a9a97cef71bb8969750df97b10fca9f",
    },
    "classic-mac-powerpc": {
        "evidence/r9_guest_browser_proof.c":
            "96eca03a3062fab51f85e1d585149cc3cf309dce",
        "evidence/r9_classic_mac_os_identity.c":
            "329c32517937a6a56f899d1f7febf70bedaff3ac",
        "scripts/r9_build_classic_mac_payload.sh":
            "01954eca1a9a97cef71bb8969750df97b10fca9f",
    },
    "amiga-m68k": {
        "evidence/r9_guest_browser_proof.c":
            "96eca03a3062fab51f85e1d585149cc3cf309dce",
        "evidence/r9_amiga_os_identity.c":
            "80576a18ca63c0e193efb830080ae952276a0ffc",
        "scripts/r9_build_amiga_payload.sh":
            "b1199f2d3eb87d91edd0dff3aa2c35fb7362471a",
    },
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
    identities = [
        match.group("identity")
        for match in WINDOWS_IDENTITY_RE.finditer(text)
    ]
    if len(identities) != 1:
        raise ValueError(
            "R9 E3 receipt: Windows proof must contain exactly "
            "one Windows [Version ...] identity occurrence"
        )

    identity = identities[0]
    for pattern in WINDOWS_9X_VER_PATTERNS:
        match = pattern.fullmatch(identity)
        if match:
            return match.group("identity")

    raise ValueError(
        "R9 E3 receipt: Windows identity is not "
        "consumer Windows 95/98/Me"
    )

def require_git_commit(value: str) -> str:
    result = subprocess.run(
        [
            "git",
            "-C",
            str(REPO_ROOT),
            "cat-file",
            "-e",
            f"{value}^{{commit}}",
        ],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if result.returncode != 0:
        raise ValueError(
            "R9 E3 receipt: source revision must resolve "
            "to a RIVET commit"
        )
    return value

def require_target_payload_source(
    value: str,
    target: str,
) -> None:
    for source_path, expected_blob in (
        TARGET_REQUIRED_SOURCE_BLOBS[target].items()
    ):
        result = subprocess.run(
            [
                "git",
                "-C",
                str(REPO_ROOT),
                "rev-parse",
                "--verify",
                f"{value}:{source_path}",
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        actual_blob = result.stdout.strip()
        if (
            result.returncode != 0
            or actual_blob != expected_blob
        ):
            raise ValueError(
                "R9 E3 receipt: source revision does not contain "
                f"frozen {target} blob {expected_blob} at {source_path}"
            )

def _single_os_line(text: str) -> str:
    lines = [
        line.strip()
        for line in text.splitlines()
        if line.strip().startswith("rivet-r9-os:")
    ]
    if len(lines) != 1:
        raise ValueError(
            "R9 E3 receipt: non-Windows proof must contain "
            "exactly one rivet-r9-os identity line"
        )
    return lines[0]

def classic_mac_identity(text: str, target: str) -> str:
    line = _single_os_line(text)
    match = CLASSIC_MAC_OS_RE.fullmatch(line)
    if match is None or match.group("target") != target:
        raise ValueError(
            "R9 E3 receipt: Classic Mac OS identity does not "
            f"match {target}"
        )

    version = tuple(
        int(match.group(field))
        for field in ("major", "minor", "patch")
    )
    if target == "classic-mac-m68k":
        valid = version in CLASSIC_MAC_Q800_RELEASES
    else:
        valid = version in CLASSIC_MAC_MAC99_RELEASES
    if not valid:
        raise ValueError(
            "R9 E3 receipt: Classic Mac OS runtime version "
            f"is incompatible with {target}"
        )
    return line

def amiga_identity(text: str) -> str:
    line = _single_os_line(text)
    match = AMIGA_OS_RE.fullmatch(line)
    if match is None:
        raise ValueError(
            "R9 E3 receipt: AmigaOS Exec/DOS runtime identity "
            "not found"
        )

    exec_version = int(match.group("exec_version"))
    dos_version = int(match.group("dos_version"))
    if not (39 <= exec_version <= 45):
        raise ValueError(
            "R9 E3 receipt: AmigaOS Exec version is outside "
            "the fixed A1200 runtime envelope"
        )
    if not (39 <= dos_version <= 45):
        raise ValueError(
            "R9 E3 receipt: AmigaOS DOS version is outside "
            "the fixed A1200 runtime envelope"
        )
    return line

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

def validate_emulator_identity(value: str, target: str) -> str:
    value = validate_identity(value, "emulator identity")
    pattern = TARGET_EMULATOR_PATTERNS[target]
    if pattern.match(value) is None:
        raise ValueError(
            "R9 E3 receipt: emulator identity is incompatible "
            f"with {target}"
        )
    return value

def read_bounded_proof(path: Path) -> str:
    with path.open("rb") as handle:
        raw = handle.read(MAX_PROOF_BYTES + 1)
    if len(raw) > MAX_PROOF_BYTES:
        raise ValueError(
            "R9 E3 receipt: guest proof exceeds "
            f"{MAX_PROOF_BYTES} bytes"
        )
    return raw.decode("utf-8", errors="strict")

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

    output = Path(args.output)
    temporary_output = output.with_name(
        output.name + ".tmp"
    )
    try:
        output.unlink(missing_ok=True)
        temporary_output.unlink(missing_ok=True)
    except OSError as exc:
        raise SystemExit(
            f"R9 E3 receipt: cannot clear output path: {exc}"
        )

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
        require_git_commit(args.source_revision)
        require_target_payload_source(
            args.source_revision,
            args.target_profile,
        )
    except ValueError as exc:
        raise SystemExit(str(exc))

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
        proof_text = read_bounded_proof(proof_path)
        proof = parse_proof_text(proof_text)
        media_label = validate_identity(
            args.guest_media_label,
            "guest media label",
            160,
        )
        emulator_identity = validate_emulator_identity(
            args.emulator,
            args.target_profile,
        )
        if args.target_profile == "windows9x-x86":
            require_windows_9x_startup(proof_text)
            require_windows_9x_proof_exit(proof_text)
            guest_os_identity = windows_9x_identity(
                proof_text
            )
        elif args.target_profile.startswith("classic-mac-"):
            guest_os_identity = classic_mac_identity(
                proof_text,
                args.target_profile,
            )
        elif args.target_profile == "amiga-m68k":
            guest_os_identity = amiga_identity(proof_text)
        else:
            raise ValueError(
                "R9 E3 receipt: unsupported OS identity target"
            )
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

    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        temporary_output.write_text(
            json.dumps(receipt, indent=2, sort_keys=True)
            + "\n",
            encoding="utf-8",
        )
        temporary_output.replace(output)
    except OSError as exc:
        temporary_output.unlink(missing_ok=True)
        output.unlink(missing_ok=True)
        raise SystemExit(
            f"R9 E3 receipt: failed to publish receipt: {exc}"
        )
    print(output)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
