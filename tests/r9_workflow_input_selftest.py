#!/usr/bin/env python3
from pathlib import Path

WORKFLOWS = (
    Path(".github/workflows/r9-windows9x.yml"),
    Path(".github/workflows/r9-classic-mac.yml"),
    Path(".github/workflows/r9-amiga.yml"),
)

def run_blocks(text: str):
    lines = text.splitlines()
    index = 0
    while index < len(lines):
        line = lines[index]
        stripped = line.lstrip()
        if stripped.startswith("run:"):
            indent = len(line) - len(stripped)
            block = [line]
            index += 1
            while index < len(lines):
                candidate = lines[index]
                candidate_stripped = candidate.lstrip()
                candidate_indent = len(candidate) - len(candidate_stripped)
                if candidate_stripped and candidate_indent <= indent:
                    break
                block.append(candidate)
                index += 1
            yield "\n".join(block)
            continue
        index += 1

def main() -> int:
    for path in WORKFLOWS:
        text = path.read_text(encoding="utf-8")
        for block in run_blocks(text):
            if "${{ inputs." in block:
                raise SystemExit(
                    f"{path}: workflow-dispatch input embedded in run source"
                )

    print("r9 workflow input routing self-test: ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
