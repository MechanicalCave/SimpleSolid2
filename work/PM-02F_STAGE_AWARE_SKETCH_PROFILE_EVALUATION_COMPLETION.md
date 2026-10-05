# PM-02F — Stage-Aware Sketch / Profile Evaluation Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Final runtime candidate:** `87d4043f9114952df8228681e4cf16937c6f664b`  
**Final Windows gate:** FULL #1434 — PASS on the exact final candidate  
**Merged runtime main:** `f620d7e9888c09a6c4e9854bca6b3092699c1fe3`  
**Scope boundary:** PM-02F only; no authored Create/Re-support Sketch on Body Surface workflow, no PM-02H Add/Cut product acceptance workflow, no Datum or Projection

## 1. Closure statement

PM-02F is COMPLETED — PASS.

Part Feature evaluation now resolves a Body-Surface-backed Sketch and its Profile from the exact declared upstream Body stage in the same current evaluation pass.

The implementation preserves the accepted authority chain:

```text
authored Sketch local U/V geometry
    + durable semantic SketchSupport
    + exact current BodyStageTopologyCatalog
    -> current resolved Sketch frame
    -> provider-neutral PlanarProfileInput
    -> consuming Feature evaluation
```

No resolved world frame, evaluated topology catalog or materialized world Profile geometry is persisted or reused as last-good CAD truth.

This closes stage-aware evaluation only. User-facing create/re-support authoring remains PM-02G.

## 2. Exact-stage support authority

For a Body-Surface-backed source Sketch, `evaluatePart` resolves support only from a prior successful Feature evaluation whose `result_topology.stage` exactly equals the durable `BodyStageRef` stored by the Sketch support.

The evaluator does not:

- search the final Body globally;
- fall back to a geometrically similar Surface;
- use Viewer or provider topology identity as durable authority;
- use a topology catalog from another Body stage;
- reuse an earlier evaluation's world frame after current support failure.

Origin-backed Sketch/Profile behavior remains compatible and does not require a Body topology catalog.

## 3. Profile materialization result

PM-02F introduces one structured transient Profile-to-Kernel materialization result.

It distinguishes:

- resolved;
- missing Profile or source Sketch;
- unresolved local Profile intent;
- support Missing;
- support Ambiguous;
- support Unsupported;
- invalid provider-neutral Kernel input.

When support resolves, authored local Shared-2D geometry remains unchanged while the current support frame supplies the world-space `Frame3` in `kernel::PlanarProfileInput`.

This keeps local curve/provenance meaning stable while upstream carrier movement changes only derived world placement.

## 4. Failure and stale-frame behavior

Missing, Ambiguous and Unsupported support block the consuming Feature before Kernel execution.

The consuming Feature receives an explicit semantic diagnostic:

- `sketch_support_missing`;
- `sketch_support_ambiguous`;
- `sketch_support_unsupported`.

The Part Tree and Extrude evaluation UI surface these states without converting them into a guessed replacement support.

A previously successful frame cannot feed a later failed current evaluation. The current support must resolve again from current stage topology before a downstream Feature receives Profile input.

## 5. Verification evidence

Final exact-head Windows FULL #1434 passed on `87d4043f9114952df8228681e4cf16937c6f664b`, including:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop CTest execution;
- SR-02 latency evidence;
- CI-04 warm FULL parity and comparative timing evidence.

The dedicated `pm02f.stage_aware_profile_evaluation` regression is registered in both desktop and core-only graphs and proves:

- an upstream extent/carrier move changes the derived world Profile frame;
- authored Body-Surface Sketch support and local Sketch model remain unchanged;
- Profile loop/provenance geometry remains the same local intent across the move;
- Missing support blocks the consumer with structured Missing;
- Ambiguous support blocks the consumer with structured Ambiguous;
- resolved non-planar support blocks the consumer as Unsupported;
- after each failed current support resolution, only the upstream Feature reaches the Kernel;
- stale prior world frame consumption count is zero.

The earlier #1433 run was cancelled after selector-membership hardening changed the candidate and is not completion evidence.

## 6. Explicit non-claims

PM-02F does not claim:

- GUI or Command Line Create Sketch on Body Face;
- authored re-support command semantics;
- one-Finish transaction behavior for face-supported Sketch creation;
- support-cycle command admission UX;
- complete Add + Cut product workflow from face-supported Profiles;
- PM-02 lifecycle/repair/cold-rebuild matrix;
- Datum;
- Projection;
- Fillet or Chamfer.

Those remain owned by PM-02G and later ordered checkpoints/packages.

## 7. Next checkpoint

PM-02G is next:

**create/re-support Sketch on arbitrary planar Body Surface**

It must add the accepted semantic command plus GUI/Command Line workflow for planar cap, lateral and Cut-exposed Surface support, preserve local U/V geometry and stable Sketch/Entity identity on re-support, reject invalid/cyclic support without partial mutation, and keep non-planar Face acquisition visible but Unsupported.

## Documentation impact

Internal/Product documentation: final as-built and PL/EN product documentation remain explicitly owned by PM-02J under the accepted Work Contract. This checkpoint closure changes governance/evidence records only and does not claim PM-02 package completion.

Product Browser: no canonical product documentation changes in this closure PR; regeneration remains owned by PM-02J.
