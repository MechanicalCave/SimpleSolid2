# Exact Part Edge Projection — Kernel as-built

<!-- doc-id: internal.part-edge-projection-kernel -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-edge-projection-kernel.boundary -->
## Geometry-only query

`kernel::IEdgeProjectionQuery` is the provider-neutral, read-only boundary for exact 3D Edge-to-Sketch-plane projection. The OCCT provider is `kernel_occt::OcctSolidModelingKernel`. Success returns `kernel::Curve2` containing exactly one Line2, Circle2 or Arc2 in the target Frame3 (mm and radians). This API does **not** author Sketch entities, Profiles, dependency links, Part schema or UI.

<!-- section-id: internal.part-edge-projection-kernel.identity -->
## Evaluation-local ownership

An unscoped `RuntimeEdgeToken` integer can recur in different OCCT runtime Bodies. First resolve the chosen source Edge against the *current strict Part stage*, then call `bindEdgeToBody(sourceBody, currentEdgeToken)`. This validates current provider inventory membership and produces an opaque, runtime-only `ScopedProjectionEdge`; its captured source Body remains alive.

`projectEdgeToPlane(currentBody, boundEdge, targetFrame)` accepts the binding only when the provided Body object is **identical** to the one captured. Passing an old scoped Edge with a new Body fails `provider_mismatch` even if raw token integers collide. Scope objects and raw tokens are **never persistent topology identity**. The caller must not hand `bindEdgeToBody` a stale integer from another source: the capture method cannot distinguish coincident bare numeric values. Future PG-01B must author and re-resolve strict `BodyStageRef + MaterialEdgeReference` before capture; no geometry-nearest or traversal-order rebinding.

<!-- section-id: internal.part-edge-projection-kernel.geometry -->
## OCCT geometric rules

The provider looks up the current `TopoDS_Edge` and reads `BRepAdaptor_Curve`. Exact world-to-Sketch point projection is a dot product with target Frame3 U/V. The target frame must be right-handed **orthonormal**, checked against `Precision::Angular()` in addition to the general `Frame3::valid()`.

An OCCT Line produces oriented 2D endpoints; projection collapsing below `Precision::Confusion()` is `degenerate_projection`. Circular images require source and target normals parallel/anti-parallel within `Precision::Angular()`; otherwise their generally elliptical images are `unsupported_curve`. Full circle produces Circle2; a bounded circular Edge produces Arc2 with correct signed sweep, reversed orientation and periodic-seam crossing. Other OCCT curve kinds are `unsupported_curve`; **no tessellated or approximate replacement**.

<!-- section-id: internal.part-edge-projection-kernel.failure -->
## Status and current limits

`EdgeProjectionStatus`: `ok`, `invalid_input`, `provider_mismatch`, `edge_unavailable`, `unsupported_curve`, `degenerate_projection`, `kernel_failure`. Failures contain no curve. OCCT work is executed inside the provider-internal `detail::guardedExactEdgeProjection()` exception boundary, which maps both `Standard_Failure` and unexpected C++ exceptions to `kernel_failure` with **no curve**. The same guarded callable is exercised via the existing OCCT-native `profile_face_evidence.cpp` adapter with deliberate `throw Standard_Failure(...)` and `std::runtime_error`. The headless test reads provider-neutral typed evidence and does not include OCCT headers or expose a fault-injection switch in the projection API. This does not implement asso, a derived Sketch view, Face boundary capture, Break Link, Save/Reopen, UI or Assembly.

<!-- section-id: internal.part-edge-projection-kernel.evidence -->
## Native tests

The already-registered `pm02p.e_kernel_lifecycle` test covers real Extrude/Revolve/Fillet/Chamfer runtime Edge inventories, XY/XZ/YZ/offset-rotated frames, Line/full Circle/Arc, reverse and seam-crossing arc inputs, tilted circle/arc Unsupported, degenerate Line, foreign/invalid inputs, repeated deterministic results and cross-generation old scoped Edge rejection despite identical raw tokens.

Windows FOCUSED: [#1918](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37892785961), [#1919](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893017161), [#1920](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893301439), [#1922](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893703340): **1/1 PASS each** on their respective tested revisions. Windows [FOCUSED #1930](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37896698388) **PASS 1/1** directly exercises the actual production exception guard with `throw Standard_Failure(...)`, unrelated `std::runtime_error` and successful Line2 passthrough. The native test uses the OCCT-enabled evidence adapter, without changing CMake or leaking provider headers to consumers. Direct synthetic B-Spline construction remains outside the currently executed cases. See `work/PROJECTION_01A_KERNEL_ACCEPTANCE_EVIDENCE.md`.
