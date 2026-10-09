# PG-01B — Associative Sketch implementation evidence

**Status:** ACTIVE — B1/v15 PASS; B2 exact effective Sketch PASS; B3 ordered Extrude/Revolve, real native linked Cut and Edit Extrude draft preview PASS; B4 atomic multi-Edge authoring, current Break Link, linked geometry/structural edit guards and cold two-Edge batch PASS. B5 exhaustive dependent-feature cold/regression, final exact-head FULL and Owner sign-off OPEN.
**Current tested source:** `94e62cf4a9c8edbdfebabbc4548a75822c028ec0` (draft [PR #301](https://github.com/MechanicalCave/SimpleSolid2/pull/301))
**Owner D2 + contract acceptance:** 2026-10-09 `zatwierdzam - kontynuuj`
**PG-01A baseline:** main `27268d4ec10fbc09721a16ab8f0e2d59bb54833f`, Owner final PASS, FULL #1932 195/195.

## Evidence and failures

- Dedicated governance activation `7bf081e2364126ca3867236835b97da00b1689d1`, [CLOSURE #1934](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37899455498) success, before code edits.
- B1 authored source binding/validation + fail-closed Profile seed guard, [FOCUSED #1937](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37900001657) **1/1 PASS**.
- Early CI requests #1935/#1936 **RED before compiler** because `SS2-Focus-Mode: core` was not supported by parser; corrected to supported desktop mode with no CI/workflow changes.
- v15 schema fixture #1939 compiled RED due to malformed C++ test output escaping; corrected without reducing assertions.
- v15 authored schema with ordered per-Sketch `projected_edges`, strict IDs/source, true native Save/Cold Load, test fixture schema legacy v14/v13 migrations and negative corruptions, [FOCUSED #1940](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37900736881) **1/1 PASS** on exact `8a6e9a984b9c4e50810e2b003b69edfd88be37fa`.
- [FOCUSED #1943](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37901629851) **1/1 PASS**: pure same-revision effective Sketch, identity preservation, no seed mutation, Unsupported/Missing/cycle fail-closed.
- [FOCUSED #1944](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37903136860) **1/1 PASS**: Profile region and the actual Kernel Profile curve reader take the same effective Sketch, not the stale authored seed.
- [FOCUSED #1945](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37903414498) **1/1 PASS**: ordered Extrude and Revolve evaluation wired to effective current Sketch.
- [FOCUSED #1946](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37903700331) **1/1 PASS**: real OCCT strictly authored upstream Edge drives downstream Cut through linked Sketch despite deliberately open authored seed; source suppression blocks current Body.
- [FOCUSED #1947](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37904100902) **1/1 PASS**: linked geometry update denied and atomic Erase of entity+binding with Undo/Redo.
- [FOCUSED #1948](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37904413926) **1/1 PASS**: headless Break Link freezes only the current real OCCT projection and keeps EntityId/role, Undo restores old seed and source binding, Redo detaches.
- [FOCUSED #1950](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37905441728) **1/1 PASS** — linked Trim/Extend/Extend Both fail closed instead of consuming authored seeds.
- [FOCUSED #1951](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37906964949) **1/1 PASS** — Extrude Edit Draft preview uses provider-derived Profile despite stale authored seed. Matching Revolve preview path is implemented but does not yet have a dedicated native draft test.
- [FOCUSED #1952](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37907733612) **1/1 PASS** — shared exact strict source/Frame resolver used by effective Sketch and the headless batch authoring path.
- [FOCUSED #1953](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37908034439) **1/1 PASS** — OCCT-based atomic two-Edge Construction batch, stable IDs, one Undo/Redo, v15 cold Save/Reopen and fresh-provider reprojection.
- [FOCUSED #1954](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37908326900) **1/1 PASS** — first supported source plus second valid-but-degenerate projection rejects the entire batch with typed failing index and zero authored mutation.
- [FOCUSED #1955](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37908534912) **1/1 PASS** — suppressed/broken source cannot Break Link from stale saved seed; no mutation.
- FOCUSED #1956 compiled **RED** due to test-fixture typo `authored` instead of the existing `sketch` variable; corrected without relaxing assertions.
- [FOCUSED #1957](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37909114770) **1/1 PASS** on `94e62cf4a9c8edbdfebabbc4548a75822c028ec0` — Duplicate linked seed blocked while ordinary unlinked duplication remains unchanged.
- **Next:** run wider FAST on documentation-sync candidate; exhaustive B5 cold linked dependent-feature chain and independent Revolve Draft preview regression still open. Exact-head Windows FULL and Owner PG-01B final PASS NOT CLAIMED.

## Implemented semantic boundaries

- `PartSketch::projection_bindings` stores stable target `EntityId` and strict Part `MaterialEdgeReference` (semantic stage/curve/branch); no OCCT/Viewer token persists.
- `PartDocument::validAuthoredState` rejects duplicate/missing target, invalid/unallocated provenance and forward/cyclic Profile-consuming stage relationships. Allocated deleted historical provenance is retained as repairable.
- `PartDocument::evaluateProfile` remains a standalone safety fence against reading stale linked seeds, while ordered Feature evaluation and Extrude draft preview use the pure effective Sketch. Revolve draft preview source route is implemented; direct native proof remains outstanding.
- Schema v15 Save/Load carries explicit sorted `projected_edges`; old schemas default to no bindings, structurally malformed/duplicate sources reject. V15 does **not** yet mean Project Edge user tooling is implemented.

## Not yet demonstrated

B3 standalone Revolve Edit/Create Draft native preview evidence, structural provider-aware edit policies beyond explicit fail-closed paths, and B5 complete cold dependent-Feature rebuild with edited upstream geometry and legacy schema migration acceptance; broader latest-head FAST, final exact-head FULL and Owner PG-01B PASS remain open. PG-01C right-panel UI, PG-01D Face-boundary projection and PM-06 remain separately gated.
