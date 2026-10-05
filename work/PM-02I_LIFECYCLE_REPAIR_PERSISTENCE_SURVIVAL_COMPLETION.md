# PM-02I — Lifecycle, Repair, Persistence and Survival Matrix Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Final runtime/evidence candidate:** `b642a5af625a48d668a21efeaf3d78bfb9bf7c79`  
**Final Windows gate:** FULL #1451 — PASS on the exact final candidate  
**Merged runtime/evidence main:** `a26d7e981b160973825603bc3c11f37d5ab594a8`  
**Scope boundary:** PM-02I only; no PM-02J documentation/manual acceptance, no new schema, no new repair heuristic, no Datum and no Projection

## 1. Closure statement

PM-02I is COMPLETED — PASS.

The production face-supported Sketch / Profile / Extrude vertical now has an integrated lifecycle and persistence regression that exercises the accepted survival rules against current authored state, current semantic topology evaluation and true native-file cold reopen.

No production modeling or persistence code was changed by PM-02I. The checkpoint closes by proving that the already-delivered PM-02A..PM-02H behavior satisfies the accepted lifecycle/survival contract when exercised as one workflow.

## 2. Integrated production survival workflow

The dedicated `pm02i.face_supported_lifecycle_survival` regression builds:

```text
Origin Sketch/Profile
    -> Base Extrude Add
    -> current evaluated planar Body Surface
    -> face-supported Sketch/Profile
    -> downstream Extrude Add
    -> downstream Extrude Cut
```

It then proves lifecycle, failure, repair and cold-rebuild behavior without persisting runtime topology/provider identity.

## 3. Delete and repair semantics

The regression closes both accepted Delete behaviors:

- deleting the Feature that produces a durable Body-Surface Sketch support is rejected atomically because the authored support/stage invariant would become structurally invalid;
- deleting a Profile that is still referenced by authored downstream Features is allowed as repairable history: the Features remain authored and evaluate as `MissingProfile` / downstream `UpstreamUnavailable` rather than being silently deleted or rewritten.

Undo restores the deleted Profile and the exact downstream Feature identities. Redo reproduces the missing-input state. A final Undo restores the workflow for the remaining survival checks.

This follows ADR-0014/ADR-0015 repairable-history semantics and does not invent cascade delete.

## 4. Support survival and fail-closed behavior

The production regression proves:

- stale re-support revision -> rejected before provider work, zero authored mutation;
- bounded Face split -> semantic planar Surface remains Resolved while strict Face becomes Ambiguous;
- support deletion with an exact same-geometry decoy -> Missing, never geometry-rebound;
- alias/collapse ambiguity -> Ambiguous, no first/nearest/arbitrary winner;
- Missing/Ambiguous support exposes no current support frame and blocks downstream modeling;
- only authoritative upstream history reaches the Kernel after support failure.

The detailed Surface/Face, Edge/Curve and Vertex/Point survival oracles remain the existing PM-02B/PM-02C production regressions and the accepted `work/PM-02P_REFERENCE_SURVIVAL_MATRIX.md`. PM-02I does not duplicate those topology-specific probes.

## 5. Semantic repair and Undo/Redo

The existing re-support command is the repair mechanism.

Repair to a valid Origin plane:

- preserves SketchId;
- preserves EntityIds and authored local U/V geometry;
- produces one authored history entry;
- restores an evaluable Add/Cut chain.

Undo restores the prior face-backed semantic support intent and its structured failure state when that support is currently Missing. Redo reapplies the repaired support. Re-support back to the original current semantic Surface preserves the same IDs/local geometry.

No automatic geometry-similarity repair is introduced.

## 6. Save / Close / Reopen and cold rebuild

The final face-backed state is saved to a real native `.ss2part` file and loaded through a new `PartDocumentStore` instance after previous session/provider state is no longer authority.

Cold reopen preserves:

- DocumentId;
- BodyId;
- FeatureIds and order;
- SketchId;
- ProfileIds;
- semantic Sketch support;
- authored local Sketch geometry.

A fresh provider generation deliberately uses different runtime token values. The same semantic Surface support resolves and the complete Base/Add/Cut chain evaluates UpToDate. Runtime token numeric equality is therefore irrelevant to durable meaning.

`DocumentRevision` is intentionally not compared across reopen because it is runtime freshness, not persisted CAD identity.

## 7. Final verification evidence

Final exact-head Windows FULL #1451 passed on `b642a5af625a48d668a21efeaf3d78bfb9bf7c79`, including:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop CTest execution including `pm02i.face_supported_lifecycle_survival`;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity and comparative timing evidence.

Final survival result includes:

- producer-delete structural rejection;
- repairable Profile Delete -> MissingProfile;
- Undo/Redo repair lifecycle;
- stale support mutation rejection;
- split Face / stable Surface;
- delete + identical-geometry decoy -> Missing;
- alias ambiguity -> Ambiguous;
- explicit re-support repair;
- Save/Close/Reopen;
- true cold rebuild with different runtime token values;
- zero false Resolved;
- no durable ID/provenance corruption.

## 8. Superseded verification attempts

The earlier attempts are evidence of test-harness correction, not production regressions:

- #1448 failed desktop compile on two test-only field-access mistakes and was superseded;
- #1449 compiled and reached CTest, then exposed an incorrect test assumption that deleting a consumed Profile must be rejected. Frozen ADR-0014/ADR-0015 semantics instead preserve the authored Features in a repairable `MissingProfile` state;
- #1450 reached CTest and exposed an incorrect persistence assertion requiring `DocumentRevision` equality across reopen. The accepted architecture defines DocumentRevision as runtime freshness, not durable identity;
- #1451 is the final exact-head PASS after aligning the test with those already-accepted semantics.

No production code was weakened to satisfy the regression.

## 9. Explicit non-claims

PM-02I does not claim:

- PM-02 final documentation completion;
- Product Browser freshness after the required PM-02J documentation update;
- final Owner Windows manual acceptance;
- PM-02 package completion;
- Datum;
- Projection;
- Revolve;
- Fillet or Chamfer.

Those remain owned by PM-02J and later separately activated packages.

## 10. Next checkpoint

PM-02J is next:

**documentation and Owner Windows acceptance**

It must update current internal as-built documentation, PL/EN product documentation and the generated Product Browser, provide the final acceptance matrix, pass the required exact-head runtime/docs gates, and receive Owner manual Windows PASS before PM-02 can be marked complete.

## Documentation impact

Internal/Product documentation: required and explicitly owned by PM-02J. This PM-02I closure itself changes only work/governance evidence.

Product Browser: regeneration remains PM-02J work.
