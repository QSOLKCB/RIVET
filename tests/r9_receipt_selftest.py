#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
from pathlib import Path

PLATFORM = (
    "rivet-r5: backend=posix-v1 os=POSIX pointer_bits=32 "
    "endian=big bytes=237 empty=0 fnv1a64=36aaff7f4aaa99ab "
    "monotonic=nondecreasing\n"
)

BROWSER = (
    "rivet-r8: document_fnv1a64=75be6cc92698ac1a "
    "source_fnv1a64=5cf7c63a1fa3d9b4 history=2 bookmarks=1 "
    "fetches=4 downloads=1 browser_state_bytes=700 "
    "proof_resident_bytes=60000 ppm=proof.ppm\n"
)

def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        platform = root / "platform.txt"
        browser = root / "browser.txt"
        receipt = root / "receipt.json"
        platform.write_text(PLATFORM, encoding="utf-8")
        browser.write_text(BROWSER, encoding="utf-8")

        subprocess.run(
            [
                sys.executable,
                "scripts/r9_receipt.py",
                "--platform-proof",
                str(platform),
                "--browser-proof",
                str(browser),
                "--output",
                str(receipt),
                "--source-revision",
                "0123456789abcdef0123456789abcdef01234567",
                "--compiler",
                "m68k-linux-gnu-gcc test",
                "--emulator",
                "qemu-m68k test",
            ],
            check=True,
        )

        data = json.loads(receipt.read_text(encoding="utf-8"))
        assert data["schema"] == "rivet.retro-receipt/v1"
        assert data["target_profile"] == "linux-m68k32-68020-qemu-user"
        assert data["evidence_class"] == "E2"
        assert data["pointer_bits"] == 32
        assert data["endian"] == "big"
        assert "classic-mac-os" in data["non_claims"]
        assert "amigaos" in data["non_claims"]
        assert data["browser_proof"]["document_fnv1a64"] == (
            "75be6cc92698ac1a"
        )

        bad = root / "bad-browser.txt"
        bad.write_text(
            BROWSER.replace(
                "75be6cc92698ac1a",
                "0000000000000000",
            ),
            encoding="utf-8",
        )
        result = subprocess.run(
            [
                sys.executable,
                "scripts/r9_receipt.py",
                "--platform-proof",
                str(platform),
                "--browser-proof",
                str(bad),
                "--output",
                str(root / "bad.json"),
                "--source-revision",
                "0123456789abcdef0123456789abcdef01234567",
                "--compiler",
                "compiler",
                "--emulator",
                "emulator",
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode != 0
        assert not (root / "bad.json").exists()

    print("r9 receipt self-test: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
