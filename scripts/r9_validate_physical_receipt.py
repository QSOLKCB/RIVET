#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

TARGETS = {
    "windows9x-x86",
    "classic-mac-m68k",
    "classic-mac-powerpc",
    "amiga-m68k",
}

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
        if data.get("target_profile") not in TARGETS:
            raise ValueError("unsupported physical target profile")
        source = data.get("source_revision")
        if not isinstance(source, str) or not re.fullmatch(r"[0-9a-f]{40}", source):
            raise ValueError("source_revision must be 40 lowercase hex characters")

        hardware = data.get("hardware")
        if not isinstance(hardware, dict):
            raise ValueError("hardware object is required")
        for field in ("manufacturer", "model", "cpu", "memory_bytes"):
            if field not in hardware:
                raise ValueError(f"hardware.{field} is required")
        if not isinstance(hardware["memory_bytes"], int) or hardware["memory_bytes"] <= 0:
            raise ValueError("hardware.memory_bytes must be a positive integer")

        proof = data.get("browser_proof")
        if not isinstance(proof, dict):
            raise ValueError("browser_proof object is required")
        expected = {
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
            if not isinstance(item, dict) or not isinstance(item.get("name"), str):
                raise ValueError(f"attachments[{index}] requires name")
            require_sha256(item.get("sha256"), f"attachments[{index}].sha256")

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
