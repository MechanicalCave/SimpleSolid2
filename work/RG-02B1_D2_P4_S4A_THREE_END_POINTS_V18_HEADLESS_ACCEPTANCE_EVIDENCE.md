# RG-02B1 / P4-S4A — Headless 3 END Points Datum v18 acceptance evidence

**Status:** IN PROGRESS — NOT OWNER FINAL PASS / NOT MERGE AUTHORIZED.
**Contract:** `work/RG-02B1_D2_P4_S4A_THREE_END_POINTS_V18_HEADLESS_WORK_CONTRACT_DRAFT.md`, Owner D2 accepted and ACTIVE-only guarded squash PR #394 merged protected main `3ce618d0045bc6681f501b32db60cc85373cff08`.
**Production PR:** #395; Draft until exact tests/docs acceptance.
**Product/UI fence:** No GUI, Viewer or CAD Command Line changes. The 3-Point constructor remains S4B, not yet shipping.

## Native implementation candidate and factual CI checkpoints

- Schema v18 introduces mandatory distinct `three_point_datum_planes` collection with fixed ordered source-stage `FeaturePointAddress` triples, one `DatumId` namespace, signed reverse, and strict parser. Existing legacy Offset and EdgePoint collections remain independent.
- Stage topology and Point status are obtained from current native OCCT Part evaluation. Candidate only publishes Resolved if three independent source keys each uniquely map to one strict current Vertex; no persistence of XYZ, OCCT tokens or incident Edges.
- DocumentSession Create/Edit/Reverse use full authored-state candidate re-evaluation and single validated transaction; existing Datum visibility/Delete paths include new kind.
- Registered real Windows native test `pm05c2b.part_edge_topology` now adds create, duplicate-source atomic refusal, Reverse, Undo/Redo, independent-provider native v18 Save/Close/Reopen. Legacy Q1/Q2/P0 assertions are unchanged.
- Initial FOCUSED run **#3113 RED** on SHA `c831ad755656fb944fc7fe0196faf3615dadcc17` was a C++ test-helper declaration-order error (`s1Near3` not declared), not a native source-identity negative. The helper was forward-declared without deleting/changing legacy assertions.
- Windows native FOCUSED [#3115](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38704172970) **PASS 1/1** on exact SHA `59bfa9f0c7e36a1dfed40cdeed7e532628bd6115`, real `pm05c2b.part_edge_topology` built and executed, 0.22 sec. This **does not** imply FAST/FULL or DOC-01 closure.

## Acceptance matrix

| Gate | Actual result |
|---|---|
| S4A-F01 strict unique Point to signed native Datum | Native FOCUSED #3115 PASS |
| S4A-F02 Reverse and edit lifecycle | Native FOCUSED #3115 PASS (bounded native test) |
| S4A-F03 broader unsupported / invalid / near-collinear negatives | PENDING finite extended matrix |
| S4A-F04 atomic Command, Undo, invalid no-mutation | Native FOCUSED #3115 PASS (bounded test); more regression PENDING |
| S4A-F05 downstream Sketch/Axis/Feature dependency | PENDING new positive tests |
| S4A-F06 independent real OCCT native v18 Save/Reopen | Native FOCUSED #3115 PASS (bounded test); older fixtures PENDING |
| S4A-F07 strict malformed v18 grammar, legacy reads, refusal negatives | PENDING |
| S4A-F08 Windows FAST, Ready FULL, DOC-01 Internal+PL/EN/browser | PENDING |
| S4A-F09 Owner technical final verdict, guarded production merge | PENDING |

All prior RED are preserved; no test deletion/weakening. If an existing registered test *outside the activated allowlist* requires new versioned fixture maintenance, STOP for one precise Owner D2 amendment — do not edit it by inference.

## Single point of failure

A computed frame is **not enough**. Required `DatumId` lookup, dependent Sketch/Axis/Feature stage floor, strict native v18 parse/replay, and guarded Save/Close/Reopen must all maintain the same semantic Point source and fail closed; any missing integration branch is a blocker, not partial product PASS.
