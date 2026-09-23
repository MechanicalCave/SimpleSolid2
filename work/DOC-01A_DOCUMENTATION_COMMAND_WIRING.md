# DOC-01A — Documentation Command Wiring Hotfix

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related baseline:** `work/DOC-01_AS_BUILT_DOCUMENTATION_SYSTEM.md`

## Goal

Fix the root `ss2.ps1 docs` command wiring introduced by DOC-01 and add regression coverage for the exact user-facing dispatcher path.

## Scope IN

- Add `docs -> scripts\ss2-docs.ps1` to the root `ss2.ps1` command map.
- Fail closed when an accepted command has no dispatcher mapping.
- Fail closed when the mapped script file does not exist.
- Make the Windows PR gate execute the exact public repository command `.\ss2.ps1 docs`.
- After generation in CI, verify that `docs/browser/index.html` remains byte/content-clean in Git so the committed Browser is deterministic and current.
- Update internal build/test/run as-built documentation to describe this dispatcher smoke check.

## Scope OUT

- no Product Browser feature redesign,
- no user/product content changes,
- no Project/CAD behavior changes,
- no persistence, identity or Foundation changes.

## Acceptance

1. `.\ss2.ps1 docs` invokes `scripts\ss2-docs.ps1` successfully.
2. Root dispatch fails explicitly for an unmapped accepted command instead of attempting to execute the repository directory.
3. Root dispatch fails explicitly if a mapped script is missing.
4. Windows PR gate executes `.\ss2.ps1 docs` on the exact PR head.
5. CI verifies that regeneration does not change the committed Browser.
6. Documentation validation remains PASS.
7. Existing CTest suite remains PASS.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: the canonical repository documentation command and CI verification workflow change, while end-user product behavior and user workflows are unchanged.

## Decision level

D0/D1 repository tooling hotfix. No architecture, durable identity, persistence or product-domain semantics change.
