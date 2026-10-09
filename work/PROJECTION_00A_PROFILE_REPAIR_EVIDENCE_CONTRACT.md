# Projection 00A — Existing Sketch / Profile / Extrude Repair Evidence Work Contract

**Status:** COMPLETED — OWNER FINAL PASS 2026-10-09; PR #299 squash-merged to main as `2cda8e04d7d0e2594edae7dc90e88cbb90dcae35`
**Program:** Part Modeling v1 v1.30 — Projection-before-PM-06 sequencing
**Baseline:** `636a2989b28fc3723e6cde0130e654bf12c17657` (PM-05F merged)
**D2 amendment:** Foundation §7.2 and §7.5 same-Part default associativity; Assembly §7.3 snapshot CORE unchanged
**Evidence design:** `work/PROJECTION_00A_PROFILE_REPAIR_EVIDENCE_PROPOSAL.md`
**Current automated evidence:** `work/PROJECTION_00A_PROFILE_REPAIR_EVIDENCE.md` — FOCUSED #1910/#1912 PASS 1/1; FAST #1913 PASS 93/93 (2026-10-09), FULL #1915 PASS 195/195 on exact PR head `efcba9ad4d436b40d533510dc076538a94d4392e`; Owner accepted final Projection 00A PASS.
**Goal:** characterize existing SS2 Sketch topology edits, semantic Profile repair, existing Extrude reuse and identity/lifecycle; **do not implement Projection**.

## Allowed edits

1. `tests/pm01c_feature_commands_test.cpp` (existing semantic Profile/Extrude test).
2. `tests/pm01g_feature_cold_persistence_test.cpp` (existing native Part persistence/cold-rebuild test), **only when needed**.
3. This contract, accepted roadmap/active-state traceability and a completion evidence report under `work/**`.

**Excluded:** `src/**`, `tests/CMakeLists.txt`, `tests/core_only/CMakeLists.txt`, `tests/kernel_native/CMakeLists.txt`, CI scripts/workflows, kernel/OCCT, CAD schema/persistence formats, UI code, shipping Projection or automatically altering captured Face membership. Any production fix found necessary is a **STOP + explicit D2/D3 remediation proposal**, not allowed under 00A.

## Required A0–A7 characterization (reuse existing command/test target)

- **A0:** create four regular Line rectangle; durable Profile001 and Extrude001 Add; record exact SketchId, EntityIds, ProfileId, FeatureId, current Body `up_to_date`.
- **A1:** edit dimensions using `UpdateSketchLinesCommand` preserving EntityIds and closed geometry; check same Profile/Extrude identity, current result, Undo/Redo.
- **A2:** replace one authored Line with two joined Lines using existing Erase/Add Sketch commands; observe whether mutation commits. If it fails, prove zero mutation and document exact reason. If it succeeds, prove original Profile intent cannot silently rebind to similar/new entity.
- **A3:** explicitly select the new region and `ReplaceProfileRegionIntentCommand` targeting **original ProfileId**. Confirm region valid and dependent Extrude recovers without replacing FeatureId; preserve fail-closed behavior if not possible.
- **A4:** optionally create another valid Profile and `EditExtrudeFeatureCommand` on the original FeatureId; compare to repairing Profile in place. No new FeatureId on Edit.
- **A5:** reproduce successful repaired branch in `pm01g.feature_cold_persistence` with Save/Close/Reopen and true fresh provider rebuild; prove serialized IDs and edited intent; Undo/Redo where supported.
- **A6:** identical geometry under a different EntityId is not automatic Profile rebinding; strict identity matters.
- **A7:** downstream Add/Cut demonstrates typed downstream failure or deliberate safe repair when upstream Profile invalid, without using stale Body.
- **A8:** GUI Workbench workflow remains a **separate future Owner manual acceptance**, not a claim made by semantic/core automated results.

Record actual behavior; if an unexpected RED occurs, keep the unweakened test as evidence and STOP for a narrowly scoped repair decision. No invented PASS.

## CI / build-time policy

- Do **not** touch any CMakeLists/test registration or build infrastructure: those changes trigger clean Windows FULL. Reuse `pm01c_feature_commands_test` / `pm01c.feature_commands` (or `pm01g_feature_cold_persistence_test` / `pm01g.feature_cold_persistence`).
- Iteration is on a **Draft** PR; commit trailer must contain **all three lines** with exact registered target/test:

```text
SS2-Focus-Target: pm01c_feature_commands_test
SS2-Focus-Test: pm01c.feature_commands
SS2-Focus-Mode: kernel
```

- Use `kernel` FOCUSED on warm `D:\SS2Build\kernel-release` tree; non-focused Draft source/test changes trigger FAST, not FULL. Once stable, run FAST and finally Ready-for-Review exact-head FULL.
- Cached build-tree fingerprint is authoritative. CMake/test graph/toolchain edits and invalid focus requests automatically fail closed to FULL. Keep CI security/quality unchanged.
- The prior proposal-only #1906 CLOSURE PASS is **not** test evidence.

## Acceptance / stop criteria

- A0–A7 behavior recorded with exact test CI, IDs, failed/resolved diagnostics and observed boundary semantics; no runtime/product code changes.
- FOCUSED (and broader FAST) PASS on exact candidate; final PR FULL PASS before any gated merge if this Work Contract closes.
- Do not claim A8 GUI PASS without explicit Owner practical test.
- Architecture conclusion: whether associative projection can reuse current Part/Profile/Extrude semantics or requires separate approved changes. No future Production Projection Work Contract auto-approved.
- Tests are not to be removed/weakened to force green.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: Evidence-only characterization, existing production behavior unchanged. When future Projection production is authorized, as-built internal and bilingual PL/EN Product docs (plus generated Browser) are mandatory.

## Formal final disposition — 2026-10-09

Owner explicitly wrote **`Zatwierdzam 00A - kontynuuj`**. Thus 00A is **COMPLETED — PASS** after real Core 25/25, kernel-native 57/57, Desktop 113/113 Windows FULL [#1915](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890913601) (195/195, docs and aggregate success) on exact PR HEAD `efcba9ad4d436b40d533510dc076538a94d4392e` and guarded squash merge [PR #299](https://github.com/MechanicalCave/SimpleSolid2/pull/299) as `2cda8e04d7d0e2594edae7dc90e88cbb90dcae35`. A8 practical Windows Workbench acceptance remains **not tested / not claimed**, outside this semantic evidence acceptance. **00A mutation authority is closed**. Future Project Geometry production requires its own exact Owner-accepted Work Contract.
