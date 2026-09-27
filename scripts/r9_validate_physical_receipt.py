#!/usr/bin/env python3
import argparse
import json
import re
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]

MAX_RECEIPT_BYTES = 64 * 1024

E4_REQUIRED_SOURCE_BLOBS = {
    "examples/r8_browser_proof.c":
        "70b368992094b92bc7050ea36a393e46b7199bf2",
    "apps/browser/browser.c":
        "a808edfc81b71f3b624cce5fc395e64e024d0093",
    "include/rivet/browser.h":
        "a7b2eac03cf08a12cd0baba8c348b04bc7f113de",
}

TARGET_MEMORY_MAX_BYTES = {
    "windows9x-x86": 2 * 1024 * 1024 * 1024,
    "classic-mac-m68k": 256 * 1024 * 1024,
    "classic-mac-powerpc": 2 * 1024 * 1024 * 1024,
    "amiga-m68k": 512 * 1024 * 1024,
}

CLASSIC_MAC_M68K_RELEASES = {
    "6.0",
    "6.0.1",
    "6.0.2",
    "6.0.3",
    "6.0.4",
    "6.0.5",
    "6.0.7",
    "6.0.8",
    "7.0",
    "7.0.1",
    "7.1",
    "7.1.1",
    "7.5",
    "7.5.1",
    "7.5.2",
    "7.5.3",
    "7.5.5",
    "7.6",
    "7.6.1",
    "8.0",
    "8.1",
}

AMIGA_OS_RELEASES = {
    "1.0",
    "1.1",
    "1.2",
    "1.3",
    "2.0",
    "2.04",
    "2.05",
    "2.1",
    "3.0",
    "3.1",
    "3.1.4",
    "3.2",
    "3.2.1",
    "3.2.2",
    "3.2.3",
    "3.5",
    "3.9",
}

CLASSIC_MAC_POWERPC_RELEASES = {
    "7.1.2",
    "7.5",
    "7.5.1",
    "7.5.2",
    "7.5.3",
    "7.5.5",
    "7.6",
    "7.6.1",
    "8.0",
    "8.1",
    "8.5",
    "8.5.1",
    "8.6",
    "9.0",
    "9.0.2",
    "9.0.3",
    "9.0.4",
    "9.1",
    "9.2",
    "9.2.1",
    "9.2.2",
}

CLASSIC_MAC_M68K_MEMORY_MIN_BYTES = {
    "6.0": 1 * 1024 * 1024,
    "6.0.1": 1 * 1024 * 1024,
    "6.0.2": 1 * 1024 * 1024,
    "6.0.3": 1 * 1024 * 1024,
    "6.0.4": 1 * 1024 * 1024,
    "6.0.5": 1 * 1024 * 1024,
    "6.0.7": 1 * 1024 * 1024,
    "6.0.8": 1 * 1024 * 1024,
    "7.0": 2 * 1024 * 1024,
    "7.0.1": 2 * 1024 * 1024,
    "7.1": 2 * 1024 * 1024,
    "7.1.1": 2 * 1024 * 1024,
    "7.5": 4 * 1024 * 1024,
    "7.5.1": 4 * 1024 * 1024,
    "7.5.2": 4 * 1024 * 1024,
    "7.5.3": 4 * 1024 * 1024,
    "7.5.5": 4 * 1024 * 1024,
    "7.6": 8 * 1024 * 1024,
    "7.6.1": 8 * 1024 * 1024,
    "8.0": 12 * 1024 * 1024,
    "8.1": 12 * 1024 * 1024,
}

CLASSIC_MAC_POWERPC_MEMORY_MIN_BYTES = {
    "7.1.2": 8 * 1024 * 1024,
    "7.5": 8 * 1024 * 1024,
    "7.5.1": 8 * 1024 * 1024,
    "7.5.2": 8 * 1024 * 1024,
    "7.5.3": 8 * 1024 * 1024,
    "7.5.5": 8 * 1024 * 1024,
    "7.6": 8 * 1024 * 1024,
    "7.6.1": 8 * 1024 * 1024,
    "8.0": 12 * 1024 * 1024,
    "8.1": 12 * 1024 * 1024,
    "8.5": 24 * 1024 * 1024,
    "8.5.1": 24 * 1024 * 1024,
    "8.6": 24 * 1024 * 1024,
    "9.0": 32 * 1024 * 1024,
    "9.0.2": 32 * 1024 * 1024,
    "9.0.3": 32 * 1024 * 1024,
    "9.0.4": 32 * 1024 * 1024,
    "9.1": 32 * 1024 * 1024,
    "9.2": 32 * 1024 * 1024,
    "9.2.1": 32 * 1024 * 1024,
    "9.2.2": 32 * 1024 * 1024,
}

CLASSIC_MAC_M68K_CPU_MIN_GENERATION = {
    "7.6": 30,
    "7.6.1": 30,
    "8.0": 40,
    "8.1": 40,
}

CLASSIC_MAC_POWERPC_CPU_MIN_GENERATION = {
    "9.2": 4,
    "9.2.1": 4,
    "9.2.2": 4,
}

AMIGA_MEMORY_MIN_BYTES = 512 * 1024
AMIGA_MID_MEMORY_MIN_BYTES = 2 * 1024 * 1024
AMIGA_HIGH_MEMORY_MIN_BYTES = 4 * 1024 * 1024
AMIGA_CPU_MIN_GENERATION = {
    "3.5": 20,
    "3.9": 20,
}

WINDOWS_CPU_MIN_GENERATION = {
    "Windows 95": 3,
    "Windows 98": 4,
    "Windows Me": 5,
}

WINDOWS_RELEASES = {
    "Windows 95": {
        "4.00.950",
        "4.00.950A",
        "4.00.950B",
        "4.00.950C",
        "4.00.1111",
        "4.00.1212",
        "4.00.1214",
    },
    "Windows 98": {
        "4.10.1998",
        "4.10.2222",
        "4.10.2222A",
    },
    "Windows Me": {"4.90.3000"},
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
            r"(?:000|010|060|(?:EC|LC)?(?:020|030|040)))"
        ),
    },
    "classic-mac-powerpc": {
        "pointer_bits": 32,
        "endian": "big",
        "os_names": {"Classic Mac OS"},
        "os_versions": {
            "Classic Mac OS": (
                r"^[0-9]+\.[0-9]+(?:\.[0-9]+)?$"
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
            r"(?:000|010|060|(?:EC)?(?:020|030|040)))"
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

def m68k_cpu_generation(cpu_identity: str) -> int | None:
    if re.fullmatch(
        r"(?i)(?:motorola\s+)?m68k",
        cpu_identity,
    ):
        return 0
    match = re.fullmatch(
        r"(?i)(?:motorola\s+)?(?:mc)?68"
        r"(?:(?:EC|LC)?(?P<generation>020|030|040)|"
        r"(?P<other>000|010|060))",
        cpu_identity,
    )
    if match is None:
        return None
    generation = match.group("generation") or match.group("other")
    return int(generation)

def powerpc_cpu_generation(cpu_identity: str) -> int | None:
    match = re.fullmatch(
        r"(?i)(?:(?:motorola|ibm|apple)\s+)?"
        r"(?:(?:powerpc|ppc)(?:\s+"
        r"(?P<model>601|603e?|604e?|750|7400|g3|g4))?"
        r"|(?P<bare>601|603e?|604e?|750|7400|g3|g4))",
        cpu_identity,
    )
    if match is None:
        return None
    model = (match.group("model") or match.group("bare") or "601").lower()
    if model in {"7400", "g4"}:
        return 5
    if model in {"750", "g3"}:
        return 4
    if model.startswith("604"):
        return 3
    if model.startswith("603"):
        return 2
    return 1

def read_bounded_receipt(path: Path) -> str:
    with path.open("rb") as handle:
        raw = handle.read(MAX_RECEIPT_BYTES + 1)
    if len(raw) > MAX_RECEIPT_BYTES:
        raise ValueError(
            "E4 receipt exceeds "
            f"{MAX_RECEIPT_BYTES} bytes"
        )
    return raw.decode("utf-8", errors="strict")

def reject_non_json_constant(value: str):
    raise ValueError(f"non-JSON numeric constant: {value}")

def require_identity(value: object, field: str) -> str:
    if not isinstance(value, str):
        raise ValueError(f"{field} must be a string")
    if any(ord(ch) < 0x20 or ord(ch) == 0x7f for ch in value):
        raise ValueError(f"{field} contains control characters")
    normalized = value.strip()
    if not normalized or len(normalized) > 160:
        raise ValueError(f"{field} must be 1..160 non-whitespace characters")
    return normalized

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

def require_source_blobs(value: str) -> None:
    for source_path, expected_blob in E4_REQUIRED_SOURCE_BLOBS.items():
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
                "source_revision does not contain frozen WEB1 "
                f"blob {expected_blob} at {source_path}"
            )

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
            read_bounded_receipt(path),
            object_pairs_hook=reject_duplicate_object,
            parse_constant=reject_non_json_constant,
        )
    except (
        OSError,
        UnicodeError,
        json.JSONDecodeError,
        ValueError,
    ) as exc:
        raise SystemExit(str(exc))

    try:
        if not isinstance(data, dict):
            raise ValueError(
                "E4 receipt root must be a JSON object"
            )
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
        require_source_blobs(source)

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
        if (
            target == "windows9x-x86"
            and os_version not in WINDOWS_RELEASES[os_name]
        ):
            raise ValueError(
                "software_environment.os_version is not a "
                f"published {os_name} release"
            )
        if (
            target == "classic-mac-m68k"
            and os_version not in CLASSIC_MAC_M68K_RELEASES
        ):
            raise ValueError(
                "software_environment.os_version is not a "
                "published Classic Mac OS m68k release"
            )
        if (
            target == "classic-mac-powerpc"
            and os_version not in CLASSIC_MAC_POWERPC_RELEASES
        ):
            raise ValueError(
                "software_environment.os_version is not a "
                "published Classic Mac OS PowerPC release"
            )
        if (
            target == "amiga-m68k"
            and os_version not in AMIGA_OS_RELEASES
        ):
            raise ValueError(
                "software_environment.os_version is not a "
                "published AmigaOS m68k release"
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
        elif target == "classic-mac-m68k":
            cpu_generation = m68k_cpu_generation(cpu_identity)
            minimum_generation = (
                CLASSIC_MAC_M68K_CPU_MIN_GENERATION.get(
                    os_version,
                    0,
                )
            )
            if (
                cpu_generation is None
                or cpu_generation < minimum_generation
            ):
                raise ValueError(
                    "hardware.cpu is below the minimum "
                    f"for Classic Mac OS {os_version}"
                )
        elif target == "classic-mac-powerpc":
            cpu_generation = powerpc_cpu_generation(cpu_identity)
            minimum_generation = (
                CLASSIC_MAC_POWERPC_CPU_MIN_GENERATION.get(
                    os_version,
                    1,
                )
            )
            if (
                cpu_generation is None
                or cpu_generation < minimum_generation
            ):
                raise ValueError(
                    "hardware.cpu is below the minimum "
                    f"for Classic Mac OS {os_version}"
                )
        else:
            cpu_generation = m68k_cpu_generation(cpu_identity)
            minimum_generation = AMIGA_CPU_MIN_GENERATION.get(
                os_version,
                0,
            )
            if (
                cpu_generation is None
                or cpu_generation < minimum_generation
            ):
                raise ValueError(
                    "hardware.cpu is below the minimum "
                    f"for AmigaOS {os_version}"
                )
        memory_bytes = require_integer(
            hardware["memory_bytes"],
            "hardware.memory_bytes",
            1,
        )
        if target == "windows9x-x86":
            minimum_memory = WINDOWS_MEMORY_MIN_BYTES[os_name]
        elif target == "classic-mac-m68k":
            minimum_memory = CLASSIC_MAC_M68K_MEMORY_MIN_BYTES[
                os_version
            ]
        elif target == "classic-mac-powerpc":
            minimum_memory = CLASSIC_MAC_POWERPC_MEMORY_MIN_BYTES[
                os_version
            ]
        else:
            if os_version in {"3.5", "3.9"}:
                minimum_memory = AMIGA_HIGH_MEMORY_MIN_BYTES
            elif os_version in {
                "3.1.4",
                "3.2",
                "3.2.1",
                "3.2.2",
                "3.2.3",
            }:
                minimum_memory = AMIGA_MID_MEMORY_MIN_BYTES
            else:
                minimum_memory = AMIGA_MEMORY_MIN_BYTES
        if memory_bytes < minimum_memory:
            raise ValueError(
                "hardware.memory_bytes is below the minimum "
                f"for {target}/{os_name} {os_version}"
            )
        maximum_memory = TARGET_MEMORY_MAX_BYTES[target]
        if memory_bytes > maximum_memory:
            raise ValueError(
                "hardware.memory_bytes exceeds the realizable "
                f"maximum for {target}: {maximum_memory}"
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
