# Projection 00A — Existing Profile/Extrude Repair Evidence

**Status:** COMPLETED — Owner final Projection 00A PASS 2026-10-09; Core 25/25 + native 57/57 + Desktop 113/113 = FULL #1915 195/195 PASS, docs and aggregate PASS; A8 practical UI separately NOT CLAIMED.
**Date:** 2026-10-09
**Authority:** Owner-accepted `work/PROJECTION_00A_PROFILE_REPAIR_EVIDENCE_CONTRACT.md`
**Baseline:** `main` `636a2989b28fc3723e6cde0130e654bf12c17657`
**PR:** [#299](https://github.com/MechanicalCave/SimpleSolid2/pull/299)
**Product changes:** none; existing test fixture expansion only.

## Exact test evidence

- [Windows FOCUSED #1910](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890070002), exact SHA `1041097855e3c8cf5133c36cbca370bd85745cea`: **PASS 1/1** `pm01c.feature_commands`, Kernel Release on the existing warm `D:\SS2Build\kernel-release` tree. This tightened the test to **require** the full successful A2–A4 path, no early return/skipped branch masquerading as PASS.
- [Windows FOCUSED #1912](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890452599), exact SHA `3f2a199a812da6000f9f22d525ab3da9f6b1c6ae`: **PASS 1/1** `pm01g.feature_cold_persistence`, with independent fresh `PartDocumentStore::load` and fresh deterministic `ColdKernel` after Save/Close.
- The test-only fixture in #1911 failed to compile on `const PartDocument` copied into a session (`C2280`). Fixed ownership transfer via `std::move`; #1912 PASS. Previous #1907/#1908 failed documentation validator due to boldface rather than plain literal required `Internal docs:` declaration; fixed and later #1909/#1910 PASS. None of these initial RED findings required a product-source change.
- [Windows FAST #1913](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890618543), exact SHA `48fda02317628f10d93e843627fb10cb4f7f66b6`: **PASS 93/93**, `windows-msvc-fast` and aggregate `windows-msvc` PASS, includes strict invalid `EditExtrudeFeatureCommand` ProfileId negative (mutation-free) and both A0–A7 fixture tests.
- The tests use **deterministic fake `ISolidModelingKernel` implementations** and the real SS2 Domain/Application Commands, Profile/Sketch region analyzer, document store and evaluator. These results are NOT claims about the correctness of OCCT projected-curve algorithms, complex face-boundary selection, or native GUI.

## Scenario matrix

| Case | Result | Specific checked behavior |
| --- | --- | --- |
| A0 | PASS #1910 | four-line Rectangle → durable Sketch/Profile/Extrude IDs and UpToDate Body |
| A1 | PASS #1910 | change all four Line endpoint positions using `UpdateSketchLinesCommand` while preserving EntityIds and valid Profile; Undo/Redo + UpToDate |
| A2 | PASS #1910 | erase one old boundary EntityId and author two replacement Lines; existing Profile is `missing_source_entity`, Body cannot remain UpToDate, authored Profile and Feature survive |
| A3 | PASS #1910 | new five-line Region analyzed; `ReplaceProfileRegionIntentCommand` repairs SAME ProfileId; SAME Extrude FeatureId recomputes UpToDate; Undo/Redo restores semantics |
| A4 | PASS #1910 | alternate Profile created, original Extrude edited to consume it, stable FeatureId, Undo/Redo; separate invalid reassignment check added for FAST |
| A5 | PASS #1912 | save repaired Base + dependent Cut, true cold load, same ProfileId/FeatureIds, five-Line geometry, two fresh Kernel evaluations, valid current Body |
| A6 | PASS #1910 | geometrically equivalent replacement under new EntityIds is never silently rebound to old Profile; explicit repair required |
| A7 | PASS #1912 | deleted source Line makes base and later Cut not UpToDate and Body unavailable; explicit Profile repair regenerates both without replaying stale Body |
| A8 | NOT CLAIMED | real Qt Workbench selection/Edit Profile/Edit Extrude interaction is Owner practical validation for later delivery scope |

## Consequences for Project Geometry product design

1. The existing strict `ProfileRegionIntent` and `ReplaceProfileRegionIntentCommand` already support a valid **manual repair** path when Sketch region membership changes from four to five lines.
2. Stable `ProfileId`/`FeatureId` survive the repair and subsequent cold Save/Reopen. Therefore no second general Profile/Extrude architecture should be introduced just for Projection.
3. After missing/deleted semantic EntityId, dependents fail closed; new geometry with matching coordinates is **not** automatic source identity. Never auto-add new linked Sketch entities on a change in Face boundary membership during v1 recompute.
4. None of these tests prove associative projection semantics, imported SS1 binding compatibility, plane projection geometry or production/UI; these need separate Work Contract and actual Kernel/Viewer acceptance.
5. Retain one right-side Project Geometry panel for Edge and planar Face, linked Regular/Construction with Break Link, exact Line/Circle/Arc projection and visible per-unsupported-edge partial capture per Owner-approved D2. Future Assembly remains snapshot by Foundation CORE §7.3.

## CI / evidence closure

**FAST #1913 PASS 93/93** plus evidence-sync FAST #1914 PASS 93/93. **Ready-for-Review exact-head FULL #1915 PASS 195/195** on `efcba9ad4d436b40d533510dc076538a94d4392e`, documentation and aggregate PASS. **PR #299 squash-merged to `main` as `2cda8e04d7d0e2594edae7dc90e88cbb90dcae35`.** Owner explicitly accepted full Projection 00A PASS. The evidence gate is closed; A8 practical UI remains separate.

## Documentation impact

No product behavior changed. Internal and user-facing current-state documentation are unchanged by this evidence/test-only package. PL/EN Product docs and generated Browser are mandatory for a separately Owner-approved shipping Project Geometry package.

## Owner final acceptance and merge — 2026-10-09

Owner's **`Zatwierdzam 00A - kontynuuj`** is final package acceptance for the defined A0–A7 semantic evidence scope. [Windows FULL #1915](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890913601) passed **195/195**, docs and aggregate on exact PR HEAD `efcba9ad4d436b40d533510dc076538a94d4392e`; guarded [PR #299](https://github.com/MechanicalCave/SimpleSolid2/pull/299) squash merge is `2cda8e04d7d0e2594edae7dc90e88cbb90dcae35`. Real GUI A8 was not performed and is not asserted. A next Project Geometry contract must be independently activated.
