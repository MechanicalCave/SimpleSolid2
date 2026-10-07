# PM-05D — Workbench / Viewer / Preview / Command Line Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-05_EDGE_FEATURES.md`  
**Checkpoint split:** D1 shared edge-feature draft/evaluation/Finish + D2 Workbench/Viewer/Command Line integration  
**Date:** 2026-10-07

## Runtime evidence

### PM-05D1 — shared Fillet/Chamfer draft evaluation and exact preview

Delivered:

- revision-bound `FilletDraft` / `ChamferDraft` evaluation through one shared semantic meaning;
- exact transient candidate Body evaluation using the complete authored semantic Edge set against the declared upstream Body stage;
- preview geometry derived from the exact candidate Body presentation mesh, not a visual approximation or raw tool substitute;
- stale DocumentId / DocumentRevision / draft-generation / evaluation rejection;
- no authored mutation and no durable FeatureId consumption before successful Finish;
- one successful Finish -> one Feature in one transaction / one Undo step;
- canonical Fillet/Chamfer create commands routed through `DocumentSession`;
- structured carry-through of target evaluation status, diagnostic, failing Edge input index and reference status.

Dedicated production regression:

- `pm05d1.edge_feature_draft_preview` on the production OCCT kernel proves exact preview, stale-evaluation rejection and one-transaction Fillet/Chamfer Finish.

### PM-05D2 — Workbench / Viewer / Command Line

Delivered:

- Part toolbar grouping into `Create:` and `Modify:`, with Fillet and Chamfer under Modify;
- semantic current-stage material-Edge acquisition through `authorMaterialEdgeReference()`;
- fail-closed filtering of non-authorable, stale or wrong-generation topology;
- explicit multi-Edge toggle selection with command-first and selection-first workflows;
- shared Fillet/Chamfer Operations panel with positive Radius/Distance input;
- exact whole-candidate Body preview using the D1 evaluation path;
- `FILLET` / `CHAMFER` Command Line parity, including Dynamic Input, `REMOVE`, `CLEAR`, `FINISH` and `CANCEL`;
- stale CAD-context rejection through distinct Fillet/Chamfer generation namespaces;
- zero-mutation Cancel;
- explicit PM-05E boundary: Fillet/Chamfer Edit, Suppress and Delete remain disabled/rejected in PM-05D.

Dedicated desktop regression:

- `pm05d2.cad_workbench_edge_features` proves toolbar grouping, selection-first and command-first workflows, semantic multi-Edge selection, exact preview, Dynamic Input parity, individual `REMOVE`, stale-context rejection, Cancel zero mutation and one-Finish commit for both Fillet and Chamfer.

## Verification

Final exact runtime candidate:

- `7f57726b368c8a5011f8007369f3de945ae0d833`.

Windows FULL #1697: **PASS**.

Exact-head evidence:

- complete desktop build graph: PASS;
- core-only build/test without Qt or OCCT: **25/25 PASS**;
- kernel-native Release build/test without Qt: **52/52 PASS**, including `pm05d1.edge_feature_draft_preview`;
- FAST/SUBSYSTEM selector verification: PASS;
- complete desktop CTest: **110/110 PASS**, including `pm05d2.cad_workbench_edge_features`;
- SR-02 latency benchmark evidence: PASS;
- CI-04 warm FULL parity/comparative timing evidence: PASS;
- final `windows-msvc` aggregate: PASS.

The immediately preceding exact candidate `1ad4124600135e4d32a98416560cf52d21563855` failed FULL #1696 only in the new D2 regression because the test assumed the first two presentation-order material Edges must form a geometrically supported two-Edge operation. The follow-up changed only `tests/pm05d2_cad_workbench_edge_features_test.cpp` to search for a genuinely supported two-Edge pair while preserving the required semantic multi-Edge, exact-preview, individual-REMOVE and stale-context assertions. No runtime/product semantics changed between #1696 and the final PASS candidate.

Squash merge:

- PR #290;
- merged `main`: `83ece0b03889e11f8dc7769498d949caee3c0be1`.

## Architecture boundary

PM-05D does **not** complete:

- Fillet/Chamfer Edit preserving `FeatureId`;
- explicit Missing/Ambiguous/Unsupported repair lifecycle;
- Fillet/Chamfer Delete/Suppress lifecycle;
- full edge-feature Undo/Redo lifecycle matrix;
- upstream preserve/split/merge/remove lifecycle closure;
- Save/Close/Reopen and cold-rebuild closure;
- Fillet -> Chamfer / Chamfer -> Fillet lifecycle chaining;
- final PM-05 documentation, cumulative acceptance matrix or Owner Windows acceptance.

Those remain owned by PM-05E/F in the accepted checkpoint sequence.

## Result

PM-05D is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-05E — edit / repair / lifecycle / persistence** under the unchanged Owner-accepted PM-05 Work Contract. PM-05F remains inactive.
