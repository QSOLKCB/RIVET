#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

SOURCE = subprocess.check_output(
    ["git", "rev-parse", "HEAD"],
    text=True,
).strip()
E4_SOURCE = SOURCE
NONEXISTENT_SOURCE = "f" * 40
PRE_PAYLOAD_SOURCE = "7d260e0671c5d089b25d6075ab1b66fb0886c99e"
GUEST_LINE = (
    "rivet-r9-guest: target=windows9x-x86 "
    f"source={SOURCE} pointer_bits=32 endian=little "
    "document_fnv1a64=75be6cc92698ac1a "
    "source_fnv1a64=5cf7c63a1fa3d9b4 "
    "history=2 bookmarks=1 fetches=4 downloads=1"
)
WIN95_VER = "Windows 95. [Version 4.00.950]"
WIN98_VER = "Microsoft Windows 98 [Version 4.10.2222]"
XP_VER = "Microsoft Windows XP [Version 5.1.2600]"
NT4_VER = "Microsoft Windows NT [Version 4.00.1381]"
AMIGA_GUEST_LINE = (
    "rivet-r9-guest: target=amiga-m68k "
    f"source={SOURCE} pointer_bits=32 endian=big "
    "document_fnv1a64=75be6cc92698ac1a "
    "source_fnv1a64=5cf7c63a1fa3d9b4 "
    "history=2 bookmarks=1 fetches=4 downloads=1"
)
MAC_M68K_GUEST_LINE = (
    "rivet-r9-guest: target=classic-mac-m68k "
    f"source={SOURCE} pointer_bits=32 endian=big "
    "document_fnv1a64=75be6cc92698ac1a "
    "source_fnv1a64=5cf7c63a1fa3d9b4 "
    "history=2 bookmarks=1 fetches=4 downloads=1"
)
MAC_M68K_OS_LINE = (
    "rivet-r9-os: target=classic-mac-m68k "
    "os=classic-mac-os version=7.6.1 api=toolbox"
)
MAC_PPC_GUEST_LINE = (
    "rivet-r9-guest: target=classic-mac-powerpc "
    f"source={SOURCE} pointer_bits=32 endian=big "
    "document_fnv1a64=75be6cc92698ac1a "
    "source_fnv1a64=5cf7c63a1fa3d9b4 "
    "history=2 bookmarks=1 fetches=4 downloads=1"
)
MAC_PPC_OS_LINE = (
    "rivet-r9-os: target=classic-mac-powerpc "
    "os=classic-mac-os version=8.6.0 api=toolbox"
)
AMIGA_OS_LINE = (
    "rivet-r9-os: target=amiga-m68k os=amigaos "
    "exec_version=40 exec_revision=68 "
    "dos_version=40 dos_revision=3 api=exec-dos"
)

def run_e3(
    proof: Path,
    output: Path,
    check: bool = False,
    emulator: str = "qemu-system-i386 test",
    source_revision: str = SOURCE,
    guest_media_sha256: str = "1" * 64,
    payload_sha256: str = "2" * 64,
):
    return subprocess.run(
        [
            sys.executable,
            "scripts/r9_e3_receipt.py",
            "--proof", str(proof),
            "--output", str(output),
            "--target-profile", "windows9x-x86",
            "--source-revision", source_revision,
            "--emulator", emulator,
            "--guest-media-sha256", guest_media_sha256,
            "--guest-media-label", "Windows 98 SE test image",
            "--payload-sha256", payload_sha256,
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
        "source_revision": E4_SOURCE,
        "software_environment": {
            "os_name": "Classic Mac OS",
            "os_version": "7.6.1",
            "api": "Mac OS Toolbox",
        },
        "hardware": {
            "manufacturer": "Apple",
            "model": "Quadra test",
            "cpu": "68040",
            "memory_bytes": 16 * 1024 * 1024,
        },
        "browser_proof": {
            "target": "classic-mac-m68k",
            "source": E4_SOURCE,
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
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n",
            encoding="utf-8",
        )
        receipt = root / "receipt.json"
        run_e3(good, receipt, check=True)
        data = json.loads(receipt.read_text(encoding="utf-8"))
        assert data["schema"] == "rivet.retro-e3-receipt/v1"
        assert data["guest_os_identity"] == WIN98_VER

        stale_before = receipt.read_bytes()
        stale_invalid = root / "proof-stale-invalid.txt"
        stale_invalid.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=nonzero\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n",
            encoding="utf-8",
        )
        result = run_e3(stale_invalid, receipt)
        assert result.returncode != 0
        assert not receipt.exists()
        assert not receipt.with_name(
            receipt.name + ".tmp"
        ).exists()
        assert stale_before

        run_e3(good, receipt, check=True)

        win95 = root / "proof-win95.txt"
        win95.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            WIN95_VER + "\n",
            encoding="utf-8",
        )
        win95_output = root / "win95.json"
        run_e3(win95, win95_output, check=True)
        win95_data = json.loads(
            win95_output.read_text(encoding="utf-8")
        )
        assert win95_data["guest_os_identity"] == WIN95_VER

        xp = root / "proof-xp.txt"
        xp.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            XP_VER + "\n",
            encoding="utf-8",
        )
        xp_output = root / "xp.json"
        result = run_e3(xp, xp_output)
        assert result.returncode != 0
        assert not xp_output.exists()

        nt4 = root / "proof-nt4.txt"
        nt4.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            NT4_VER + "\n",
            encoding="utf-8",
        )
        nt4_output = root / "nt4.json"
        result = run_e3(nt4, nt4_output)
        assert result.returncode != 0
        assert not nt4_output.exists()

        no_ver = root / "proof-no-ver.txt"
        no_ver.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n",
            encoding="utf-8",
        )
        no_ver_output = root / "no-ver.json"
        result = run_e3(no_ver, no_ver_output)
        assert result.returncode != 0
        assert not no_ver_output.exists()

        nonzero_exit = root / "proof-nonzero-exit.txt"
        nonzero_exit.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=nonzero\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n",
            encoding="utf-8",
        )
        nonzero_exit_output = root / "nonzero-exit.json"
        result = run_e3(nonzero_exit, nonzero_exit_output)
        assert result.returncode != 0
        assert not nonzero_exit_output.exists()

        null_source_output = root / "e3-null-source.json"
        result = run_e3(
            good,
            null_source_output,
            source_revision="0" * 40,
        )
        assert result.returncode != 0
        assert not null_source_output.exists()

        pre_payload_proof = root / "proof-pre-payload-source.txt"
        pre_payload_proof.write_text(
            GUEST_LINE.replace(
                SOURCE,
                PRE_PAYLOAD_SOURCE,
            ) + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n",
            encoding="utf-8",
        )
        pre_payload_output = root / "pre-payload-source.json"
        result = run_e3(
            pre_payload_proof,
            pre_payload_output,
            source_revision=PRE_PAYLOAD_SOURCE,
        )
        assert result.returncode != 0
        assert not pre_payload_output.exists()

        ambiguous = root / "proof-multiple-lines.txt"
        ambiguous.write_text(
            GUEST_LINE + "\n" +
            (
                "rivet-r9-guest: target=amiga-m68k "
                "source=ffffffffffffffffffffffffffffffffffffffff "
                "pointer_bits=32 endian=big "
                "document_fnv1a64=0000000000000000 "
                "source_fnv1a64=0000000000000000 "
                "history=0 bookmarks=0 fetches=0 downloads=0"
            ) + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n",
            encoding="utf-8",
        )
        ambiguous_output = root / "multiple-proof-lines.json"
        result = run_e3(ambiguous, ambiguous_output)
        assert result.returncode != 0
        assert not ambiguous_output.exists()

        malformed_duplicate = root / "proof-malformed-duplicate.txt"
        malformed_duplicate.write_text(
            GUEST_LINE + "\n" +
            "rivet-r9-guest: target=amiga-m68k "
            "malformed-and-contradictory\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n",
            encoding="utf-8",
        )
        malformed_duplicate_output = (
            root / "malformed-duplicate.json"
        )
        result = run_e3(
            malformed_duplicate,
            malformed_duplicate_output,
        )
        assert result.returncode != 0
        assert not malformed_duplicate_output.exists()

        conflicting_windows = root / "proof-conflicting-windows.txt"
        conflicting_windows.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            WIN98_VER + "\n" +
            XP_VER + "\n",
            encoding="utf-8",
        )
        conflicting_windows_output = (
            root / "conflicting-windows.json"
        )
        result = run_e3(
            conflicting_windows,
            conflicting_windows_output,
        )
        assert result.returncode != 0
        assert not conflicting_windows_output.exists()

        same_line_conflict = root / "proof-same-line-windows.txt"
        same_line_conflict.write_text(
            GUEST_LINE + "\n" +
            "proof_exit=0\n" +
            "startup_stage=winstart\n" +
            XP_VER + " " + WIN98_VER + "\n",
            encoding="utf-8",
        )
        same_line_conflict_output = (
            root / "same-line-windows.json"
        )
        result = run_e3(
            same_line_conflict,
            same_line_conflict_output,
        )
        assert result.returncode != 0
        assert not same_line_conflict_output.exists()

        zero_digests_output = root / "zero-e3-digests.json"
        result = run_e3(
            good,
            zero_digests_output,
            guest_media_sha256="0" * 64,
            payload_sha256="0" * 64,
        )
        assert result.returncode != 0
        assert not zero_digests_output.exists()

        empty_emulator = root / "empty-emulator.json"
        result = run_e3(
            good,
            empty_emulator,
            emulator="   ",
        )
        assert result.returncode != 0
        assert not empty_emulator.exists()

        amiga_proof = root / "proof-amiga.txt"
        amiga_proof.write_text(
            AMIGA_GUEST_LINE + "\n" +
            AMIGA_OS_LINE + "\n",
            encoding="utf-8",
        )
        amiga_without_rom = root / "amiga-without-rom.json"
        result = subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(amiga_proof),
                "--output", str(amiga_without_rom),
                "--target-profile", "amiga-m68k",
                "--source-revision", SOURCE,
                "--emulator", "fs-uae test",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "Amiga test media",
                "--payload-sha256", "2" * 64,
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode != 0
        assert not amiga_without_rom.exists()

        amiga_cpu_only = root / "amiga-cpu-only.txt"
        amiga_cpu_only.write_text(
            AMIGA_GUEST_LINE + "\n",
            encoding="utf-8",
        )
        amiga_cpu_only_output = root / "amiga-cpu-only.json"
        result = subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(amiga_cpu_only),
                "--output", str(amiga_cpu_only_output),
                "--target-profile", "amiga-m68k",
                "--source-revision", SOURCE,
                "--emulator", "qemu-m68k-linux-user",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "linux-m68k",
                "--payload-sha256", "2" * 64,
                "--rom-sha256", "4" * 64,
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode != 0
        assert not amiga_cpu_only_output.exists()

        amiga_with_rom = root / "amiga-with-rom.json"
        subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(amiga_proof),
                "--output", str(amiga_with_rom),
                "--target-profile", "amiga-m68k",
                "--source-revision", SOURCE,
                "--emulator", "fs-uae test",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "Amiga test media",
                "--payload-sha256", "2" * 64,
                "--rom-sha256", "4" * 64,
            ],
            check=True,
        )
        assert amiga_with_rom.exists()

        mac_m68k_proof = root / "proof-mac-m68k.txt"
        mac_m68k_proof.write_text(
            MAC_M68K_GUEST_LINE + "\n" +
            MAC_M68K_OS_LINE + "\n",
            encoding="utf-8",
        )
        mac_without_rom = root / "mac-m68k-without-rom.json"
        result = subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(mac_m68k_proof),
                "--output", str(mac_without_rom),
                "--target-profile", "classic-mac-m68k",
                "--source-revision", SOURCE,
                "--emulator", "qemu-system-m68k test",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "Classic Mac test media",
                "--payload-sha256", "2" * 64,
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode != 0
        assert not mac_without_rom.exists()

        mac_system6_proof = root / "proof-mac-system6.txt"
        mac_system6_proof.write_text(
            MAC_M68K_GUEST_LINE + "\n" +
            "rivet-r9-os: target=classic-mac-m68k "
            "os=classic-mac-os version=6.0.8 api=toolbox\n",
            encoding="utf-8",
        )
        mac_system6_output = root / "mac-system6.json"
        result = subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(mac_system6_proof),
                "--output", str(mac_system6_output),
                "--target-profile", "classic-mac-m68k",
                "--source-revision", SOURCE,
                "--emulator", "QEMU q800",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "System 6.0.8 test media",
                "--payload-sha256", "2" * 64,
                "--rom-sha256", "4" * 64,
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode != 0
        assert not mac_system6_output.exists()

        mac_m68k_with_rom = root / "mac-m68k-with-rom.json"
        subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(mac_m68k_proof),
                "--output", str(mac_m68k_with_rom),
                "--target-profile", "classic-mac-m68k",
                "--source-revision", SOURCE,
                "--emulator", "QEMU q800",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "Classic Mac OS 7.6.1 test",
                "--payload-sha256", "2" * 64,
                "--rom-sha256", "4" * 64,
            ],
            check=True,
        )
        assert mac_m68k_with_rom.exists()

        mac_ppc_proof = root / "proof-mac-ppc.txt"
        mac_ppc_proof.write_text(
            MAC_PPC_GUEST_LINE + "\n" +
            MAC_PPC_OS_LINE + "\n",
            encoding="utf-8",
        )
        mac_ppc_output = root / "mac-ppc.json"
        subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(mac_ppc_proof),
                "--output", str(mac_ppc_output),
                "--target-profile", "classic-mac-powerpc",
                "--source-revision", SOURCE,
                "--emulator", "qemu-system-ppc test",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "Classic Mac OS 8.6 test",
                "--payload-sha256", "2" * 64,
            ],
            check=True,
        )
        assert mac_ppc_output.exists()

        nonexistent_ppc_proof = root / "proof-nonexistent-ppc.txt"
        nonexistent_ppc_proof.write_text(
            MAC_PPC_GUEST_LINE.replace(
                SOURCE,
                NONEXISTENT_SOURCE,
            ) + "\n" +
            MAC_PPC_OS_LINE + "\n",
            encoding="utf-8",
        )
        nonexistent_ppc_output = root / "nonexistent-ppc.json"
        result = subprocess.run(
            [
                sys.executable,
                "scripts/r9_e3_receipt.py",
                "--proof", str(nonexistent_ppc_proof),
                "--output", str(nonexistent_ppc_output),
                "--target-profile", "classic-mac-powerpc",
                "--source-revision", NONEXISTENT_SOURCE,
                "--emulator", "qemu-system-ppc test",
                "--guest-media-sha256", "1" * 64,
                "--guest-media-label", "Classic Mac OS 8.6 test",
                "--payload-sha256", "2" * 64,
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert result.returncode != 0
        assert not nonexistent_ppc_output.exists()

        bad_hash = root / "proof-bad-hash.txt"
        bad_hash.write_text(
            (
                GUEST_LINE + "\n" +
                "proof_exit=0\n" +
                "startup_stage=winstart\n" +
                WIN98_VER + "\n"
            ).replace(
                "75be6cc92698ac1a",
                "0000000000000000",
            ),
            encoding="utf-8",
        )
        bad_output = root / "bad-hash.json"
        result = run_e3(bad_hash, bad_output)
        assert result.returncode != 0
        assert not bad_output.exists()

        template = Path(
            "evidence/retro/physical-receipt.example.json"
        )
        assert validate_e4(template).returncode != 0

        e4 = root / "e4.json"
        write_json(e4, base_e4())
        validate_e4(e4, check=True)

        mc68040 = base_e4()
        mc68040["hardware"]["model"] = "Quadra 840AV"
        mc68040["hardware"]["cpu"] = "Motorola MC68040"
        mc68040_path = root / "e4-mc68040.json"
        write_json(mc68040_path, mc68040)
        validate_e4(mc68040_path, check=True)

        ppc604e = base_e4()
        ppc604e["target_profile"] = "classic-mac-powerpc"
        ppc604e["software_environment"] = {
            "os_name": "Classic Mac OS",
            "os_version": "8.6",
            "api": "Mac OS Toolbox",
        }
        ppc604e["hardware"] = {
            "manufacturer": "Apple",
            "model": "Power Macintosh 9600",
            "cpu": "PowerPC 604e",
            "memory_bytes": 128 * 1024 * 1024,
        }
        ppc604e["browser_proof"]["target"] = (
            "classic-mac-powerpc"
        )
        ppc604e_path = root / "e4-powerpc-604e.json"
        write_json(ppc604e_path, ppc604e)
        validate_e4(ppc604e_path, check=True)

        ppc603e = base_e4()
        ppc603e["target_profile"] = "classic-mac-powerpc"
        ppc603e["software_environment"] = {
            "os_name": "Classic Mac OS",
            "os_version": "8.6",
            "api": "Mac OS Toolbox",
        }
        ppc603e["hardware"] = {
            "manufacturer": "Apple",
            "model": "PowerBook 5300",
            "cpu": "Motorola PowerPC 603e",
            "memory_bytes": 64 * 1024 * 1024,
        }
        ppc603e["browser_proof"]["target"] = (
            "classic-mac-powerpc"
        )
        ppc603e_path = root / "e4-powerpc-603e.json"
        write_json(ppc603e_path, ppc603e)
        validate_e4(ppc603e_path, check=True)

        ppc7400 = base_e4()
        ppc7400["target_profile"] = "classic-mac-powerpc"
        ppc7400["software_environment"] = {
            "os_name": "Classic Mac OS",
            "os_version": "9.2.2",
            "api": "Mac OS Toolbox",
        }
        ppc7400["hardware"] = {
            "manufacturer": "Apple",
            "model": "Power Mac G4",
            "cpu": "Motorola PowerPC 7400",
            "memory_bytes": 128 * 1024 * 1024,
        }
        ppc7400["browser_proof"]["target"] = (
            "classic-mac-powerpc"
        )
        ppc7400_path = root / "e4-powerpc-7400.json"
        write_json(ppc7400_path, ppc7400)
        validate_e4(ppc7400_path, check=True)

        ppc_nonexistent_release = json.loads(
            json.dumps(ppc7400)
        )
        ppc_nonexistent_release["software_environment"][
            "os_version"
        ] = "9.99.99"
        ppc_nonexistent_release_path = (
            root / "e4-powerpc-nonexistent-release.json"
        )
        write_json(
            ppc_nonexistent_release_path,
            ppc_nonexistent_release,
        )
        assert (
            validate_e4(
                ppc_nonexistent_release_path
            ).returncode != 0
        )

        ppc_pre_floor = base_e4()
        ppc_pre_floor["target_profile"] = "classic-mac-powerpc"
        ppc_pre_floor["software_environment"] = {
            "os_name": "Classic Mac OS",
            "os_version": "7.0",
            "api": "Mac OS Toolbox",
        }
        ppc_pre_floor["hardware"] = {
            "manufacturer": "Apple",
            "model": "Power Macintosh 9600",
            "cpu": "PowerPC 604e",
            "memory_bytes": 128 * 1024 * 1024,
        }
        ppc_pre_floor["browser_proof"]["target"] = (
            "classic-mac-powerpc"
        )
        ppc_pre_floor_path = root / "e4-powerpc-pre-floor.json"
        write_json(ppc_pre_floor_path, ppc_pre_floor)
        assert validate_e4(ppc_pre_floor_path).returncode != 0

        missing_software = base_e4()
        del missing_software["software_environment"]
        missing_software_path = root / "e4-missing-software.json"
        write_json(missing_software_path, missing_software)
        assert validate_e4(missing_software_path).returncode != 0

        wrong_os = base_e4()
        wrong_os["software_environment"]["os_name"] = "AROS"
        wrong_os_path = root / "e4-wrong-os.json"
        write_json(wrong_os_path, wrong_os)
        assert validate_e4(wrong_os_path).returncode != 0

        wrong_version = base_e4()
        wrong_version["target_profile"] = "windows9x-x86"
        wrong_version["software_environment"] = {
            "os_name": "Windows 95",
            "os_version": "10.0.19045",
            "api": "Win32",
        }
        wrong_version["hardware"] = {
            "manufacturer": "IBM Compatible",
            "model": "Physical test system",
            "cpu": "Intel 80486DX2",
            "memory_bytes": 64 * 1024 * 1024,
        }
        wrong_version["browser_proof"]["target"] = (
            "windows9x-x86"
        )
        wrong_version["browser_proof"]["endian"] = "little"
        wrong_version_path = root / "e4-wrong-version.json"
        write_json(wrong_version_path, wrong_version)
        assert validate_e4(wrong_version_path).returncode != 0

        wrong_cpu = base_e4()
        wrong_cpu["hardware"]["cpu"] = "Intel Core i9-14900K"
        wrong_cpu_path = root / "e4-wrong-cpu.json"
        write_json(wrong_cpu_path, wrong_cpu)
        assert validate_e4(wrong_cpu_path).returncode != 0

        mixed_emulated_cpu = base_e4()
        mixed_emulated_cpu["hardware"]["cpu"] = (
            "Intel Core i9-14900K running a 68040 emulator"
        )
        mixed_emulated_cpu_path = (
            root / "e4-mixed-emulated-cpu.json"
        )
        write_json(
            mixed_emulated_cpu_path,
            mixed_emulated_cpu,
        )
        assert (
            validate_e4(
                mixed_emulated_cpu_path
            ).returncode != 0
        )

        windows95 = base_e4()
        windows95["target_profile"] = "windows9x-x86"
        windows95["software_environment"] = {
            "os_name": "Windows 95",
            "os_version": "4.00.950",
            "api": "Win32",
        }
        windows95["hardware"] = {
            "manufacturer": "IBM Compatible",
            "model": "Physical 486 test system",
            "cpu": "Intel 80486DX2",
            "memory_bytes": 64 * 1024 * 1024,
        }
        windows95["browser_proof"]["target"] = "windows9x-x86"
        windows95["browser_proof"]["endian"] = "little"
        windows95_path = root / "e4-windows95.json"
        write_json(windows95_path, windows95)
        validate_e4(windows95_path, check=True)

        windows95_386 = base_e4()
        windows95_386["target_profile"] = "windows9x-x86"
        windows95_386["software_environment"] = {
            "os_name": "Windows 95",
            "os_version": "4.00.950",
            "api": "Win32",
        }
        windows95_386["hardware"] = {
            "manufacturer": "Intel",
            "model": "Physical 386 test system",
            "cpu": "Intel 80386DX",
            "memory_bytes": 16 * 1024 * 1024,
        }
        windows95_386["browser_proof"]["target"] = (
            "windows9x-x86"
        )
        windows95_386["browser_proof"]["endian"] = "little"
        windows95_386_path = root / "e4-windows95-386.json"
        write_json(windows95_386_path, windows95_386)
        validate_e4(windows95_386_path, check=True)

        windows98_386 = base_e4()
        windows98_386["target_profile"] = "windows9x-x86"
        windows98_386["software_environment"] = {
            "os_name": "Windows 98",
            "os_version": "4.10.2222",
            "api": "Win32",
        }
        windows98_386["hardware"] = dict(
            windows95_386["hardware"]
        )
        windows98_386["browser_proof"]["target"] = (
            "windows9x-x86"
        )
        windows98_386["browser_proof"]["endian"] = "little"
        windows98_386_path = root / "e4-windows98-386.json"
        write_json(windows98_386_path, windows98_386)
        assert validate_e4(windows98_386_path).returncode != 0

        windows_me_386 = base_e4()
        windows_me_386["target_profile"] = "windows9x-x86"
        windows_me_386["software_environment"] = {
            "os_name": "Windows Me",
            "os_version": "4.90.3000",
            "api": "Win32",
        }
        windows_me_386["hardware"] = dict(
            windows95_386["hardware"]
        )
        windows_me_386["browser_proof"]["target"] = (
            "windows9x-x86"
        )
        windows_me_386["browser_proof"]["endian"] = "little"
        windows_me_386_path = root / "e4-windows-me-386.json"
        write_json(windows_me_386_path, windows_me_386)
        assert validate_e4(windows_me_386_path).returncode != 0

        windows_me_pentium = base_e4()
        windows_me_pentium["target_profile"] = "windows9x-x86"
        windows_me_pentium["software_environment"] = {
            "os_name": "Windows Me",
            "os_version": "4.90.3000",
            "api": "Win32",
        }
        windows_me_pentium["hardware"] = {
            "manufacturer": "Intel",
            "model": "Physical Pentium test system",
            "cpu": "Intel Pentium",
            "memory_bytes": 64 * 1024 * 1024,
        }
        windows_me_pentium["browser_proof"]["target"] = (
            "windows9x-x86"
        )
        windows_me_pentium["browser_proof"]["endian"] = "little"
        windows_me_pentium_path = (
            root / "e4-windows-me-pentium.json"
        )
        write_json(
            windows_me_pentium_path,
            windows_me_pentium,
        )
        validate_e4(windows_me_pentium_path, check=True)

        windows_me_low_memory = json.loads(
            json.dumps(windows_me_pentium)
        )
        windows_me_low_memory["hardware"]["memory_bytes"] = 1
        windows_me_low_memory_path = (
            root / "e4-windows-me-low-memory.json"
        )
        write_json(
            windows_me_low_memory_path,
            windows_me_low_memory,
        )
        assert (
            validate_e4(
                windows_me_low_memory_path
            ).returncode != 0
        )

        windows_me_min_memory = json.loads(
            json.dumps(windows_me_pentium)
        )
        windows_me_min_memory["hardware"]["memory_bytes"] = (
            32 * 1024 * 1024
        )
        windows_me_min_memory_path = (
            root / "e4-windows-me-min-memory.json"
        )
        write_json(
            windows_me_min_memory_path,
            windows_me_min_memory,
        )
        validate_e4(windows_me_min_memory_path, check=True)

        amd_am486 = base_e4()
        amd_am486["target_profile"] = "windows9x-x86"
        amd_am486["software_environment"] = {
            "os_name": "Windows 95",
            "os_version": "4.00.950",
            "api": "Win32",
        }
        amd_am486["hardware"] = {
            "manufacturer": "AMD",
            "model": "Physical Am486 test system",
            "cpu": "AMD Am486DX4",
            "memory_bytes": 64 * 1024 * 1024,
        }
        amd_am486["browser_proof"]["target"] = "windows9x-x86"
        amd_am486["browser_proof"]["endian"] = "little"
        amd_am486_path = root / "e4-amd-am486.json"
        write_json(amd_am486_path, amd_am486)
        validate_e4(amd_am486_path, check=True)

        itanium = base_e4()
        itanium["target_profile"] = "windows9x-x86"
        itanium["software_environment"] = {
            "os_name": "Windows 95",
            "os_version": "4.00.950",
            "api": "Win32",
        }
        itanium["hardware"] = {
            "manufacturer": "HP",
            "model": "Integrity rx2600",
            "cpu": "Intel Itanium 2",
            "memory_bytes": 1024 * 1024 * 1024,
        }
        itanium["browser_proof"]["target"] = "windows9x-x86"
        itanium["browser_proof"]["endian"] = "little"
        itanium_path = root / "e4-itanium.json"
        write_json(itanium_path, itanium)
        assert validate_e4(itanium_path).returncode != 0

        relabeled = base_e4()
        relabeled["target_profile"] = "amiga-m68k"
        relabeled_path = root / "e4-relabeled.json"
        write_json(relabeled_path, relabeled)
        assert validate_e4(relabeled_path).returncode != 0

        null_source = base_e4()
        null_source["source_revision"] = "0" * 40
        null_source["browser_proof"]["source"] = "0" * 40
        null_source_path = root / "e4-null-source.json"
        write_json(null_source_path, null_source)
        assert validate_e4(null_source_path).returncode != 0

        nonexistent_source = base_e4()
        nonexistent_source["source_revision"] = "f" * 40
        nonexistent_source["browser_proof"]["source"] = "f" * 40
        nonexistent_source_path = root / "e4-nonexistent-source.json"
        write_json(nonexistent_source_path, nonexistent_source)
        assert validate_e4(nonexistent_source_path).returncode != 0

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

        zero_digest = base_e4()
        zero_digest["attachments"][0]["sha256"] = "0" * 64
        zero_digest_path = root / "e4-zero-digest.json"
        write_json(zero_digest_path, zero_digest)
        assert validate_e4(zero_digest_path).returncode != 0

        sentinel_name = base_e4()
        sentinel_name["attachments"][0]["name"] = (
            "REPLACE-WITH-EVIDENCE-FILENAME"
        )
        sentinel_name_path = root / "e4-sentinel-name.json"
        write_json(sentinel_name_path, sentinel_name)
        assert validate_e4(sentinel_name_path).returncode != 0

        sentinel_model = base_e4()
        sentinel_model["hardware"]["model"] = (
            "REPLACE-WITH-EXACT-MODEL"
        )
        sentinel_model_path = root / "e4-sentinel-model.json"
        write_json(sentinel_model_path, sentinel_model)
        assert validate_e4(sentinel_model_path).returncode != 0

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

        duplicate_top = root / "e4-duplicate-top.json"
        duplicate_top_text = json.dumps(
            base_e4(),
            separators=(",", ":"),
        )
        duplicate_top_text = duplicate_top_text.replace(
            '"result":"pass"',
            '"result":"fail","result":"pass"',
            1,
        )
        duplicate_top.write_text(
            duplicate_top_text + "\n",
            encoding="utf-8",
        )
        assert validate_e4(duplicate_top).returncode != 0

        duplicate_nested = root / "e4-duplicate-nested.json"
        duplicate_nested_text = json.dumps(
            base_e4(),
            separators=(",", ":"),
        )
        duplicate_nested_text = duplicate_nested_text.replace(
            '"model":"Quadra test"',
            '"model":"Wrong model","model":"Quadra test"',
            1,
        )
        duplicate_nested.write_text(
            duplicate_nested_text + "\n",
            encoding="utf-8",
        )
        assert validate_e4(duplicate_nested).returncode != 0

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
                "RIVET_INPUT_DOS_DRIVE": "d:",
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
        assert "RIVET_VALIDATED_DOS_DRIVE=D:" in env_text
        assert not marker.exists()

    print("r9 E3/E4 receipt self-test: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
