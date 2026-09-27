#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
from pathlib import Path

SOURCE = "0123456789abcdef0123456789abcdef01234567"
GOOD = (
    "rivet-r9-guest: target=windows9x-x86 "
    f"source={SOURCE} pointer_bits=32 endian=little "
    "document_fnv1a64=75be6cc92698ac1a "
    "source_fnv1a64=5cf7c63a1fa3d9b4 "
    "history=2 bookmarks=1 fetches=4 downloads=1\n"
)

def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        proof = root / "proof.txt"
        output = root / "receipt.json"
        proof.write_text(GOOD, encoding="utf-8")

        subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(proof),
                "--output", str(output),
                "--target-profile", "windows9x-x86",
                "--source-revision", SOURCE,
                "--emulator", "qemu-system-i386 test",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "Windows 98 SE test image",
                "--payload-sha256", "2" * 64,
            ],
            check=True,
        )
        data = json.loads(output.read_text(encoding="utf-8"))
        assert data["schema"] == "rivet.retro-e3-receipt/v1"
        assert data["browser_proof"]["document_fnv1a64"] == "75be6cc92698ac1a"

        bad = root / "bad.txt"
        bad.write_text(
            GOOD.replace("75be6cc92698ac1a", "0000000000000000"),
            encoding="utf-8",
        )
        result = subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(bad),
                "--output", str(root / "bad.json"),
                "--target-profile", "windows9x-x86",
                "--source-revision", SOURCE,
                "--emulator", "qemu",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "test",
                "--payload-sha256", "2" * 64,
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode != 0
        assert not (root / "bad.json").exists()

        e4 = root / "e4.json"
        e4.write_text(
            json.dumps(
                {
                    "schema": "rivet.retro-e4-receipt/v1",
                    "evidence_class": "E4",
                    "execution": "physical-hardware",
                    "target_profile": "classic-mac-m68k",
                    "source_revision": SOURCE,
                    "hardware": {
                        "manufacturer": "Apple",
                        "model": "Quadra test",
                        "cpu": "68040",
                        "memory_bytes": 16 * 1024 * 1024,
                    },
                    "browser_proof": {
                        "document_fnv1a64": "75be6cc92698ac1a",
                        "source_fnv1a64": "5cf7c63a1fa3d9b4",
                        "history": 2,
                        "bookmarks": 1,
                        "fetches": 4,
                        "downloads": 1,
                    },
                    "attachments": [
                        {"name": "receipt-photo.jpg", "sha256": "3" * 64}
                    ],
                    "result": "pass",
                },
                indent=2,
            ) + "\n",
            encoding="utf-8",
        )
        subprocess.run(
            [sys.executable, "scripts/r9_validate_physical_receipt.py", str(e4)],
            check=True,
        )

    print("r9 E3/E4 receipt self-test: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
