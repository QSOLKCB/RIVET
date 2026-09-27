#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

TARGETS = {
    "windows9x-x86": {
        "pointer_bits": 32,
        "endian": "little",
        "os_names": {"Windows 95", "Windows 98", "Windows Me"},
        "os_versions": {
            "Windows 95": r"^4\.00(?:\.[0-9A-Za-z]+)*$",
            "Windows 98": r"^4\.10(?:\.[0-9A-Za-z]+)*$",
            "Windows Me": r"^4\.90(?:\.[0-9A-Za-z]+)*$",
        },
        "api": "Win32",
        "cpu_pattern": (
            r"(?i)(?:\bx86\b|\b80386\b|\b80486\b|"
            r"\bi[3-6]86\b|\bpentium(?:\s+"
            r"(?:pro|ii|iii|4|mmx))?\b|\bceleron\b|"
            r"\bathlon(?:\s+xp)?\b|\bduron\b|"
            r"\bk[56](?:-[23])?\b|\bcyrix\s+6x86\b|"
            r"\bvia\s+c3\b)"
        ),
    },
    "classic-mac-m68k": {
        "pointer_bits": 32,
        "endian": "big",
        "os_names": {"Classic Mac OS"},
        "os_versions": {
            "Classic Mac OS": (
                r"^(?:6\.[0-9]+(?:\.[0-9]+)?|"
                r"7\.[0-9]+(?:\.[0-9]+)?|"
                r"8\.[01](?:\.[0-9]+)?)$"
            ),
        },
        "api": "Mac OS Toolbox",
        "cpu_pattern": (
            r"(?i)(?:m68k|(?:motorola\s+)?"
            r"68(?:000|010|020|030|040|060))"
        ),
    },
    "classic-mac-powerpc": {
        "pointer_bits": 32,
        "endian": "big",
        "os_names": {"Classic Mac OS"},
        "os_versions": {
            "Classic Mac OS": (
                r"^(?:7\.[0-9]+(?:\.[0-9]+)?|"
                r"8\.[0-9]+(?:\.[0-9]+)?|"
                r"9\.[0-9]+(?:\.[0-9]+)?)$"
            ),
        },
        "api": "Mac OS Toolbox",
        "cpu_pattern": (
            r"(?i)(?:powerpc|\bppc\b|\b60[134]\b|"
            r"\b750\b|\bg[34]\b)"
        ),
    },
    "amiga-m68k": {
        "pointer_bits": 32,
        "endian": "big",
        "os_names": {"AmigaOS"},
        "os_versions": {
            "AmigaOS": r"^[1-3]\.[0-9]+(?:\.[0-9]+)?$",
        },
        "api": "AmigaOS",
        "cpu_pattern": (
            r"(?i)(?:m68k|(?:motorola\s+)?"
            r"68(?:000|010|020|030|040|060))"
        ),
    },
}

def require_identity(value: object, field: str) -> str:
    if not isinstance(value, str):
        raise ValueError(f"{field} must be a string")
    value = value.strip()
    if not value or len(value) > 160:
        raise ValueError(f"{field} must be 1..160 non-whitespace characters")
    if any(ord(ch) < 0x20 or ord(ch) == 0x7f for ch in value):
        raise ValueError(f"{field} contains control characters")
    return value

def require_integer(
    value: object,
    field: str,
    minimum: int = 0,
) -> int:
    if type(value) is not int or value < minimum:
        raise ValueError(
            f"{field} must be an integer >= {minimum}"
        )
    return value

def require_sha256(value: object, field: str) -> str:
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
        raise ValueError(f"{field} must be 64 lowercase hex characters")
    return value

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("receipt")
    args = parser.parse_args()

    path = Path(args.receipt)
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise SystemExit(str(exc))

    try:
        if data.get("schema") != "rivet.retro-e4-receipt/v1":
            raise ValueError("unexpected E4 receipt schema")
        if data.get("evidence_class") != "E4":
            raise ValueError("physical receipt must declare E4")
        if data.get("execution") != "physical-hardware":
            raise ValueError("physical receipt must declare physical-hardware execution")
        target = data.get("target_profile")
        if target not in TARGETS:
            raise ValueError("unsupported physical target profile")
        source = data.get("source_revision")
        if not isinstance(source, str) or not re.fullmatch(r"[0-9a-f]{40}", source):
            raise ValueError("source_revision must be 40 lowercase hex characters")

        software = data.get("software_environment")
        if not isinstance(software, dict):
            raise ValueError(
                "software_environment object is required"
            )
        for field in ("os_name", "os_version", "api"):
            if field not in software:
                raise ValueError(
                    f"software_environment.{field} is required"
                )
        os_name = require_identity(
            software["os_name"],
            "software_environment.os_name",
        )
        os_version = require_identity(
            software["os_version"],
            "software_environment.os_version",
        )
        api = require_identity(
            software["api"],
            "software_environment.api",
        )
        target_contract = TARGETS[target]
        if os_name not in target_contract["os_names"]:
            raise ValueError(
                "software_environment.os_name does not match "
                f"{target}"
            )
        if api != target_contract["api"]:
            raise ValueError(
                "software_environment.api does not match "
                f"{target}"
            )
        version_pattern = target_contract["os_versions"][os_name]
        if re.fullmatch(version_pattern, os_version) is None:
            raise ValueError(
                "software_environment.os_version does not match "
                f"{target}/{os_name}"
            )

        hardware = data.get("hardware")
        if not isinstance(hardware, dict):
            raise ValueError("hardware object is required")
        for field in ("manufacturer", "model", "cpu", "memory_bytes"):
            if field not in hardware:
                raise ValueError(f"hardware.{field} is required")
        for field in ("manufacturer", "model", "cpu"):
            require_identity(hardware[field], f"hardware.{field}")
        cpu_identity = hardware["cpu"].strip()
        if re.search(
            target_contract["cpu_pattern"],
            cpu_identity,
        ) is None:
            raise ValueError(
                "hardware.cpu is incompatible with "
                f"{target}"
            )
        require_integer(
            hardware["memory_bytes"],
            "hardware.memory_bytes",
            1,
        )

        proof = data.get("browser_proof")
        if not isinstance(proof, dict):
            raise ValueError("browser_proof object is required")
        pointer_bits = target_contract["pointer_bits"]
        endian = target_contract["endian"]
        for field in (
            "pointer_bits",
            "history",
            "bookmarks",
            "fetches",
            "downloads",
        ):
            require_integer(
                proof.get(field),
                f"browser_proof.{field}",
                0,
            )

        expected = {
            "target": target,
            "source": source,
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
            if proof.get(key) != value:
                raise ValueError(f"browser_proof.{key} mismatch")

        attachments = data.get("attachments")
        if not isinstance(attachments, list) or not attachments:
            raise ValueError("at least one hashed attachment is required")
        for index, item in enumerate(attachments):
            if not isinstance(item, dict):
                raise ValueError(
                    f"attachments[{index}] must be an object"
                )
            require_identity(
                item.get("name"),
                f"attachments[{index}].name",
            )
            require_sha256(
                item.get("sha256"),
                f"attachments[{index}].sha256",
            )

        if data.get("result") != "pass":
            raise ValueError("physical receipt result must be pass")
    except ValueError as exc:
        raise SystemExit(str(exc))

    print(
        "validated E4 receipt:",
        data["target_profile"],
        data["hardware"]["manufacturer"],
        data["hardware"]["model"],
        data["source_revision"],
    )
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
