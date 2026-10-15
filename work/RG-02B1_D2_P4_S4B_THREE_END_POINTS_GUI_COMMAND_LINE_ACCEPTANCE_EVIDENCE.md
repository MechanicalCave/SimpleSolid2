# RG-02B1 / P4-S4B — single Datum Plane 3 END Points GUI / CAD Command Line acceptance evidence

**State:** WORK IN PROGRESS — NOT Owner manual GUI FINAL PASS / NOT product-merge-authorized.
**Date:** 2026-10-15.
**Active authority:** `work/RG-02B1_D2_P4_S4B_THREE_END_POINTS_SINGLE_DATUM_UI_COMMAND_LINE_WORK_CONTRACT_PROPOSAL.md`, exact D2 accepted. Governance PR #400 guarded squash-MERGED `70fd2de70c48dec47b5f2acef01b1405dda70a2e`, separate ACTIVE-only PR #401 guarded squash-MERGED protected main `61f13d64860cf8491806779f91b2cc64d5068c5f`, both with exact-head Windows governance PASS (#3172 and #3174). Existing S4A headless v18 protected main baseline #395 real Windows FULL #3166 199/199 is not reclassified as new S4B success.
**Implementation:** Draft [#402](https://github.com/MechanicalCave/SimpleSolid2/pull/402), based strictly on activated main, exact 12-file future allowlist from S4B contract. No Part/kernel/Viewer engine/persistence schema v19/CMake/CI/extra GUI-tool edits.

## Feature as currently authored (subject to test verification)

- The ONE existing Datum Plane tool / Operations combo now has Offset (index 0 default), Edge + Point (index 1 unchanged), and 3 Points (index 2). The last mode stores ONLY three ephemeral ordered `BodyStageRef + FeaturePointAddress` slots, editable by P1/P2/P3 slot controls, and a transient signed Reverse; no source Edge, XYZ, view cursor or native token is authored.
- Current viewport callback accepts only strict uniquely certified current-stage semantic Vertex/Point address, rejects a duplicate source and a foreign Body stage, and delegates real noncollinear/near-collinear and native identity revalidation to a disposable Part S4A candidate.
- Preview remains OFF until three valid native points. Finish uses existing S4A transactional create/edit preserving native DatumId, one Undo/Redo; Invalid/Stale/Unsupported fail closed. Explicit Reverse/CANCEL/FINISH, P1/P2/P3, 3POINTS/EDGEPOINT/OFFSET CLI tokens remain context-scoped; no new CAD command tool.
- Same Reference Geometry tree, single global DatumId, visibility group/row, existing Properties and Edit action are extended. No Offset/Edge+Point constructors or S3A native hover removed.

## Real Windows checkpoints (all scope-limited to their exact SHA)

- [Draft FAST #3175](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38727801568): **PASS 93/93** after initial S4B Workbench and Tree implementation. Known CI-04 FAST tier-build membership limitation from S4A still applies; this cannot substitute for registered focused native GUI tests.
- [Qt desktop FOCUSED #3176](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38728263202) **RED at test compile**: local test variable `slots` collided with Qt `slots` macro, corrected without removing any test.
- [Qt desktop FOCUSED #3177](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38728459980) **RED at runtime**: Cancel leaked transient S4B mode in `clearDatumPlaneRuntimeContext` after an earlier misplaced reset in Begin. Corrected to clear all slot/reverse/revision/preview state **on Cancel/Finish**, not just at Begin.
- [Qt desktop FOCUSED #3178](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38728645049) **PASS 1/1** exact `e38f46296739347217772633e539eefbaf35e160`: real Qt single-tool third-constructor UI, slot buttons, in-tool P1/P2/P3/REVERSE/OFFSET/3POINTS, fail-closed FINISH/Enter, Cancel, stale input context and zero Part/Undo mutation with unavailable native source.
- [Native Qt/OCCT FOCUSED #3179](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38729182096) **RED at test compile**: additive mouse fixture attempted to store kernel-space native endpoint position directly as `viewer::Point3`. Corrected solely in test aiming code via typed finite XYZ conversion, never stored in semantic Part.
- [Native Qt/OCCT FOCUSED #3180](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38729365434) **PASS 1/1** exact `a302c53665a42c2dcfbfa432b7464849b02f070f`, real `pm05f_r2.native_workbench_edit` **41.42 s**, preserving original S3A MID↔END and Qt hover checks. The new additive witness uses genuine Qt pointer END hover/click on three distinct real OCCT box corners, rejects a repeated shared corner, confirms non-collinear Frame3 and one S4A Part Create/Undo/Redo, and performs native v18 Save/Close/Reopen with an independent new OCCT provider.
- **NEW, NOT YET TESTED:** native Qt Tree/Properties/true S4B Edit Reverse, identity-stable Undo/Redo and cold v18 replay additions after #3180. Never cite #3180 as testing these newer assertions.

## Finite acceptance matrix — current honest status

| Gate | Status |
|---|---|
| F01 single tool / unchanged Offset + EdgePoint | Qt FOCUSED #3178 PASS for 3rd mode, original registered tests retained; FULL pending |
| F02 real distinct material END Qt clicks | Native FOCUSED #3180 PASS |
| F03 common-corner dedup / stale/unsupported | Shared corner and unavailable native source Qt focused PASS; extended positive/negative pending |
| F04 geometric invalid / preview / signed Reverse | Headless v18 legacy native S4A PASS, UI no preview until valid from #3180; GUI signed Reverse test pending |
| F05 UI Properties Edit/slot replacement/Undo | New real Qt Edit test authored; pending run |
| F06 shared DatumId Tree/Visibility/Save/Reopen | Native v18 cold original #3180 PASS; new Tree/Edit and group visibility pending |
| F07 context-only CLI/Enter/Esc | Qt desktop #3178 PASS (bounded); explicit slot replacement with mouse pending |
| F08 additive real Qt/OCCT negative and source edit | New registered native test now authored; pending run |
| F09 Windows FAST + Ready FULL 199/199 + DOC-01 | PENDING new exact-head tests, source/PL/EN/Browser docs |
| F10 Owner manual Windows GUI technical FINAL PASS, guarded production merge | PENDING separate Owner acceptance and authorization |

Prior RED/Cancelled results stay in the audit trail. The six native Qt CTest RESOURCE_LOCK cases remain active; do not modify CMake or skip a single asserted historical test.

## Explicit limitations

The native v18 S4A Point provenance remains strict and bound to one current Body stage. S4B does not claim all Viewer vertices, universal Point modeling, 3-point MID, or arbitrary XYZ entry. If a source family needs a new Viewer/kernel API or extra file outside the twelve paths, STOP for Owner D2 amendment rather than expanding by inference.
