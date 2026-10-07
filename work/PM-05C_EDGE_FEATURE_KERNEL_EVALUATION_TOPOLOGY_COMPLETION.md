# PM-05C — Edge Feature Kernel / Evaluation / Topology Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-05_EDGE_FEATURES.md`  
**Checkpoint split:** C1 provider-neutral admission/evaluator + C2 production OCCT operations/topology lineage  
**Date:** 2026-10-07

## Runtime evidence

### PM-05C1 — provider-neutral edge-feature kernel admission / evaluator

- exact candidate: `2f55b8382b6fe8022ad26843a7f0eb4dcb15bd1c`;
- Windows FULL #1632: PASS;
- final aggregate `windows-msvc`: PASS;
- merged main: `9547b986ccbd4dc7533c446c797eb5c4886ad0a6` (#287).

C1 delivered:

- provider-neutral `EdgeFeatureOperation` and explicit multi-Edge `EdgeFeatureInput`;
- positive constant-radius Fillet / equal-distance Chamfer parameter admission;
- whole-set `MaterialEdgeReference` resolution against one exact upstream Body stage;
- deterministic failing Edge index and Missing/Ambiguous/Unsupported diagnostics;
- zero provider invocation unless every authored Edge is Resolved;
- transient T1 contour-membership evidence with set equality independent of provider traversal order;
- ordered Part evaluator integration with Blocked vs Failed distinction;
- fail-closed rejection of provider success without exact T1 evidence;
- default fail-closed provider boundary before production OCCT implementation.

### PM-05C2 — production OCCT Fillet/Chamfer / topology lineage

- exact candidate: `e5826858a1ef2580a3c00913f94791653696dc50`;
- Windows FULL #1660: PASS;
- final aggregate `windows-msvc`: PASS;
- merged main: `85c7df1395c59a35b1a80fdf446cdb9c2f77775c` (#288).

C2 delivered:

- production `OcctSolidModelingKernel::edgeFeature()`;
- constant-radius Fillet and equal-distance Chamfer with explicit 1..N authored Edge registration;
- stage-local runtime Edge lookup through the supplied upstream `OcctRuntimeSolid`, with no global token registry;
- T1 native contour-membership inspection before Build and fail-closed provider mismatch on implicit contour growth/omission;
- complete successful result Face/Edge/Vertex inventory;
- inherited Face/Surface/Edge/Vertex lineage separated from PM-05-generated topology;
- inherited Surface continuity only when `Modified` preserves the same analytic carrier; provider `Modified()` alone is not semantic continuity;
- P2 edge-transition and P3 corner-transition generated Surface lineage;
- generated Surface ownership mapped back to already-authored `MaterialEdgeReference` / `FeaturePointAddress` provenance;
- exact one-owner successful-stage Face accounting across inherited and generated semantic Surface carriers;
- `edge_feature_boundary` Curve publication for supported ordinary engineering boundaries;
- generated ordinary engineering Edge authoring for downstream accepted edge-feature intent;
- strict-T1 chaining proof in which provider contour evidence must be fully expressible as explicit durable semantic Edge intent before the downstream Feature is accepted;
- ADR-0016-compliant generated planar frames: provider plane geometry may witness the current plane, but provider UV axes never become semantic frame authority;
- no XYZ, length, nearest, provider-order, first/longest, fuzzy, refine or generic healing fallback.

## Verification

Windows FULL #1632 on C1 and #1660 on C2 passed their exact candidate heads.

Final C2 FULL #1660 passed:

- exact candidate checkout;
- complete desktop build graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity/comparative timing evidence;
- final `windows-msvc` aggregate.

Dedicated regressions:

- `pm05c1.edge_semantic_kernel` proves provider-neutral whole-set resolution, T1 admission and ordered Failed/Blocked evaluator behavior;
- `pm05c2a.production_edge_provider` proves native Fillet/Chamfer, connected explicit Edge sets, exact T1 provider membership, complete topology inventory and stage-local runtime authority;
- `pm05c2b.part_edge_topology` proves production Part P2/P3 publication, common-corner Fillet/Chamfer topology, generated ordinary engineering boundaries, ADR-0016 provider-UV independence and strict-T1 downstream chaining through explicit durable semantic Edge sets.

## Architecture boundary

PM-05C does **not** add:

- Part toolbar / Modify actions for Fillet or Chamfer;
- semantic Viewer multi-Edge picking interaction;
- Operations panels;
- transient Fillet/Chamfer preview;
- `FILLET` / `CHAMFER` Command Line workflows;
- Create Finish wiring through the product UI;
- Edit / repair lifecycle completion;
- Delete/Suppress/Undo/Redo integration specific to the new edge features;
- Save/Reopen/cold-rebuild lifecycle closure;
- final PM-05 Product documentation / Owner Windows acceptance.

Those remain owned by PM-05D/E/F in the accepted checkpoint sequence.

## Result

PM-05C is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-05D — Workbench / toolbar / Viewer / preview / Command Line** under the unchanged Owner-accepted PM-05 Work Contract.
