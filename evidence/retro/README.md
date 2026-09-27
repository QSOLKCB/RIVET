# R9 Retro evidence retention

This directory is for retained target-specific evidence, not guest operating-system media.

Do not commit Windows, Classic Mac OS, AmigaOS/Workbench, Kickstart ROM, Macintosh ROM, or other proprietary media.

## E3

Full-system workflows upload `rivet.retro-e3-receipt/v1` artifacts only after the guest itself writes a source-bound WEB1 proof that passes `scripts/r9_e3_receipt.py`.

A harness-ready target remains unclaimed until that passing receipt is retained for the source revision being released.

## E4

Physical hardware evidence uses `rivet.retro-e4-receipt/v1`.

Copy `physical-receipt.example.json`, fill in real hardware identity and source revision, add one or more SHA-256-bound photos/logs/captures as external or retained evidence, and validate with:

```sh
python3 scripts/r9_validate_physical_receipt.py path/to/receipt.json
```

A physical receipt is evidence for the exact hardware/OS configuration named in that receipt. It is not a universal family claim.
