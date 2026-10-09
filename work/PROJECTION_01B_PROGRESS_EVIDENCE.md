# PG-01B — Associative Sketch implementation evidence

**Status:** ACTIVE — B1 structural binding/v15 persistence FOCUSED PASS; B2–B5 NOT YET COMPLETE
**Current source:** `8a6e9a984b9c4e50810e2b003b69edfd88be37fa` (draft [PR #301](https://github.com/MechanicalCave/SimpleSolid2/pull/301))
**Owner D2 + contract acceptance:** 2026-10-09 `zatwierdzam - kontynuuj`
**PG-01A baseline:** main `27268d4ec10fbc09721a16ab8f0e2d59bb54833f`, Owner final PASS, FULL #1932 195/195.

## Evidence and failures

- Dedicated governance activation `7bf081e2364126ca3867236835b97da00b1689d1`, [CLOSURE #1934](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37899455498) success, before code edits.
- B1 authored source binding/validation + fail-closed Profile seed guard, [FOCUSED #1937](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37900001657) **1/1 PASS**.
- Early CI requests #1935/#1936 **RED before compiler** because `SS2-Focus-Mode: core` was not supported by parser; corrected to supported desktop mode with no CI/workflow changes.
- v15 schema fixture #1939 compiled RED due to malformed C++ test output escaping; corrected without reducing assertions.
- v15 authored schema with ordered per-Sketch `projected_edges`, strict IDs/source, true native Save/Cold Load, test fixture schema legacy v14/v13 migrations and negative corruptions, [FOCUSED #1940](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37900736881) **1/1 PASS** on exact `8a6e9a984b9c4e50810e2b003b69edfd88be37fa`.
- Wider FAST cross-suite regression **PENDING**; exact-head Windows FULL and Owner final PG-01B acceptance **NOT CLAIMED**.

## Implemented B1 semantics

- `PartSketch::projection_bindings` stores stable target `EntityId` and strict Part `MaterialEdgeReference` (semantic stage/curve/branch); no OCCT/Viewer token persists.
- `PartDocument::validAuthoredState` rejects duplicate/missing target, invalid/unallocated provenance and forward/cyclic Profile-consuming stage relationships. Allocated deleted historical provenance is retained as repairable.
- `PartDocument::evaluateProfile` currently rejects a Profile using linked targets rather than reading a potentially stale authored seed. This is an **intermediate safety fence**, not completion of associative evaluation.
- Schema v15 Save/Load carries explicit sorted `projected_edges`; old schemas default to no bindings, structurally malformed/duplicate sources reject. V15 does **not** yet mean Project Edge user tooling is implemented.

## Not yet demonstrated

PG-01B/B2 effective Sketch projection through exact runtime body stage, current linked geometry on source changes; B3 Region/Profile/Extrude/Revolve and Edit/Preview using the same derived Sketch; B4 headless semantic commands, edit protection and Break Link Undo; B5 comprehensive native cold rebuild, malformed migration matrix, full Windows regression and Owner final PASS. PG-01C right-panel UI, PG-01D planar Face capture and PM-06 remain separately gated.
