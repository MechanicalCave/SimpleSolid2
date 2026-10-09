# PG-01B — Associative Sketch implementation evidence

**Status:** ACTIVE — B1 PASS, B2 PASS, B3 ordered Extrude/Revolve + real OCCT linked Cut PASS, B4 direct-edit/Erase/Break Link subset PASS; remaining B3 preview, B4 authoring and structural edits, B5 cold/final gates OPEN.
**Current tested source:** `108bbf4af3e2c1143cc060239efd02fb3173d577` (draft [PR #301](https://github.com/MechanicalCave/SimpleSolid2/pull/301))
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
- Wider FAST cross-suite regression **PENDING** for the current candidate; exact-head Windows FULL and Owner final PG-01B acceptance **NOT CLAIMED**.

## Implemented B1 semantics

- `PartSketch::projection_bindings` stores stable target `EntityId` and strict Part `MaterialEdgeReference` (semantic stage/curve/branch); no OCCT/Viewer token persists.
- `PartDocument::validAuthoredState` rejects duplicate/missing target, invalid/unallocated provenance and forward/cyclic Profile-consuming stage relationships. Allocated deleted historical provenance is retained as repairable.
- `PartDocument::evaluateProfile` currently rejects a Profile using linked targets rather than reading a potentially stale authored seed. This is an **intermediate safety fence**, not completion of associative evaluation.
- Schema v15 Save/Load carries explicit sorted `projected_edges`; old schemas default to no bindings, structurally malformed/duplicate sources reject. V15 does **not** yet mean Project Edge user tooling is implemented.

## Not yet demonstrated

B3 Edit/Create draft preview uses the same effective Sketch on every preview/Finish path; B4 atomic batch link creation, source-driven protection against Trim/Extend/duplicate and complete command diagnostics; B4 broken-link Break Link mutation-free negative; B5 complete v15 fresh-provider cold linked reconstruction, malformed/migration regressions, broader FAST/FULL and Owner final PASS. PG-01C right-panel UI, PG-01D planar Face capture and PM-06 remain separately gated.
