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

## Subsequent Windows FULL RED and bounded corrections (2026-10-15)

- [Windows Ready FULL #3123](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38704879754), exact `99ad7dc5fe65491f1b6a4d16e274ab3ccb90c3db`: **RED**. Core **25/25 PASS**, kernel-native **56/57 PASS**, including the new registered P4-S4A real-OCCT triple-Point Datum/Undo/cold replay and dependent Sketch/Axis test. The **one** failing test was `pm02jr2.add_surface_continuation`, exact `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp:1521` expecting **17** for the now-current native writer **18**. Its deliberate v15 down-conversion helper also lacks the new v18-only EMPTY-collection requirement. Desktop was not executed; **FULL is not PASS**.
- The failing PM-02JR2 path is **NOT** in the active S4A file allowlist. Requested one-path Owner D2 amendment as governance-only [PR #396](https://github.com/MechanicalCave/SimpleSolid2/pull/396) (Draft). No changes to that unapproved test, no source/parser workaround and no blanket re-interpretation of earlier Owner permissions.
- [Windows FULL #3128](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38708065538) **RED at native test compilation**, exact `0be0b70e8d4221182adceaf8723496f85299f17e`: an additive schema-negative fixture incorrectly included private `nlohmann/json.hpp` from registered `pm05c2b` native OCCT test (not publicly exported to its CMake target). Removed the forbidden direct JSON dependency without touching CMake/CI; relocated seven malformed v18 schema cases to already permitted JSON-capable `tests/pm03a_datum_schema_v10_test.cpp`. New malformed-fixture tests still require exact native/core rerun.
- DOC-01 current-state Internal + PL/EN product Markdown updated to describe **headless** 3-Point Datum and v18 while keeping current GUI to Offset/Edge+Point END/MID only. Generated `docs/browser/index.html` from unchanged template and canonical source-block algorithm, with official Windows `.\\ss2.ps1 docs` parity still to be confirmed. No manual product UI claim or generated template change.
- Pending newer F03 native regression additions: exact collinear and scale-near-collinear current-stage transient evaluations must be Unsupported/Blocked **without Resolved Frame3**; the same stage-qualified semantic P1/P2/P3 source must replay after same producer FeatureId height change. This is new code not covered by earlier #3115 FOCUSED PASS.
- [Windows FOCUSED #3133](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38708941986) on exact code+test+docs/evidence HEAD `dc6812401a51a711782f020e37fc8e7462a22e95`: **PASS 1/1** (`pm05c2b.part_edge_topology`, real Windows OCCT, 0.23 sec). This includes the S4A new strict current-stage collinear / near-collinear *transient catalog* negatives, real native 3-Point Frame/Create/Edit/Undo/Redo, dependent Sketch/Axis and blocked Delete, cold independent provider v18 Save/Reopen, plus same FeatureId height-edit re-evaluation. Original Q1/Q2/P0 native assertions also pass. Does NOT turn cancelled or RED FULL into PASS.
- No new Part/Kernel provider public API, no v19, no GUI/Viewer, no S4B Command Line and no test weakening/deletion.

## Acceptance matrix

| Gate | Actual result |
|---|---|
| S4A-F01 strict unique Point to signed native Datum | Native FOCUSED #3115 PASS |
| S4A-F02 Reverse and edit lifecycle | Native FOCUSED #3115 PASS (bounded native test) |
| S4A-F03 broader unsupported / invalid / near-collinear negatives | Real OCCT test + synthetic current-stage collinear / near-collinear invalid Frame **FOCUSED #3133 PASS**; universal Point classes not inferred |
| S4A-F04 atomic Command, Undo, invalid no-mutation | Native FOCUSED #3115 PASS (bounded test); more regression PENDING |
| S4A-F05 downstream Sketch/Axis/Feature dependency | Native dependent Sketch/Axis identity, blocked Delete and same producer FeatureId height-edit replay **FOCUSED #3133 PASS**; independent wider FULL PENDING |
| S4A-F06 independent real OCCT native v18 Save/Reopen | Native FOCUSED #3115 and #3133 PASS (separate provider and dependent IDs); older fixtures still require FULL |
| S4A-F07 strict malformed v18 grammar, legacy reads, refusal negatives | Existing permitted v17/v16/v9 fixture and new v18 grammar tests authored; pre-new-tests Core #3123 25/25 PASS, current fixtures PENDING rerun |
| S4A-F08 Windows FAST, Ready FULL, DOC-01 Internal+PL/EN/browser | FULL #3123 RED PM-02JR2 unallowlisted legacy test; FULL #3128 RED additive native JSON header (repaired); canonical docs/browser authored, Windows parity PENDING |
| S4A-F09 Owner technical final verdict, guarded production merge | PENDING |

All prior RED are preserved; no test deletion/weakening. If an existing registered test *outside the activated allowlist* requires new versioned fixture maintenance, STOP for one precise Owner D2 amendment — do not edit it by inference.

## Single point of failure

A computed frame is **not enough**. Required `DatumId` lookup, dependent Sketch/Axis/Feature stage floor, strict native v18 parse/replay, and guarded Save/Close/Reopen must all maintain the same semantic Point source and fail closed; any missing integration branch is a blocker, not partial product PASS.
