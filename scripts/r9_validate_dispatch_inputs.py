#!/usr/bin/env python3
import argparse
import os
import re
from pathlib import Path

def require_sha256(name: str, value: str, optional: bool = False) -> str:
    value = value.strip().lower()
    if optional and value == "":
        return ""
    if not re.fullmatch(r"[0-9a-f]{64}", value):
        raise ValueError(f"{name} must be exactly 64 hex characters")
    return value

def require_text(name: str, value: str, maximum: int = 160) -> str:
    if not value or len(value) > maximum:
        raise ValueError(f"{name} must be 1..{maximum} characters")
    if any(ord(ch) < 0x20 or ord(ch) == 0x7f for ch in value):
        raise ValueError(f"{name} contains control characters")
    return value

def optional_env_text(
    env_name: str,
    display_name: str,
    pattern: str | None = None,
    choices: set[str] | None = None,
) -> str:
    if env_name not in os.environ:
        return ""
    value = require_text(display_name, os.environ.get(env_name, ""))
    if pattern is not None and re.fullmatch(pattern, value) is None:
        raise ValueError(f"{display_name} has invalid syntax")
    if choices is not None and value not in choices:
        raise ValueError(f"{display_name} is not an allowed value")
    return value

def require_timeout(value: str) -> int:
    if not re.fullmatch(r"[0-9]+", value):
        raise ValueError("timeout_seconds must contain decimal digits only")
    timeout = int(value)
    if timeout < 60 or timeout > 7200:
        raise ValueError("timeout_seconds must be between 60 and 7200")
    return timeout

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--github-env", default="")
    args = parser.parse_args()

    try:
        media = require_sha256(
            "media_sha256",
            os.environ.get("RIVET_INPUT_MEDIA_SHA256", ""),
        )
        rom = require_sha256(
            "rom_sha256",
            os.environ.get("RIVET_INPUT_ROM_SHA256", ""),
            optional=True,
        )
        timeout = require_timeout(
            os.environ.get("RIVET_INPUT_TIMEOUT_SECONDS", "")
        )
        media_label = optional_env_text(
            "RIVET_INPUT_MEDIA_LABEL",
            "media_label",
        )
        guest_partition = optional_env_text(
            "RIVET_INPUT_GUEST_PARTITION",
            "guest_partition",
            r"/dev/(?:sd|hd|vd)[A-Za-z][0-9]{1,2}",
        )
        windows_directory = optional_env_text(
            "RIVET_INPUT_WINDOWS_DIRECTORY",
            "windows_directory",
            r"/[A-Za-z0-9._-]{1,32}",
        )
        dos_drive = optional_env_text(
            "RIVET_INPUT_DOS_DRIVE",
            "dos_drive",
            r"[A-Za-z]:",
        )
        if dos_drive:
            dos_drive = dos_drive.upper()
        architecture = optional_env_text(
            "RIVET_INPUT_ARCHITECTURE",
            "architecture",
            choices={"m68k", "powerpc"},
        )
        partition = optional_env_text(
            "RIVET_INPUT_PARTITION",
            "partition",
            r"[A-Za-z0-9._-]{1,32}",
        )
    except ValueError as exc:
        raise SystemExit(str(exc))

    if args.github_env:
        path = Path(args.github_env)
        with path.open("a", encoding="utf-8") as handle:
            handle.write(f"RIVET_VALIDATED_MEDIA_SHA256={media}\n")
            handle.write(f"RIVET_VALIDATED_ROM_SHA256={rom}\n")
            handle.write(f"RIVET_VALIDATED_TIMEOUT_SECONDS={timeout}\n")
            if media_label:
                handle.write(f"RIVET_VALIDATED_MEDIA_LABEL={media_label}\n")
            if guest_partition:
                handle.write(
                    f"RIVET_VALIDATED_GUEST_PARTITION={guest_partition}\n"
                )
            if windows_directory:
                handle.write(
                    f"RIVET_VALIDATED_WINDOWS_DIRECTORY={windows_directory}\n"
                )
            if dos_drive:
                handle.write(
                    f"RIVET_VALIDATED_DOS_DRIVE={dos_drive}\n"
                )
            if architecture:
                handle.write(
                    f"RIVET_VALIDATED_ARCHITECTURE={architecture}\n"
                )
            if partition:
                handle.write(f"RIVET_VALIDATED_PARTITION={partition}\n")
    else:
        print(media)
        print(rom)
        print(timeout)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
