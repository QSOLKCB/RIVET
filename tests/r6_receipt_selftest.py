#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
from pathlib import Path

PROOF = (
    "rivet-r5: backend=posix-v1 os=POSIX pointer_bits=32 "
    "endian=big bytes=237 empty=0 fnv1a64=36aaff7f4aaa99ab "
    "monotonic=nondecreasing\n"
)

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    proof = root / "proof.txt"
    receipt = root / "receipt.json"
    proof.write_text(PROOF, encoding="utf-8")

    subprocess.run(
        [
            sys.executable,
            "scripts/r6_receipt.py",
            "--proof-output", str(proof),
            "--output", str(receipt),
            "--source-revision", "deadbeef",
            "--target-profile", "self-test",
            "--evidence-class", "E2",
            "--cpu", "powerpc",
            "--execution", "qemu-user",
            "--compiler", "test-compiler",
            "--emulator", "test-emulator",
            "--accelerator", "tcg",
            "--cpu-model", "Nehalem",
            "--expected-backend", "posix-v1",
            "--expected-os", "POSIX",
            "--expected-pointer-bits", "32",
            "--expected-endian", "big",
        ],
        check=True,
    )

    data = json.loads(receipt.read_text(encoding="utf-8"))
    assert data["result"] == "pass"
    assert data["proof"]["endian"] == "big"
    assert data["proof"]["empty"] == 0
    assert data["accelerator"] == "tcg"
    assert data["cpu_model"] == "Nehalem"

print("r6 receipt self-test: ok")
