#!/usr/bin/env python3
import os
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT = "scripts/r6_validate_dispatch_inputs.py"

def run_case(hash_value, timeout_value, should_pass, marker=None):
    env = os.environ.copy()
    env["RIVET_INPUT_IMAGE_SHA256"] = hash_value
    env["RIVET_INPUT_TIMEOUT_SECONDS"] = timeout_value

    result = subprocess.run(
        [sys.executable, SCRIPT],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    if should_pass and result.returncode != 0:
        raise SystemExit(f"valid case failed: {result.stdout}")
    if not should_pass and result.returncode == 0:
        raise SystemExit(f"invalid case passed: {result.stdout}")
    if marker is not None and marker.exists():
        raise SystemExit("malicious-looking dispatch input was executed")

valid_hash = "A" * 64
run_case(valid_hash, "5400", True)
run_case(valid_hash.lower(), "300", True)
run_case(valid_hash, "6000", True)
run_case("0" * 63, "5400", False)
run_case("g" * 64, "5400", False)
run_case(valid_hash, "299", False)
run_case(valid_hash, "6001", False)
run_case(valid_hash, "-1", False)

with tempfile.TemporaryDirectory() as temporary:
    marker = Path(temporary) / "owned"
    malicious_hash = f"$(touch {marker})"
    malicious_timeout = f"$(touch {marker})"
    run_case(malicious_hash, "5400", False, marker)
    run_case(valid_hash, malicious_timeout, False, marker)

print("r6 dispatch input validator self-test: ok")
