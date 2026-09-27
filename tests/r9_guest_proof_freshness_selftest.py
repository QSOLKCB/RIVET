#!/usr/bin/env python3
import json
import re
from pathlib import Path

WINDOWS = Path("scripts/r9_windows9x_full_system.sh")
WINDOWS_BUILD = Path("scripts/r9_build_windows9x_payload.sh")
WINDOWS_PROOF = Path("evidence/r9_win9x_browser_proof.c")
WINDOWS_WORKFLOW = Path(".github/workflows/r9-windows9x.yml")
TARGET_WORKFLOW = Path(".github/workflows/r9-target-harnesses.yml")
TRUST_WORKFLOW = Path(".github/workflows/r9-trust-anchor.yml")
MAC = Path("scripts/r9_classic_mac_full_system.sh")
AMIGA = Path("scripts/r9_amiga_full_system.sh")
AMIGA_WORKFLOW = Path(".github/workflows/r9-amiga.yml")
RETRO_MACHINE = Path("machine/retro-v2.json")

def require_before(text: str, first: str, second: str, label: str):
    first_at = text.find(first)
    second_at = text.find(second)
    if first_at < 0:
        raise SystemExit(f"{label}: missing {first!r}")
    if second_at < 0:
        raise SystemExit(f"{label}: missing {second!r}")
    if first_at >= second_at:
        raise SystemExit(
            f"{label}: {first!r} must occur before {second!r}"
        )

def main() -> int:
    windows = WINDOWS.read_text(encoding="utf-8")
    require_before(
        windows,
        "rm-f /RIVET-R9/RECEIPT.TXT",
        'timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"',
        "Windows freshness",
    )
    require_before(
        windows,
        "CALL %s\\\\RIVET-R9\\\\RUN-R9.BAT",
        'cat "$WINSTART_ORIGINAL" >> "$WINSTART"',
        "Windows startup ordering",
    )
    if '>> "$WINSTART"' in windows and (
        "CALL %s\\\\RIVET-R9\\\\RUN-R9.BAT" in windows[
            windows.find('cat "$WINSTART_ORIGINAL" >> "$WINSTART"') :
        ]
    ):
        raise SystemExit(
            "Windows startup proof call must not be appended after "
            "the original WINSTART contents"
        )

    if "%ERRORLEVEL%" in windows:
        raise SystemExit(
            "Windows proof exit capture must not use "
            "%ERRORLEVEL% under COMMAND.COM"
        )
    if "IF ERRORLEVEL 1 GOTO RIVET_FAIL" not in windows:
        raise SystemExit(
            "Windows proof exit capture must use "
            "IF ERRORLEVEL-compatible syntax"
        )
    require_before(
        windows,
        "IF ERRORLEVEL 1 GOTO RIVET_FAIL",
        "ECHO proof_exit=0",
        "Windows proof exit ordering",
    )

    if "C:\\RIVET-R9" in windows:
        raise SystemExit(
            "Windows harness must not hard-code the C: guest drive"
        )
    if 'DOS_DRIVE="${DOS_DRIVE^^}"' not in windows:
        raise SystemExit(
            "Windows harness must normalize an explicit DOS drive"
        )

    windows_proof = WINDOWS_PROOF.read_text(encoding="utf-8")
    if '#define RECEIPT_PATH "RECEIPT.TXT"' not in windows_proof:
        raise SystemExit(
            "Windows proof receipt path must be relative to the "
            "selected DOS drive working directory"
        )
    if "C:\\RIVET-R9" in windows_proof:
        raise SystemExit(
            "Windows proof source must not hard-code the C: drive"
        )

    windows_workflow = WINDOWS_WORKFLOW.read_text(
        encoding="utf-8"
    )
    for required in (
        "RIVET_INPUT_DOS_DRIVE:",
        "$RIVET_VALIDATED_DOS_DRIVE",
    ):
        if required not in windows_workflow:
            raise SystemExit(
                "Windows workflow must retain validated DOS drive "
                "routing: " + required
            )

    windows_build = WINDOWS_BUILD.read_text(encoding="utf-8")
    for required in (
        "-nostdlib",
        "evidence/r9_win9x_browser_proof.c",
        "payload-imports.txt",
        "MSVCRT|UCRTBASE|api-ms-win-crt",
    ):
        if required not in windows_build:
            raise SystemExit(
                "Windows 95 payload must retain CRT-free build guard: "
                + required
            )

    if "$QEMU_STATUS -ne 137" not in windows:
        raise SystemExit(
            "Windows harness must accept timeout kill-after status 137"
        )

    target_workflow = TARGET_WORKFLOW.read_text(
        encoding="utf-8"
    )
    for forbidden in (
        "check_authority_blob",
        "frozen-r9-v1:",
        "git diff --name-only --no-renames",
    ):
        if forbidden in target_workflow:
            raise SystemExit(
                "Candidate workflow must not self-authorize the "
                "immutable boundary: " + forbidden
            )

    trust_workflow = TRUST_WORKFLOW.read_text(encoding="utf-8")
    for required in (
        "pull_request_target:",
        "github.event.pull_request.head.sha",
        "candidate",
        "RETRO-v2.md",
        "machine/retro-v2.json",
        "machine/project-v12.json",
        ".github/workflows/r9-trust-anchor.yml",
        "git -C candidate diff --name-only --no-renames",
    ):
        if required not in trust_workflow:
            raise SystemExit(
                "Base trust workflow is missing invariant: " + required
            )

    for forbidden in (
        "PR_NUMBER",
        "github.event.pull_request.number",
    ):
        if forbidden in trust_workflow:
            raise SystemExit(
                "Base trust workflow must enforce the frozen-surface "
                "scan for every non-maintenance PR: " + forbidden
            )

    machine = json.loads(RETRO_MACHINE.read_text(encoding="utf-8"))
    emulator_patterns = machine["e3_emulator_identity"]["target_patterns"]
    emulator_samples = {
        "windows9x-x86": "qemu-system-i386 test",
        "classic-mac-m68k": "qemu-system-m68k test",
        "classic-mac-powerpc": "qemu-system-ppc test",
        "amiga-m68k": "fs-uae test",
    }
    for target, sample in emulator_samples.items():
        pattern = emulator_patterns[target]
        if re.match(pattern, sample, re.IGNORECASE) is None:
            raise SystemExit(
                "Machine emulator regex disagrees with validator for "
                + target
            )

    mac = MAC.read_text(encoding="utf-8")
    require_before(
        mac,
        'hdel ":RIVET-R9-RECEIPT.TXT"',
        'timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"',
        "Classic Mac freshness",
    )

    if "$QEMU_STATUS -ne 137" not in mac:
        raise SystemExit(
            "Classic Mac harness must accept timeout "
            "kill-after status 137"
        )

    for required in (
        'hls ":RIVET-R9-RECEIPT.TXT"',
        'hdel ":RIVET-R9-RECEIPT.TXT"',
        'hcopy -m "$PAYLOAD" ":RIVETR9"',
        'hcopy -t ":RIVET-R9-RECEIPT.TXT" "$PROOF"',
    ):
        if required not in mac:
            raise SystemExit(
                "Classic Mac HFS operand must be colon-qualified: "
                + required
            )
    for forbidden in (
        'hcopy -m "$PAYLOAD" "RIVETR9"',
        'hcopy -t "RIVET-R9-RECEIPT.TXT" "$PROOF"',
    ):
        if forbidden in mac:
            raise SystemExit(
                "Classic Mac HFS operand lost its explicit colon: "
                + forbidden
            )

    amiga = AMIGA.read_text(encoding="utf-8")
    require_before(
        amiga,
        "delete RIVET-R9-RECEIPT.TXT",
        'timeout --signal=TERM --kill-after=20 "$TIMEOUT_SECONDS"',
        "Amiga freshness",
    )

    if "$FSUAE_STATUS -ne 137" not in amiga:
        raise SystemExit(
            "Amiga harness must accept timeout kill-after status 137"
        )

    amiga_workflow = AMIGA_WORKFLOW.read_text(
        encoding="utf-8"
    )
    if "xvfb-run -a bash scripts/r9_amiga_full_system.sh" not in (
        amiga_workflow
    ):
        raise SystemExit(
            "Amiga workflow must execute FS-UAE harness under Xvfb"
        )
    if "xauth xvfb" not in amiga_workflow:
        raise SystemExit(
            "Amiga workflow must install Xvfb and xauth"
        )

    print("r9 guest proof freshness self-test: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
