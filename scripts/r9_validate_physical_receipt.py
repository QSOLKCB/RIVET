#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]

WINDOWS_CPU_MIN_GENERATION = {
    "Windows 95": 3,
    "Windows 98": 4,
    "Windows Me": 5,
}

WINDOWS_MEMORY_MIN_BYTES = {
    "Windows 95": 4 * 1024 * 1024,
    "Windows 98": 16 * 1024 * 1024,
    "Windows Me": 32 * 1024 * 1024,
}

WINDOWS_CPU_GENERATION_PATTERNS = (
    (
        3,
        re.compile(
            r"(?i)(?:(?:intel\s+)?"
            r"(?:(?:80|i)?386(?:dx|sx)?)|"
            r"(?:amd\s+)?am386(?:dx|sx)?)"
        ),
    ),
    (
        4,
        re.compile(
            r"(?i)(?:(?:intel\s+)?"
            r"(?:(?:80|i)?486(?:dx(?:2|4)?|sx)?)|"
            r"(?:amd\s+)?am486(?:dx(?:2|4)?|sx)?)"
        ),
    ),
    (
        5,
        re.compile(
            r"(?i)(?:(?:intel\s+)?"
            r"(?:i586|pentium(?:\s+mmx)?)|"
            r"(?:amd\s+)?k[56](?:-[23])?|"
            r"cyrix\s+6x86)"
        ),
    ),
    (
        6,
        re.compile(
            r"(?i)(?:(?:intel\s+)?"
            r"(?:i686|pentium\s+(?:pro|ii|iii|4)|celeron)|"
            r"(?:amd\s+)?(?:athlon(?:\s+xp)?|duron)|"
            r"via\s+c3)"
        ),
    ),
)

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
            r"(?i)(?:"
            r"(?:intel\s+)?(?:x86|"
            r"(?:80|i)?386(?:dx|sx)?|"
            r"(?:80|i)?486(?:dx(?:2|4)?|sx)?|"
            r"i[5-6]86|pentium(?:\s+"
            r"(?:pro|ii|iii|4|mmx))?|celeron)|"
            r"(?:amd\s+)?(?:am386(?:dx|sx)?|"
            r"am486(?:dx(?:2|4)?|sx)?|"
            r"athlon(?:\s+xp)?|duron|"
            r"k[56](?:-[23])?)|"
            r"cyrix\s+6x86|via\s+c3"
            r")"
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
            r"(?i)(?:(?:motorola\s+)?m68k|"
            r"(?:motorola\s+)?(?:mc)?68"
            r"(?:000|010|020|030|040|060))"
        ),
    },
    "classic-mac-powerpc": {
        "pointer_bits": 32,
        "endian": "big",
        "os_names": {"Classic Mac OS"},
        "os_versions": {
            "Classic Mac OS": (
                r"^(?:7\.(?:"
                r"1\.(?:[2-9]|[1-9][0-9]+)|"
                r"[2-9](?:\.[0-9]+)?)|"
                r"8\.[0-9]+(?:\.[0-9]+)?|"
                r"9\.[0-9]+(?:\.[0-9]+)?)$"
            ),
        },
        "api": "Mac OS Toolbox",
        "cpu_pattern": (
            r"(?i)(?:(?:(?:motorola|ibm|apple)\s+)?"
            r"(?:powerpc|ppc)(?:\s+"
            r"(?:601|603e?|604e?|750|7400|g3|g4))?|"
            r"(?:601|603e?|604e?|750|7400|g3|g4))"
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
            r"(?i)(?:(?:motorola\s+)?m68k|"
            r"(?:motorola\s+)?(?:mc)?68"
            r"(?:000|010|020|030|040|060))"
        ),
    },
}

def reject_duplicate_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(
                f"duplicate JSON field: {key}"
            )
        result[key] = value
    return result

def windows_cpu_generation(cpu_identity: str) -> int | None:
    for generation, pattern in WINDOWS_CPU_GENERATION_PATTERNS:
        if pattern.fullmatch(cpu_identity):
            return generation
    return None

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

def require_evidence_name(value: object, field: str) -> str:
    name = require_identity(value, field)
    if name.upper().startswith("REPLACE-WITH-"):
        raise ValueError(
            f"{field} must not use a shipped placeholder sentinel"
        )
    return name

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
            "source_revision must resolve to a RIVET commit"
        )
    return value

def require_sha256(value: object, field: str) -> str:
    if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
        raise ValueError(f"{field} must be 64 lowercase hex characters")
    if value == "0" * 64:
        raise ValueError(f"{field} must not be the all-zero placeholder digest")
    return value

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("receipt")
    args = parser.parse_args()

    path = Path(args.receipt)
    try:
        data = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=reject_duplicate_object,
        )
    except (
        OSError,
        UnicodeError,
        json.JSONDecodeError,
        ValueError,
    ) as exc:
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
        if source == "0" * 40:
            raise ValueError("source_revision must not be the Git null OID")
        require_git_commit(source)

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
            require_evidence_name(
                hardware[field],
                f"hardware.{field}",
            )
        cpu_identity = hardware["cpu"].strip()
        if re.fullmatch(
            target_contract["cpu_pattern"],
            cpu_identity,
        ) is None:
            raise ValueError(
                "hardware.cpu is incompatible with "
                f"{target}"
            )
        if target == "windows9x-x86":
            cpu_generation = windows_cpu_generation(
                cpu_identity
            )
            minimum_generation = (
                WINDOWS_CPU_MIN_GENERATION[os_name]
            )
            if (
                cpu_generation is None
                or cpu_generation < minimum_generation
            ):
                raise ValueError(
                    "hardware.cpu is below the minimum "
                    f"for {os_name}"
                )
        memory_bytes = require_integer(
            hardware["memory_bytes"],
            "hardware.memory_bytes",
            1,
        )
        if target == "windows9x-x86":
            minimum_memory = WINDOWS_MEMORY_MIN_BYTES[os_name]
            if memory_bytes < minimum_memory:
                raise ValueError(
                    "hardware.memory_bytes is below the minimum "
                    f"for {os_name}"
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
            require_evidence_name(
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
