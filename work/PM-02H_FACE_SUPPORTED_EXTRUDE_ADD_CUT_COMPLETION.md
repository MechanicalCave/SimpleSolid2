# PM-02H — Face-Supported Extrude Add/Cut Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Final runtime candidate:** `0540a762be42b3f4329e7032545e652b21332486`  
**Final Windows gate:** FULL #1446 — PASS on the exact final candidate  
**Merged runtime main:** `16818f4b47d4ec2f1c18410856e2b1c1ba51f2c0`  
**Superseded candidate:** PR #216 / `0f0e19605a7c31396184dd21db34c1f016b72def` passed FULL #1445 but was intentionally closed without merge in favor of the stronger final regression in PR #217  
**Scope boundary:** PM-02H only; no PM-02I lifecycle/repair/persistence closure, no PM-02J documentation/manual acceptance, no Datum and no Projection

## 1. Closure statement

PM-02H is COMPLETED — PASS.

The existing PM-01 Extrude workflow now works end-to-end from a face-supported Sketch/Profile for both Add and Cut. No second Extrude implementation, alternate feature schema or provider-specific modeling path was introduced.

The vertical authority remains:

```text
upstream Body Feature
    -> current exact-stage semantic topology catalog
    -> face-supported Sketch semantic SurfaceReference
    -> derived current support frame
    -> local Sketch/Profile materialization
    -> existing PM-01 ExtrudeDraft / Feature evaluation
    -> provider-neutral Kernel input
```

## 2. Runtime fix

The full candidate `evaluatePart` path was already stage-aware from PM-02F and remained the sole authority for whether Extrude Finish is legal.

PM-02H closes the remaining product-preview gap: the separate runtime-only Extrude delta preview now resolves a face-backed source Profile against the exact declared support stage from the same current candidate evaluation and passes that `BodyStageTopologyCatalog` to the existing `makeKernelExtrudeInput` path.

No evaluated frame, topology catalog, runtime token or provider object is persisted.

## 3. Add / Cut vertical workflow

The dedicated PM-02H regression builds the scenario through public application commands:

1. Origin Sketch/Profile -> Base Extrude Add;
2. obtain the actual resolved planar Surface from the current evaluated Body topology catalog;
3. Create Sketch on that semantic Surface support;
4. author local Sketch geometry and Profile;
5. existing `ExtrudeDraft` Add -> stage-aware preview -> Finish;
6. existing `ExtrudeDraft` Cut -> stage-aware preview -> Finish.

Both operations use the current derived support frame and the existing PM-01 transaction boundary.

## 4. Upstream edit and failure semantics

Editing the upstream Base extent re-evaluates the support stage and moves the derived world frame while preserving authored support and local U/V geometry.

The regression proves both downstream Add and Cut recompute from the moved frame.

When the support becomes Missing or Ambiguous:

- the first face-backed consumer is structurally blocked;
- later Features are blocked as upstream unavailable;
- only the upstream Base reaches the Kernel;
- no prior successful world frame is reused;
- no stale downstream truth is published.

This preserves the PM-02 fail-closed identity/evaluation contract.

## 5. Verification evidence

Final exact-head Windows FULL #1446 passed on `0540a762be42b3f4329e7032545e652b21332486`, including:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop CTest execution;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity and comparative timing evidence.

The final regression proves:

- face-supported Add preview + Finish;
- face-supported Cut preview + Finish;
- support acquisition from the actual evaluated topology catalog;
- current support frame use in both previews;
- upstream carrier move/recompute;
- authored support/local U/V preservation;
- Missing fail-closed;
- Ambiguous fail-closed;
- zero stale downstream Kernel consumption.

PR #216 also passed FULL #1445, but was intentionally superseded because PR #217 exercises the stronger end-to-end support-discovery path.

## 6. Explicit non-claims

PM-02H does not claim:

- full Undo/Redo/Delete lifecycle closure;
- Save/Close/Reopen or cold-rebuild survival matrix closure;
- split/delete/alias/same-geometry replacement repair closure;
- complete semantic repair/re-support acceptance;
- PM-02 documentation or Owner manual acceptance;
- PM-02 package completion;
- Datum;
- Projection;
- Fillet or Chamfer.

Those remain owned by PM-02I, PM-02J and later packages.

## 7. Next checkpoint

PM-02I is next:

**lifecycle, repair, persistence and regression matrix**

It must close Undo/Redo, Delete interactions, Save/Close/Reopen, cold rebuild, stale selection, split/delete/alias failure, same-geometry replacement traps, semantic repair/re-support and exact diagnostics with zero false Resolved and no identity/provenance corruption.

## Documentation impact

Internal/Product documentation: final as-built and PL/EN product documentation remain explicitly owned by PM-02J under the accepted Work Contract. This checkpoint closure changes governance/evidence records only and does not claim PM-02 package completion.

Product Browser: no canonical product documentation changes in this closure PR; regeneration remains owned by PM-02J.
