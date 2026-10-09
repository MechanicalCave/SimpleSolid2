# Exact Part Edge Projection and Planar Face Boundaries — as-built

<!-- doc-id: internal.part-edge-projection-kernel -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-edge-projection-kernel.boundary -->
## Geometry-only query

`kernel::IEdgeProjectionQuery` is the provider-neutral, read-only boundary for exact 3D Edge-to-Sketch-plane projection. The OCCT provider is `kernel_occt::OcctSolidModelingKernel`. Success returns `kernel::Curve2` containing exactly one Line2, Circle2 or Arc2 in the target Frame3 (mm and radians). This API does **not** author Sketch entities, Profiles, dependency links, Part schema or UI.

<!-- section-id: internal.part-edge-projection-kernel.identity -->
## Evaluation-local ownership

An unscoped `RuntimeEdgeToken` integer can recur in different OCCT runtime Bodies. First resolve the chosen source Edge against the *current strict Part stage*, then call `bindEdgeToBody(sourceBody, currentEdgeToken)`. This validates current provider inventory membership and produces an opaque, runtime-only `ScopedProjectionEdge`; its captured source Body remains alive.

`projectEdgeToPlane(currentBody, boundEdge, targetFrame)` accepts the binding only when the provided Body object is **identical** to the one captured. Passing an old scoped Edge with a new Body fails `provider_mismatch` even if raw token integers collide. Scope objects and raw tokens are **never persistent topology identity**. The caller must not hand `bindEdgeToBody` a stale integer from another source: the capture method cannot distinguish coincident bare numeric values. The implemented PG-01B/C/D Part and application paths **first** author or re-resolve a strict `BodyStageRef + MaterialEdgeReference` on the current stage, then bind the resolved runtime Edge. No geometry-nearest or traversal-order rebinding.

<!-- section-id: internal.part-edge-projection-kernel.geometry -->
## OCCT geometric rules

The provider looks up the current `TopoDS_Edge` and reads `BRepAdaptor_Curve`. Exact world-to-Sketch point projection is a dot product with target Frame3 U/V. The target frame must be right-handed **orthonormal**, checked against `Precision::Angular()` in addition to the general `Frame3::valid()`.

An OCCT Line produces oriented 2D endpoints; projection collapsing below `Precision::Confusion()` is `degenerate_projection`. Circular images require source and target normals parallel/anti-parallel within `Precision::Angular()`; otherwise their generally elliptical images are `unsupported_curve`. Full circle produces Circle2; a bounded circular Edge produces Arc2 with correct signed sweep, reversed orientation and periodic-seam crossing. Other OCCT curve kinds are `unsupported_curve`; **no tessellated or approximate replacement**.

<!-- section-id: internal.part-edge-projection-kernel.failure -->
## Status and current limits

`EdgeProjectionStatus`: `ok`, `invalid_input`, `provider_mismatch`, `edge_unavailable`, `unsupported_curve`, `degenerate_projection`, `kernel_failure`. Failures contain no curve. OCCT work is executed inside the provider-internal `detail::guardedExactEdgeProjection()` exception boundary, which maps both `Standard_Failure` and unexpected C++ exceptions to `kernel_failure` with **no curve**. The same guarded callable is exercised via the existing OCCT-native `profile_face_evidence.cpp` adapter with deliberate `throw Standard_Failure(...)` and `std::runtime_error`. The headless test reads provider-neutral typed evidence and does not include OCCT headers or expose a fault-injection switch in the projection API. This kernel seam alone does not implement associativity, derived Sketch views, Face boundary capture, Break Link, Save/Reopen, UI or Assembly; the separate **implemented PG-01B/C/D** Part/application/Workbench layers listed below own those bounded responsibilities.

<!-- section-id: internal.part-edge-projection-kernel.evidence -->
## Native tests

The already-registered `pm02p.e_kernel_lifecycle` test covers real Extrude/Revolve/Fillet/Chamfer runtime Edge inventories, XY/XZ/YZ/offset-rotated frames, Line/full Circle/Arc, reverse and seam-crossing arc inputs, tilted circle/arc Unsupported, degenerate Line, foreign/invalid inputs, repeated deterministic results and cross-generation old scoped Edge rejection despite identical raw tokens.

Windows FOCUSED: [#1918](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37892785961), [#1919](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893017161), [#1920](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893301439), [#1922](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893703340): **1/1 PASS each** on their respective tested revisions. Windows [FOCUSED #1930](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37896698388) **PASS 1/1** directly exercises the actual production exception guard with `throw Standard_Failure(...)`, unrelated `std::runtime_error` and successful Line2 passthrough. The native test uses the OCCT-enabled evidence adapter, without changing CMake or leaking provider headers to consumers. Direct synthetic B-Spline construction remains outside the currently executed cases. See `work/PROJECTION_01A_KERNEL_ACCEPTANCE_EVIDENCE.md`.

<!-- section-id: internal.part-edge-projection-kernel.pg01d-native-face -->
## PG-01D — exact read-only native planar Face wire boundary

The optional provider-neutral `kernel::IFaceBoundaryQuery` is defined in `src/kernel/include/simplesolid2/kernel/face_boundary.hpp`, implemented by `kernel_occt::OcctSolidModelingKernel`. A `ScopedBoundaryFace` is bound to the **exact runtime Body object** plus one current `RuntimeFaceToken`; the query fails `provider_mismatch` against another Body generation, even if numeric topology tokens recur. OCCT reads the actual bounded planar `TopoDS_Face` wires and their oriented `TopoDS_Edge` *uses* (exact outer plus inner/hole wires). The provider never obtains boundary members by mesh/tessellation, Surface similarity or Viewer outline. A nonplanar Face or malformed wire accounting is rejected as a typed native status; no geometry repair.

`part::inspectMaterialFaceBoundary` in `src/part/feature_evaluation.cpp` cross-checks this answer against the **complete material Edge catalog from the same FeatureEvaluation and Body stage**. Strict bounded `BodyFaceTopologyRecord::semantic_address` is required; a merely selectable or Sketch-supportable `Surface` carrier is not enough. Each wire Edge must resolve through the established `authorMaterialEdgeReference`: reject seam, nonmaterial representation partition, missing or ambiguous Edge meaning, duplicate/repeated unsound semantic references and topology inventory/provider failure. Any **semantic or wire-integrity** failure rejects the entire Face. Successful `MaterialFaceBoundaryAdmission` has one bounded Face observation and oriented outer/hole Edge uses whose only durable candidates are individual `MaterialEdgeReference` values. A Face/wire number/runtime token is never written into the Part v15 schema.

<!-- section-id: internal.part-edge-projection-kernel.pg01d-workbench -->
## PG-01D — one Workbench tool, exact stage and partial classification

`CadWorkbench` extends the existing PG-01C Project Geometry draft, without a second command, with `EDGES` and `FACE`/`PLANARFACE` source acquisition. `PartViewportController` accepts only current-generation, same-Part legal source-stage Face picks, calls the strict Part adapter and uses `currentMaterialEdgeProjectionStatus` / `part::projectStrictMaterialEdge` with the current Sketch frame for **each semantic Edge**. Only `ProjectedSketchSourceStatus::unsupported_projection` (provider `unsupported_curve`, e.g. Circle onto oblique/perpendicular frame) permits skipping a uniquely resolved member. `missing_stage`, `ambiguous_source`, `unsupported_source`, `degenerate_projection`, provider failure and stale revision do **not** authorize partial admission. Manual Edges and one transient Face gesture are combined/deduplicated by strict source reference.

The right Operations panel reports outer/hole wire counts, per-member `Outer Edge n` / `Hole n Edge m` classification and prominent `PARTIAL`/open-contour warning. `BodyTopologyOverlayScene` has runtime `project_geometry_supported` (cyan) and `project_geometry_unsupported` (red) groups, resolved back through exact stage material references and displayed as separate native OCCT AIS Edge objects. Feedback has explicit current `DocumentRevision` and `BodyPresentationGeneration` leases; Remove/Clear/Cancel/Finish, scene loss and Document/kernel reset clear it. **Zero supported** means Finish disabled and typed Finish rejected; Clear/Remove still work for the all-Unsupported Face.

Before Finish, `CadWorkbench::finishProjectEdgeTool` reruns Face admission for **the same selected, current provider-scoped Face** and compares its bounded semantic address plus exact oriented wire-member runtime Edge/reference membership and **supported vs skipped** classification to the staged snapshot. Changed membership/revision/source/generation fails closed; no unseen Face re-expansion. One successful Finish delegates all supported unique semantic Edges to the **existing** `application::CreateProjectedSketchEdgesCommand` with one role (Regular/Construction), one atomic transaction/Undo. There is **no durable Face reference** and no schema/version change. Subsequent Undo/Redo, v15 Save/Reopen, cold native provider evaluation and ordinary Break Link operate exclusively on per-target EntityId material Edge bindings. A partial batch may leave an open Sketch contour with **no Profile**; no auto-close/healing and no synthesized hidden source.

<!-- section-id: internal.part-edge-projection-kernel.pg01d-evidence -->
## PG-01D native Windows evidence and remaining gates

The isolated full-tier `pg01d.native_face_boundary_d0` checks real provider/Part strict bounded Face admission on Cut holes, including **two native hole wires**, reversed oriented uses and suppression/rebuild/provider generation. The isolated `pg01d.native_face_ui` uses real Qt/OCCT Viewport clicks, not injected tokens, and covers:

- one rectangular cap: four exact linked material Lines and one Undo/Redo;
- two real through-Cuts on the cap: outer wire with four Lines and two inner Circle wires, **six strict distinct** sources, mixed manual Edge+Face dedup, mode switch and Face Remove/restore, one transaction, cold v15 Save/Reopen into a fresh OCCT kernel with a region containing two holes;
- a tilted rectangle cap plus actual circular Cut onto YZ Sketch: **four Resolved Lines plus one geometric Unsupported Circle**, **four cyan plus one red rendered native AIS** objects, `PARTIAL` diagnostics, four linked targets, one Undo and explicitly **no artificial Profile**;
- a separate circular-disk cap onto YZ: **zero supported, one geometric Unsupported**, one red AIS, Finish blocked including typed `FINISH`, Clear/Cancel complete no-op with overlay cleanup;
- suppression of the source Feature after Face staging: stale Finish rejected without mutation; existing links Broken/unresolved until the original Feature is restored, with no old seed fallback.

Exact-HEAD Windows [FOCUSED #2178](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37996833611) PASS 1/1 for positive geometric Partial at `efb2aeeec5ff1d54e1f631bfb794d2344f13aa57`; [FOCUSED #2179](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37997063069) PASS 1/1 for the all-Unsupported gate at `242c7bdc9b878cbca5ca98aa8e4f34c32c9e5dbe`. An earlier complete [Windows FULL #2158](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37992197072) **198/198 PASS applies only to earlier SHA** `4ec50e94190a30d61e254151ae2f6b7f61c287c5`; it is **not** exact-head proof for these later changes.

**Outstanding before Owner PG-01D FINAL PASS:** broader provider/wire/ambiguous-source negative Workbench coverage, final PL/EN/docs Browser validation and immutable Windows FULL at the *final* SHA, then separate practical Owner acceptance and explicit merge authorization. PR #309 remains Draft/Unmerged; unrelated CI #302, Foundation/accepted ADRs, Part v15 and PM-06/PG-01E are outside this Work Contract.

