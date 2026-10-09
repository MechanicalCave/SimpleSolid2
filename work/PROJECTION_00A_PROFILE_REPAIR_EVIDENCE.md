# Projection 00A — Existing Profile/Extrude Repair Evidence

**Status:** Automated semantic evidence A0–A7 PASS on two exact-head Windows FOCUSED runs; broader FAST / final FULL / Owner practical GUI A8 outstanding as of this record.
**Date:** 2026-10-09
**Authority:** Owner-accepted `work/PROJECTION_00A_PROFILE_REPAIR_EVIDENCE_CONTRACT.md`
**Baseline:** `main` `636a2989b28fc3723e6cde0130e654bf12c17657`
**PR:** [#299](https://github.com/MechanicalCave/SimpleSolid2/pull/299)
**Product changes:** none; existing test fixture expansion only.

## Exact test evidence

- [Windows FOCUSED #1910](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890070002), exact SHA `1041097855e3c8cf5133c36cbca370bd85745cea`: **PASS 1/1** `pm01c.feature_commands`, Kernel Release on the existing warm `D:\SS2Build\kernel-release` tree. This tightened the test to **require** the full successful A2–A4 path, no early return/skipped branch masquerading as PASS.
- [Windows FOCUSED #1912](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890452599), exact SHA `3f2a199a812da6000f9f22d525ab3da9f6b1c6ae`: **PASS 1/1** `pm01g.feature_cold_persistence`, with independent fresh `PartDocumentStore::load` and fresh deterministic `ColdKernel` after Save/Close.
- The test-only fixture in #1911 failed to compile on `const PartDocument` copied into a session (`C2280`). Fixed ownership transfer via `std::move`; #1912 PASS. Previous #1907/#1908 failed documentation validator due to boldface rather than plain literal required `Internal docs:` declaration; fixed and later #1909/#1910 PASS. None of these initial RED findings required a product-source change.
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

Next: run Draft **FAST** for the additional invalid-candidate negative and cross-suite regression without a focus trailer. Once green, run **one** Ready-for-Review exact-head FULL (Core + native kernel + desktop + docs) before formal package closeout / squash merge. No clean FULL during iteration; keep CMake/CI unchanged.

## Documentation impact

No product behavior changed. Internal and user-facing current-state documentation are unchanged by this evidence/test-only package. PL/EN Product docs and generated Browser are mandatory for a separately Owner-approved shipping Project Geometry package.
