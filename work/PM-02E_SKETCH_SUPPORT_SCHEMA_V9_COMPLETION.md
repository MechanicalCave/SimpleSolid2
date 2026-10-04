# PM-02E — Sketch Support Schema v9 / Deterministic Frame Resolver Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Final runtime candidate:** `8b5e2c6844c93648e1884464d138d9a98e2a7a9a`  
**Final Windows gate:** FULL #1431 — PASS on the exact final candidate  
**Merged runtime main:** `e3452d4d0051564f332319a7945344655e91aa3a`  
**Scope boundary:** PM-02E only; no stage-aware face-supported Profile evaluation, face-supported Sketch creation/re-support, downstream Extrude workflow, Datum or Projection

## 1. Closure statement

PM-02E is COMPLETED — PASS.

The Part Sketch persistence boundary is now schema v9. Durable Sketch state stores semantic support plus authored local Shared-2D geometry and no longer stores an independent absolute world `SketchPlacement` authority.

The production resolver derives the current Sketch frame from semantic support:

- Origin XY/XZ/YZ retain their established deterministic frames;
- Body planar-Surface support is a durable provider-neutral `SurfaceReference` scoped to an explicit Body stage;
- a Body support resolves only from the exact matching topology catalog and current semantic Surface record;
- Missing, Ambiguous, Unsupported and non-planar support fail closed without a current frame;
- provider/runtime topology identity, evaluated topology catalogs and evaluated frames are not serialized.

This closes persistence and deterministic-frame authority only. Full stage-aware Profile materialization remains PM-02F.

## 2. Schema v9 and migration result

Schema v9:

- persists `PartSketchSupport` as either built-in Origin plane or Body planar Surface;
- persists Body Surface meaning as explicit Body stage plus semantic Surface provenance;
- preserves Sketch-local authored geometry and stable authored IDs;
- omits redundant absolute world placement.

The v8 -> v9 migration:

- preserves DocumentId, BodyId, FeatureId, SketchId, EntityId and ProfileId;
- validates the legacy Origin support/placement pair before migration;
- discards the old absolute placement only after that validation succeeds;
- rejects malformed or incoherent legacy support/placement state;
- rewrites a subsequently saved migrated document as schema v9.

## 3. Derived-frame boundary

The same support/frame authority is used by current Sketch presentation and interaction paths that need a host frame.

The legacy document-only Profile -> Kernel conversion remains deliberately Origin-support-only in PM-02E. It does not synthesize Body-Surface world geometry or search the final Body. PM-02F owns the exact upstream-stage topology context required for face-supported Profile evaluation.

This preserves the ordered checkpoint boundary:

```text
PM-02E  persistence + support resolver
PM-02F  stage-aware Sketch/Profile evaluation
PM-02G  authored create/re-support workflow
PM-02H  Extrude Add/Cut from face-supported Profile
```

## 4. Verification evidence

Final exact-head Windows FULL #1431 passed on `8b5e2c6844c93648e1884464d138d9a98e2a7a9a`, including:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop test execution;
- SR-02 latency evidence;
- CI-04 warm FULL parity and comparative timing evidence.

The dedicated `pm02e.sketch_support_schema_v9` regression proves:

- rich v8 -> v9 migration;
- stable authored identity preservation;
- malformed legacy placement rejection;
- schema-v9 rewrite without persisted placement;
- Body planar-Surface support Save/Reopen roundtrip;
- identical semantic support meaning and resolved frame before/after reopen against equivalent current topology evidence.

Intermediate FULL #1424, #1426 and #1429 were compile-only failures discovered during migration hardening and are not accepted as completion evidence. Their stale API references were removed before final exact-head #1431.

## 5. Explicit non-claims

PM-02E does not claim:

- stage-aware world Profile materialization from Body support;
- downstream Feature evaluation from a face-supported Sketch;
- Create Sketch on Face product workflow;
- re-support command semantics;
- dependency-cycle command admission;
- repair workflow;
- Datum;
- Projection;
- Fillet or Chamfer.

Those remain owned by PM-02F and later ordered checkpoints/packages.

## 6. Next checkpoint

PM-02F is next:

**stage-aware Sketch/Profile evaluation**

It must resolve support from the declared upstream Body stage, derive the current Sketch world frame, materialize Profile geometry from authored local U/V geometry, and fail structurally on Missing/Ambiguous/Unsupported support without consuming stale last-good world geometry.

## Documentation impact

Internal/Product documentation: final as-built and PL/EN product documentation remain explicitly owned by PM-02J under the accepted Work Contract. This checkpoint closure changes governance/evidence records only and does not claim PM-02 package completion.

Product Browser: no canonical product documentation changes in this closure PR; regeneration remains owned by PM-02J.
