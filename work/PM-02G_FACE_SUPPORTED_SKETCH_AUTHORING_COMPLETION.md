# PM-02G — Face-Supported Sketch Authoring Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Final runtime candidate:** `f45ed8ee09aabe098ab88bc7e47a0c614841fb40`  
**Final Windows gate:** FULL #1443 — PASS on the exact final candidate  
**Merged runtime main:** `17778bdfe80d0411031b458ef4f6d582eccc205b`  
**Scope boundary:** PM-02G only; no PM-02H Add/Cut product acceptance, no PM-02I lifecycle/repair closure, no Datum and no Projection

## 1. Closure statement

PM-02G is COMPLETED — PASS.

SimpleSolid 2.0 now has one semantic authored workflow for creating a Sketch on an arbitrary supported planar Body Surface and for changing an existing Sketch support between Origin planes and supported Body Surfaces.

The accepted authority chain remains:

```text
fresh Viewer Face acquisition
    -> current Body topology catalog
    -> singular semantic Surface carrier
    -> durable SurfaceReference
    -> runtime-only support draft
    -> Finish-time semantic revalidation
    -> one Part transaction
```

The selected runtime Face, provider token and presentation generation remain transient. Durable Sketch support stores only provider-neutral semantic support intent.

## 2. Semantic command boundary

PM-02G adds shared application/domain commands over `PartSketchSupport` for:

- Create Sketch on support;
- Re-support existing Sketch.

For Body-Surface support, Finish revalidates the selected semantic reference against a fresh exact-stage Part evaluation before mutation.

Structured rejection distinguishes:

- stale revision;
- missing Sketch;
- invalid support;
- Missing;
- Ambiguous;
- Unsupported;
- CycleDependency;
- evaluation/transaction failure.

Origin-plane creation remains provider-independent and does not require a solid-modeling kernel.

## 3. Planar Body Surface admission

The Viewer acquisition path is Face-only while the Sketch support tool is active, but Part semantic admission remains authoritative.

Supported Body-Surface authoring covers semantic planar carriers including:

- Extrude cap Surface;
- planar lateral Surface;
- Cut-exposed planar Surface.

A non-planar Face remains visible/acquirable and reports Unsupported instead of silently falling through to another candidate.

The durable support is the semantic `SurfaceReference`, never the bounded provider Face handle or topology ordinal.

## 4. Re-support and dependency admission

Re-support preserves:

- SketchId;
- EntityIds;
- authored local U/V Sketch geometry.

The support mapping changes without rewriting local geometry.

A bounded ordered single-Body preflight rejects a support stage that is not strictly upstream of every Feature consuming the Sketch through Sketch -> Profile -> Feature.

The same bounded invariant is enforced by `PartDocument::validAuthoredState`, so reconstruction cannot bypass command admission.

No universal dependency graph is introduced.

## 5. Finish / Cancel transaction semantics

The final PM-02G remediation closes the Work Contract gate literally:

- selecting an Origin plane or Body Face creates only a runtime support draft;
- support selection does not change authored state, DocumentRevision or Undo depth;
- the draft is bound to the DocumentRevision observed at target selection;
- GUI Finish, Command Line `FINISH`, and empty Enter after a target is selected invoke the same Finish boundary;
- Finish performs exactly one semantic Create/Re-support transaction;
- Cancel after target selection discards the draft with zero authored mutation;
- rejected or stale Finish performs no partial mutation;
- Undo/Redo are unavailable while the support draft is active.

After successful Create Finish, the newly authored Sketch opens in the existing Sketch edit context. Finishing that later edit context remains presentation/context lifecycle only and does not duplicate the support transaction.

## 6. GUI and Command Line parity

PM-02G uses the same semantic support draft and Finish boundary from:

- the Sketch toolbar command;
- Body Face acquisition in the 3D viewport;
- Origin-plane selection;
- the document-tree **Change Sketch Support** action;
- global Command Line `SKETCH`;
- global Command Line `RESUPPORT`.

`RESUPPORT` requires a singular selected Sketch. Neither GUI nor Command Line persists provider/runtime selection identity.

## 7. Verification evidence

Final exact-head Windows FULL #1443 passed on `f45ed8ee09aabe098ab88bc7e47a0c614841fb40`, including:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop CTest execution;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity and comparative timing evidence.

Dedicated PM-02G evidence proves:

- Origin support parity;
- planar cap support;
- planar lateral support;
- Cut-exposed planar support;
- non-planar support -> Unsupported;
- stale revision rejection;
- bounded cycle rejection before mutation;
- SketchId/EntityId/local-U/V preservation on re-support;
- one support Finish -> one Undo entry;
- support pick before Finish -> no mutation;
- Cancel after support pick -> no mutation;
- GUI Face-only acquisition from a mixed Vertex/Edge/Face hit stack;
- global Command Line `SKETCH` and `RESUPPORT` parity.

The initial PM-02G runtime candidate `2c5ca237143c5722bacde04031235f52f4f05a0f` passed FULL #1439 and merged as `18c3eb8c717468dbb850a630bd4d26bb8f2a7418`, but it is **not** final completion evidence: post-gate contract review found that support pick committed before Finish. Follow-up PR #214 corrected that lifecycle. Intermediate #1441 failed desktop build on one stale symbol and #1442 failed one legacy Workbench expectation; both candidates were superseded. FULL #1443 is the final exact-head evidence.

## 8. Explicit non-claims

PM-02G does not claim:

- final Add/Cut product acceptance from face-supported Profiles;
- complete upstream-edit Add/Cut workflow acceptance;
- PM-02 lifecycle/repair persistence survival matrix;
- final Save/Close/Reopen/cold-rebuild repair acceptance;
- PM-02 package completion;
- Datum;
- Projection;
- Fillet or Chamfer.

Those remain owned by PM-02H and later ordered checkpoints/packages.

## 9. Next checkpoint

PM-02H is next:

**existing Extrude Add/Cut from face-supported Sketch**

It must prove the primary vertical workflow through the existing PM-01 Extrude implementation: Add and Cut from face-backed Profiles, upstream edit re-resolution, valid support move/recompute, and structural lost/ambiguous support failure without stale downstream truth.

## Documentation impact

Internal/Product documentation: final as-built and PL/EN product documentation remain explicitly owned by PM-02J under the accepted Work Contract. This checkpoint closure changes governance/evidence records only and does not claim PM-02 package completion.

Product Browser: no canonical product documentation changes in this closure PR; regeneration remains owned by PM-02J.
