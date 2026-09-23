#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

PROOF_RE = re.compile(
    r"^rivet-r5: "
    r"backend=(?P<backend>\S+) "
    r"os=(?P<os_api>\S+) "
    r"pointer_bits=(?P<pointer_bits>[0-9]+) "
    r"endian=(?P<endian>little|big) "
    r"bytes=(?P<bytes>[0-9]+) "
    r"empty=(?P<empty>[0-9]+) "
    r"fnv1a64=(?P<fnv1a64>[0-9a-f]{16}) "
    r"monotonic=(?P<monotonic>nondecreasing)$"
)

def parse_proof(text: str) -> dict:
    for line in text.splitlines():
        match = PROOF_RE.match(line.strip())
        if match:
            data = match.groupdict()
            data["pointer_bits"] = int(data["pointer_bits"])
            data["bytes"] = int(data["bytes"])
            data["empty"] = int(data["empty"])
            return data
    raise SystemExit("R6 receipt: R5 proof line not found")

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--proof-output", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--target-profile", required=True)
    parser.add_argument("--evidence-class", required=True)
    parser.add_argument("--cpu", required=True)
    parser.add_argument("--execution", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--emulator", default="")
    parser.add_argument("--expected-backend", required=True)
    parser.add_argument("--expected-os", required=True)
    parser.add_argument("--expected-pointer-bits", type=int, required=True)
    parser.add_argument("--expected-endian", choices=["little", "big"], required=True)
    args = parser.parse_args()

    proof_text = Path(args.proof_output).read_text(encoding="utf-8")
    proof = parse_proof(proof_text)

    expected = {
        "backend": args.expected_backend,
        "os_api": args.expected_os,
        "pointer_bits": args.expected_pointer_bits,
        "endian": args.expected_endian,
        "bytes": 237,
        "empty": 0,
        "fnv1a64": "36aaff7f4aaa99ab",
        "monotonic": "nondecreasing",
    }
    for key, value in expected.items():
        if proof[key] != value:
            raise SystemExit(
                f"R6 receipt: {key} mismatch: got {proof[key]!r}, expected {value!r}"
            )

    receipt = {
        "schema": "rivet.historical-receipt/v1",
        "source_revision": args.source_revision,
        "target_profile": args.target_profile,
        "evidence_class": args.evidence_class,
        "execution": args.execution,
        "cpu": args.cpu,
        "compiler": args.compiler,
        "emulator": args.emulator or None,
        "proof": proof,
        "result": "pass",
    }

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(receipt, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(output)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
