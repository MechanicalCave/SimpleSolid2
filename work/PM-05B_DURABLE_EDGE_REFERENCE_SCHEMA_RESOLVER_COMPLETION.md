# PM-05B — Durable Edge Reference / Feature Model / Schema / Resolver Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-05_EDGE_FEATURES.md`  
**Checkpoint split:** B1 durable model/schema + B2 strict-edge resolver/application foundation + B3 generated-Surface durable provenance  
**Date:** 2026-10-07

## Runtime evidence

### PM-05B1 — durable MaterialEdgeReference / Feature model / schema v14

- exact candidate: `abb758a17e52849db4345e03dc84915543718ad4`;
- Windows FULL #1613: PASS;
- merged main: `bfe197b78ff1bc81a8e2e4bd26f2f3e204b429ab` (#283).

B1 delivered:

- durable `FeatureCurveAddress` / `FeaturePointAddress` semantic-reference vocabulary;
- `MaterialEdgeReference = BodyStageRef + FeatureCurveAddress + EdgeBranchDiscriminator`;
- `SingularAtAuthoredStage` and canonical `BetweenSemanticPoints`;
- durable `FilletFeature` and `ChamferFeature` definitions;
- canonical non-empty, deduplicated same-stage multi-Edge sets;
- structural validation rejecting self/future provenance;
- preservation of repairable missing historical provenance so Delete may later produce Missing/Blocked rather than malformed authored state;
- native Part schema v14 persistence and v1..v13 migration;
- fail-closed parsing of edge-feature records mislabeled as pre-v14 schema;
- exact round-trip and malformed/noncanonical persistence regressions.

B1 verification history also exposed and corrected stale legacy tests that asserted current schema v13. Those changes updated only the expected current schema version; prior migration semantics were not weakened.

### PM-05B2 — strict material-Edge resolver / application foundation

- exact candidate: `9b6c6318599ec85300c7b3ebb0ebc24720d5c287`;
- Windows FULL #1617: PASS;
- final aggregate `windows-msvc`: PASS;
- merged main: `13e2c14505be6d25b733b95c71ca891d16a7e4b4` (#284).

B2 delivered:

- runtime-only material Edge incidence at semantic Vertices; no provider token is authored or serialized;
- exact-`BodyStageRef` `MaterialEdgeReference` resolution;
- `SingularAtAuthoredStage`: one current descendant -> Resolved, zero -> Missing, more than one -> Ambiguous;
- `BetweenSemanticPoints`: semantic Point-pair branch discrimination without XYZ, length, provider order or nearest-match fallback;
- Ambiguous semantic endpoint Point remains Ambiguous and cannot be indirectly winner-selected through Edge connectivity;
- current runtime material-Edge authoring admission with material/referenceable/non-seam/non-partition requirements;
- multi-branch authoring fails Unsupported when two defensible semantic endpoint Points are unavailable;
- create-only `FilletDraft` / `ChamferDraft`;
- explicit positive Radius/Distance required; no arbitrary authored default;
- canonical exact-stage Edge-set handling and duplicate rejection;
- DocumentId/DocumentRevision-bound draft freshness;
- `CreateFilletFeatureCommand` / `CreateChamferFeatureCommand` intent DTOs without preallocated FeatureId;
- no production execute/Finish/provider operation before PM-05C.

### PM-05B3 — generated Fillet/Chamfer Surface provenance in schema v14

- exact candidate: `384835c58c5823e5f5ac20c619eb960f3a5acb7a`;
- Windows FULL #1624: PASS;
- final aggregate `windows-msvc`: PASS;
- merged main: `cab3cea663a07102e1749d08dc027a85764db790` (#286).

B3 closed the durable-vocabulary gap found before PM-05C activation:

- durable `fillet_surface`, `chamfer_surface` and `corner_transition` Surface roles;
- accepted P2 provenance: producer FeatureId + source `MaterialEdgeReference`;
- accepted P3 provenance: producer FeatureId + source `FeaturePointAddress` + canonical incident authored Edge set;
- recursive fail-closed history/stage validation with local provenance caps and repairable deleted historical provenance;
- schema-v14 recursive persistence for generated Surface provenance;
- generated Body-Surface references preserve the same provenance when used by Sketch support or Datum Plane sources;
- no runtime/provider topology handle becomes authored or serialized;
- downstream generated engineering Edge intent can name Fillet/Chamfer-generated Surfaces durably before PM-05C publishes the runtime topology.

#1623 on the preceding B3 candidate failed only during desktop compilation with MSVC C1060 compiler-heap exhaustion caused by recursive template/lambda instantiation. The final candidate uses one concrete `SemanticProvenanceBounds` recursion model and passed the complete FULL matrix.

## Verification

Windows FULL #1613 on B1, #1617 on B2 and #1624 on B3 passed their exact candidate heads.

#1617 and #1624 each passed the complete applicable FULL matrix, including:

- complete desktop build graph;
- core-only build/test without Qt/OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity/comparative evidence;
- final `windows-msvc` aggregate.

Dedicated regressions:

- `pm05b1.edge_feature_schema_v14` proves v14 durable Feature/reference persistence, canonical set validation, migration and repairable missing-provenance behavior;
- `pm05b2.strict_edge_resolver` proves exact-stage strict Edge resolution, split/remove behavior, semantic Point-pair discrimination, ambiguity propagation and zero geometry/provider-order fallback;
- `pm05b2.edge_feature_draft` proves canonical create-draft/command intent, explicit parameter admission, duplicate/stage rejection and revision freshness without production provider execution;
- `pm05b3.generated_surface_reference_v14` proves P2/P3 generated-Surface provenance, Fillet-generated Surface -> generated Edge -> downstream Chamfer durable intent, generated Surface Sketch/Datum support round-trip, malformed provenance rejection and zero provider/runtime identity persistence.

## Architecture boundary

PM-05B does **not** add:

- production OCCT Fillet/Chamfer operations;
- production Fillet/Chamfer evaluator execution;
- generated Fillet/Chamfer Surface/Curve/Point topology publication;
- provider tangent-contour admission enforcement in a production operation;
- toolbar / Operations / Viewer preview / Command Line;
- edit/repair lifecycle completion.

Those remain owned by PM-05C/D/E/F in the accepted checkpoint sequence.

## Result

PM-05B is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-05C — kernel operations / evaluation / topology lineage** under the unchanged Owner-accepted PM-05 Work Contract.
