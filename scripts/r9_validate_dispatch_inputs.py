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
    except ValueError as exc:
        raise SystemExit(str(exc))

    if args.github_env:
        path = Path(args.github_env)
        with path.open("a", encoding="utf-8") as handle:
            handle.write(f"RIVET_VALIDATED_MEDIA_SHA256={media}\n")
            handle.write(f"RIVET_VALIDATED_ROM_SHA256={rom}\n")
            handle.write(f"RIVET_VALIDATED_TIMEOUT_SECONDS={timeout}\n")
    else:
        print(media)
        print(rom)
        print(timeout)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
