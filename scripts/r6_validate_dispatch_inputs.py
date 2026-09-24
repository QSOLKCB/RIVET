#!/usr/bin/env python3
import argparse
import os
import re
from pathlib import Path

HASH_RE = re.compile(r"^[0-9A-Fa-f]{64}$")
TIMEOUT_RE = re.compile(r"^[0-9]+$")
MIN_TIMEOUT = 300
MAX_TIMEOUT = 6000

def validate(image_sha256: str, timeout_text: str):
    if not HASH_RE.fullmatch(image_sha256 or ""):
        raise ValueError("image_sha256 must be exactly 64 hexadecimal characters")

    if not TIMEOUT_RE.fullmatch(timeout_text or ""):
        raise ValueError("timeout_seconds must contain decimal digits only")

    timeout = int(timeout_text, 10)
    if timeout < MIN_TIMEOUT or timeout > MAX_TIMEOUT:
        raise ValueError(
            f"timeout_seconds must be between {MIN_TIMEOUT} and {MAX_TIMEOUT} seconds"
        )

    return image_sha256.lower(), timeout

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--github-env")
    args = parser.parse_args()

    try:
        image_sha256, timeout = validate(
            os.environ.get("RIVET_INPUT_IMAGE_SHA256", ""),
            os.environ.get("RIVET_INPUT_TIMEOUT_SECONDS", ""),
        )
    except ValueError as exc:
        raise SystemExit(str(exc))

    if args.github_env:
        path = Path(args.github_env)
        with path.open("a", encoding="utf-8") as handle:
            handle.write(f"RIVET_VALIDATED_IMAGE_SHA256={image_sha256}\n")
            handle.write(f"RIVET_VALIDATED_TIMEOUT_SECONDS={timeout}\n")
    else:
        print(f"image_sha256={image_sha256}")
        print(f"timeout_seconds={timeout}")

    return 0

if __name__ == "__main__":
    raise SystemExit(main())
