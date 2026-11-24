---
name: fm1-release-gate
description: Read-only safety check of an AMB+ FM1 release artifact against the owner’s real flash-chip identity, two backups, recovery plan and test evidence. Never installs firmware.
---

# Human hardware release review
First apply the changelog and diff review in AGENTS.md, “Choose release checks from the changelog”. Use the extended checks below when relevant to the change. For a lightweight package, record build and checksum evidence, attribute prior evidence to its version and leave hardware signoff to the owner.

1. Inspect `SAFE_FLASH_GATES.md`, `MILESTONE_TEST_CARD.md`, pinned source revision and artifact hash.
2. Obtain *actual* JEDEC flash-chip ID, complete read-only backup files and checksums, exported settings, compatible known-good firmware, and documented recovery route for the owner's Mac/device.
3. Inspect flash-region changes, installer/loader compatibility, native rendering, stored-state migration, CPU/RAM and host/regression results. Do not assume emulator pass means flash-safe.
4. Return **NO FLASH** if evidence is missing, device support is uncertain, or any regression is unresolved.
5. Otherwise return **READY FOR HUMAN DECISION — NOT FLASHED**, including specific low-volume acceptance tests and rollback instructions. Owner personally authorizes a specific binary.

Never invoke an installer, erase/repartition/bootloader command, recovery command or a physical device action.
