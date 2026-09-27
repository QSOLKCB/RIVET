#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

SOURCE = "0123456789abcdef0123456789abcdef01234567"
GUEST_LINE = (
    "rivet-r9-guest: target=windows9x-x86 "
    f"source={SOURCE} pointer_bits=32 endian=little "
    "document_fnv1a64=75be6cc92698ac1a "
    "source_fnv1a64=5cf7c63a1fa3d9b4 "
    "history=2 bookmarks=1 fetches=4 downloads=1"
)
WIN98_VER = "Microsoft Windows 98 [Version 4.10.2222]"
XP_VER = "Microsoft Windows XP [Version 5.1.2600]"

def run_e3(
    proof: Path,
    output: Path,
    check: bool = False,
    emulator: str = "qemu-system-i386 test",
):
    return subprocess.run(
        [
            sys.executable,
            "scripts/r9_e3_receipt.py",
            "--proof", str(proof),
            "--output", str(output),
            "--target-profile", "windows9x-x86",
            "--source-revision", SOURCE,
            "--emulator", emulator,
            "--guest-media-sha256", "1" * 64,
            "--guest-media-label", "Windows 98 SE test image",
            "--payload-sha256", "2" * 64,
        ],
        check=check,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

def validate_e4(path: Path, check: bool = False):
    return subprocess.run(
        [
            sys.executable,
            "scripts/r9_validate_physical_receipt.py",
            str(path),
        ],
        check=check,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

def base_e4():
    return {
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
            "target": "classic-mac-m68k",
            "source": SOURCE,
            "pointer_bits": 32,
            "endian": "big",
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
    }

def write_json(path: Path, data):
    path.write_text(
        json.dumps(data, indent=2) + "\n",
        encoding="utf-8",
    )

def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)

        good = root / "proof-good.txt"
        good.write_text(
            GUEST_LINE + "\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n",
            encoding="utf-8",
        )
        receipt = root / "receipt.json"
        run_e3(good, receipt, check=True)
        data = json.loads(receipt.read_text(encoding="utf-8"))
        assert data["schema"] == "rivet.retro-e3-receipt/v1"
        assert data["guest_os_identity"] == WIN98_VER

        xp = root / "proof-xp.txt"
        xp.write_text(
            GUEST_LINE + "\n" + XP_VER + "\n",
            encoding="utf-8",
        )
        xp_output = root / "xp.json"
        result = run_e3(xp, xp_output)
        assert result.returncode != 0
        assert not xp_output.exists()

        no_ver = root / "proof-no-ver.txt"
        no_ver.write_text(GUEST_LINE + "\n", encoding="utf-8")
        no_ver_output = root / "no-ver.json"
        result = run_e3(no_ver, no_ver_output)
        assert result.returncode != 0
        assert not no_ver_output.exists()

        empty_emulator = root / "empty-emulator.json"
        result = run_e3(
            good,
            empty_emulator,
            emulator="   ",
        )
        assert result.returncode != 0
        assert not empty_emulator.exists()

        bad_hash = root / "proof-bad-hash.txt"
        bad_hash.write_text(
            (GUEST_LINE + "\n" + WIN98_VER + "\n").replace(
                "75be6cc92698ac1a",
                "0000000000000000",
            ),
            encoding="utf-8",
        )
        bad_output = root / "bad-hash.json"
        result = run_e3(bad_hash, bad_output)
        assert result.returncode != 0
        assert not bad_output.exists()

        e4 = root / "e4.json"
        write_json(e4, base_e4())
        validate_e4(e4, check=True)

        relabeled = base_e4()
        relabeled["target_profile"] = "amiga-m68k"
        relabeled_path = root / "e4-relabeled.json"
        write_json(relabeled_path, relabeled)
        assert validate_e4(relabeled_path).returncode != 0

        wrong_source = base_e4()
        wrong_source["source_revision"] = "f" * 40
        wrong_source_path = root / "e4-wrong-source.json"
        write_json(wrong_source_path, wrong_source)
        assert validate_e4(wrong_source_path).returncode != 0

        anonymous = base_e4()
        anonymous["hardware"]["manufacturer"] = ""
        anonymous["hardware"]["model"] = ""
        anonymous["hardware"]["cpu"] = ""
        anonymous_path = root / "e4-anonymous.json"
        write_json(anonymous_path, anonymous)
        assert validate_e4(anonymous_path).returncode != 0

        unnamed_attachment = base_e4()
        unnamed_attachment["attachments"][0]["name"] = ""
        unnamed_attachment_path = root / "e4-unnamed-attachment.json"
        write_json(
            unnamed_attachment_path,
            unnamed_attachment,
        )
        assert (
            validate_e4(unnamed_attachment_path).returncode != 0
        )

        boolean_counts = base_e4()
        boolean_counts["hardware"]["memory_bytes"] = True
        boolean_counts["browser_proof"]["bookmarks"] = True
        boolean_counts["browser_proof"]["downloads"] = True
        boolean_counts_path = root / "e4-boolean-counts.json"
        write_json(boolean_counts_path, boolean_counts)
        assert validate_e4(boolean_counts_path).returncode != 0

        env_path = root / "github-env.txt"
        marker = root / "should-not-exist"
        malicious_label = f"$(touch {marker})"
        env = os.environ.copy()
        env.update(
            {
                "RIVET_INPUT_MEDIA_SHA256": "a" * 64,
                "RIVET_INPUT_ROM_SHA256": "",
                "RIVET_INPUT_TIMEOUT_SECONDS": "600",
                "RIVET_INPUT_MEDIA_LABEL": malicious_label,
                "RIVET_INPUT_GUEST_PARTITION": "/dev/sda1",
                "RIVET_INPUT_WINDOWS_DIRECTORY": "/WINDOWS",
            }
        )
        subprocess.run(
            [
                sys.executable,
                "scripts/r9_validate_dispatch_inputs.py",
                "--github-env",
                str(env_path),
            ],
            check=True,
            env=env,
        )
        env_text = env_path.read_text(encoding="utf-8")
        assert f"RIVET_VALIDATED_MEDIA_LABEL={malicious_label}" in env_text
        assert not marker.exists()

    print("r9 E3/E4 receipt self-test: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
