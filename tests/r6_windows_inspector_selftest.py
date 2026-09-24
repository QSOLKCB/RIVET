#!/usr/bin/env python3
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT = "scripts/r6_validate_windows_inspector.py"

def xml(product_name, product_variant="Server", arch="x86_64", major="6", minor="3", name="windows", distro="windows"):
    return f"""<?xml version="1.0"?>
<operatingsystems>
  <operatingsystem>
    <name>{name}</name>
    <distro>{distro}</distro>
    <arch>{arch}</arch>
    <major_version>{major}</major_version>
    <minor_version>{minor}</minor_version>
    <product_name>{product_name}</product_name>
    <product_variant>{product_variant}</product_variant>
  </operatingsystem>
</operatingsystems>
"""

def run_case(root, label, text, should_pass):
    path = root / f"{label}.xml"
    path.write_text(text, encoding="utf-8")
    result = subprocess.run(
        [sys.executable, SCRIPT, str(path)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if should_pass and result.returncode != 0:
        raise SystemExit(f"{label} unexpectedly failed: {result.stdout}")
    if not should_pass and result.returncode == 0:
        raise SystemExit(f"{label} unexpectedly passed: {result.stdout}")

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)

    run_case(
        root,
        "datacenter-eval",
        xml("Windows Server 2012 R2 Datacenter Evaluation"),
        True,
    )
    run_case(
        root,
        "datacenter-eval-microsoft-prefix",
        xml("Microsoft Windows Server 2012 R2 Datacenter Evaluation"),
        True,
    )
    run_case(
        root,
        "windows-8-1",
        xml("Windows 8.1 Enterprise Evaluation", product_variant="Client"),
        False,
    )
    run_case(
        root,
        "server-standard",
        xml("Windows Server 2012 R2 Standard Evaluation"),
        False,
    )
    run_case(
        root,
        "wrong-arch",
        xml("Windows Server 2012 R2 Datacenter Evaluation", arch="i386"),
        False,
    )

print("r6 Windows inspector validator self-test: ok")
