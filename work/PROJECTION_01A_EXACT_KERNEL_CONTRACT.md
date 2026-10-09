# PG-01A — Exact Kernel Edge Projection Work Contract

**Status:** ACTIVE — OWNER ACCEPTED 2026-10-09; bounded exact Kernel/OCCT only
**Prepared:** 2026-10-09
**Baseline:** main `2cda8e04d7d0e2594edae7dc90e88cbb90dcae35` — Projection 00A accepted/merged
**Program:** accepted Part v1 roadmap v1.30, Projection before PM-06
**Design:** `work/PROJECTION_01_PROJECT_GEOMETRY_DESIGN_PROPOSAL.md`
**Source donor:** `MechanicalCave/SimpleSolid` SS1 SK-01 kernel API/OCCT geometry and tests
**Activation:** Owner explicitly wrote `Akceptuje twoje propozycje - kontynuuj pracę` after reviewing proposed PG-01A contract; `work/ACTIVE.yaml` points to this accepted Work Contract. Authority is confined to this exact Kernel-only scope; PG-01B/C/D/E remain inactive.

## Objective

Provide a provider-neutral **exact geometry-only** way to project one *already-resolved*, evaluation-local OCCT material Edge from an authoritative same-revision `RuntimeSolidHandle` onto a validated target 2D `Frame3`, returning exactly one Line/Circle/Arc or an explicit typed failure. Reuse the mathematical/tests ideas of SS1 without porting SS1 identity, dependency model, sketch store or UI.

## Authorized source scope

- SS2 `src/kernel/include/simplesolid2/kernel/**`: narrow result variant, status and projection query boundary. Use or factor existing `Point2`, `Frame3`, `Line2`, `Arc2` where it is already appropriate. No linking to Qt/Viewer.
- SS2 `src/kernel_occt/**`: implement OCCT provider adapter on current-stage runtime body and `RuntimeEdgeToken`, with proper shape/token ownership/generation checks.
- An existing registered native test source and optionally a core-only contract test where needed; candidate `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` and its already-registered `pm02p.e_kernel_lifecycle` target, subject to build-graph/API fit. Prefer a clean additional source if maintainability demands it, but **group any required CMake registration change once**, knowing it triggers CLEAN FULL.
- Package-local `work/**` evidence and contract status, internal API documentation as appropriate.
- **No edits** to `src/part/**`, `src/sketch/**`, `src/application/**`, `src/ui/**`, `src/viewer/**`, persistent Part/schema, `work/ACTIVE.yaml` outside independent activation/completion sync, Foundation, accepted ADRs, global CI workflow or PM-06.

## Typed result and failure policy

- Provider-neutral discriminated output: `LineSegment2`, `Circle2`, `Arc2` using the validated target Sketch frame; exact units in mm/radians.
- Errors at minimum: `InvalidInput`, `EdgeUnavailable` or `ProviderMismatch`, `UnsupportedCurve`, `DegenerateProjection`, `KernelFailure` (exact naming can follow established Kernel vocabulary). No implicit polyline/spline approximations, hidden tolerance escalation or silent fallback.
- `RuntimeEdgeToken` is **only valid in the provided runtime body evaluation/generation**. No guessed provider-order lookup, no persisting the token and no "next nearest".
- `Line` edges may yield nondegenerate 2D Line; a line projected end-on to one point is `DegenerateProjection`.
- Full circular edge whose geometric plane is parallel to target plane yields full `Circle`; bounded OCCT circular edge yields oriented `Arc` (correct reversed orientation and parameter seam). A tilted circle/arc with elliptical image yields `UnsupportedCurve`, not lossy Line or polyline.
- Other OCCT kinds (`GeomAbs_BSplineCurve`, ellipse, conic, etc.) must return typed `UnsupportedCurve`. No manual boundary splitting into multiple unrelated pieces.
- Strict numerical policy and correct frame handedness. SS1 has a fixed angular threshold, but **SS2 must use its own accepted numeric policy and tests**; no copy-pasted `1e-10` constant without justification.
- All exceptional OCCT/provider errors map to typed failure; no throw beyond the Kernel boundary.

## Mandatory native acceptance tests

1. Provider-neutral result validity for Line, Circle, Arc; invalid target frame and mismatched provider solid/tokens refuse.
2. Orthographic cases along Origin XY/XZ/YZ and a non-origin offset/rotated frame; endpoints, center/radius, angle, orientation and sweep within declared numeric policy.
3. Reversed OCCT Edge orientation, partial arc across parameter seam and full 360° circle (no accidental circle→zero-sweep arc).
4. Line with projection collapsed to point: `DegenerateProjection`.
5. Tilted source circle/arc whose projected image is ellipse: `UnsupportedCurve`, no approximation.
6. Unsupported B-Spline/ellipse or complex provider edge: `UnsupportedCurve`; exception path: typed KernelFailure.
7. Reproducibility: same runtime source+frame gives identical typed representation under same provider/machine; no dependence on edge traversal index or Qt.
8. Representative current SS2 Extrude/Revolve/Fillet/Chamfer result Edges: read source from same-revision `BodyStageTopologyCatalog` but let the Part-layer semantic capture remain explicitly OUT OF SCOPE. Probe compatibility; document any particular current material Edge not representable.
9. Fail closed on invalid or cross-generation runtime token, with no partial authored Sketch mutation (this package cannot author Sketch at all).

Required evidence: exact candidate SHA, native FOCUSED, FAST broad regression, final Windows FULL/Owner approval, test names/counts and explicit uncovered cases. No unexecuted PASS claims. Do not weaken assertions or add permissive fallback to make tests green.

## Iteration and build-time policy

Use one existing registered native CTest target first, preferably `pm02p_e_kernel_lifecycle_test` / `pm02p.e_kernel_lifecycle` with `SS2-Focus-Mode: kernel` and matching focus trailers. Avoid full clean rebuild for every small change. If implementation needs an additional registered compilation unit or target, make a **single, justified** CMake modification and record its clean FULL cost. Prefer adapting an existing test and the existing `kernel_occt/solid_modeling_kernel.cpp` compilation unit for bounded work. Do not bypass required CI. Final Ready-for-Review exact-head FULL and docs verification are mandatory.

## Completion / owner gate

- All checked analytic curves are exact; unsupported result types are explicit.
- No Part/Sketch/identity/persistence/UI dependency on PG-01A.
- Canonical SS2 numeric policy and error reporting are documented; tests include negative branches.
- Ready-for-Review Windows FULL PASS, aggregate gate PASS on exact SHA; Owner accepts PG-01A.
- **No automatic PG-01B activation**: freeze durable binding + evaluator/preview D2 next using 00A evidence and PG-01A kernel results, with its own accepted Work Contract.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: New provider-neutral exact Kernel capability requires internal API documentation and native test/evidence coverage; no user-facing Project Geometry tool ships in PG-01A.
