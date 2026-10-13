# RG-02B1/D2-Q0 — Exact current 3D bounded Edge query: smallest engineering scope plan

**Status: DRAFT DESIGN/SCOPE PROPOSAL — NOT OWNER-ACCEPTED EXECUTABLE WORK CONTRACT; NOT MERGED; NOT ACTIVE.**  
**Prepared:** 2026-10-13, original planning-branch baseline `main` `b46f9f1f13cb22b30bfe047f4e7b97972cd30bac`; **latest read-only main recheck** after separately authorized UX [#345](https://github.com/MechanicalCave/SimpleSolid2/pull/345) squash = `c346133790a644d44b529a1dd288b162f0158540`. Roadmap v1.33; `work/ACTIVE.yaml.active_work` remains **RG-02B1/E0 evidence-only**. **NO authority to mutate production or test files below.**  
**Design authority:** Owner accepted UX §0A/§0B/§1B as D2 design basis and **separately squash-MERGED UX [PR #345](https://github.com/MechanicalCave/SimpleSolid2/pull/345)** as work-only review on 2026-10-13. Source semantics [PR #349](https://github.com/MechanicalCave/SimpleSolid2/pull/349) and P4 frame+Reverse [PR #350](https://github.com/MechanicalCave/SimpleSolid2/pull/350) were also independently merged as **work-only design direction**, not implementation authority. Native measured E0 [PR #348](https://github.com/MechanicalCave/SimpleSolid2/pull/348) MERGED, FULL #2828 199/199 PASS, technical conclusion **PARTIAL**.  
**Decision class:** prospective **D2 Kernel public interface**. Explicit Owner approval of exact types, files, tests, numerical policy, documentation impact, later dedicated Work Contract governance and ACTIVE switch are required before D0 code.

## 1. Narrow reason for Q0: a provable 3D query seam before any Datum product

E0-03/04 currently have a **provider-neutral 3D query gap**. Existing `kernel::IEdgeProjectionQuery` in `src/kernel/include/simplesolid2/kernel/edge_projection.hpp` binds a specific runtime Body+Edge generation and returns a **2D projected** Line/Arc/Circle. It cannot serve as the exact current **3D bounded-END/MID/tangent** query for Datum. No amount of GUI clicking or 2D projection can fill this gap, and a displayed polyline/tessellation midpoint is not CAD geometry.

Propose to prove **only** a provider-neutral, evaluation-local **3D metric/analytic Edge observation**:
- Bind a `RuntimeEdgeToken` to the **exact** current `RuntimeSolidHandle` generation from a **previously strict Part-stage `MaterialEdgeReference` resolution**. Enforce provider ownership and same current Body object for query; equal numeric edge tokens on a rebuilt Body MUST NOT rebind.
- For **nondegenerate bounded straight Line**: exact two *current physical terminal positions* as **unlabeled transient geometry**, exact half bounded length 3D point, metric bounded length and **unoriented** analytic tangent line. Never publish a persistent End0/End1 or native first/last semantically.
- For **open trimmed circular Arc**: exact two unlabeled current terminal positions, exact half **bounded arc length** 3D point and unoriented tangent at that point, include trims and both physical sweep orientations. No averaging parameters unless separately proven equal to the half-arc metric under the exact supported curve class. Closed full circle / seam must reject MID (no unique bounded open source).
- Curves other than supported Line/open circular Arc, degenerate/collapsed edges, unknown provider, mismatched Body generation, unbound/invalid tokens and unverified source stages must return typed fail-closed results. **No tessellated sampled path** or nearest-point source matching.
- This output is **geometry only**; token/binding is runtime ephemeral, not a durable authored source, and its analytic **tangent line has no prescribed ±sign**. Later P4 can choose a deterministic normal and propose preview+Odwróć, but Q0 **does not** implement that algorithm.

**Critical division of responsibilities:** `Part` authorizes/strictly resolves `MaterialEdgeReference` within an exact `BodyStageTopologyCatalog` first. `Kernel` validates that the current provider Body contains the exact bound Edge and returns only analytic 3D data. Kernel cannot invent the semantic END terminal provenance key from geometric coordinates, and Part must not promote intrinsic global `FeaturePointAddress::Ambiguous` through incident Edge lists.

## 2. Proposed public seam (conceptual C++ design, NOT approved signature)

```text
IExactBoundedEdgeGeometryQuery  // independent of IEdgeProjectionQuery
  bindCurrentEdge(currentBody, runtimeEdge) -> opaque scoped binding | fail
  queryCurrentBoundedGeometry(currentBody, scopedBinding)
       -> {typed status, curveKind, exactBoundedLength,
           transientTerminalPositions[2] if open,
           exactHalfLengthPoint, unorientedTangentLine}
```

The opaque scoped Edge retains the provider Body generation, and `queryCurrentBoundedGeometry` must compare the same runtime Body authority. The new independent interface must not add a pure virtual function to the existing `ISolidModelingKernel` or change existing `IEdgeProjectionQuery` behavior. Exact result types, ownership and status enum need explicit Owner D2 acceptance; the above is a **nonbinding shape** to compare alternatives. Do not expose OCCT `TopoDS_*`, `gp_*`, mutable viewer types, persisted XYZ as identity or new public Datum schema.

**Known OCCT foothold:** `src/kernel_occt/solid_modeling_kernel.cpp` already uses `BRepAdaptor_Curve` on a current bounded Edge in `projectEdgeToPlane`, reads bounded trim `FirstParameter/LastParameter` and native current endpoints for geometry. `OcctSolidModelingKernel` in the existing public header implements `IEdgeProjectionQuery`. **Reuse may be possible**, but the current code is **2D projection** and its tests do **not** prove a provider-neutral metric-3D query. No OCCT parameter order or `TopAbs_REVERSED` orientation becomes persisted semantics.

## 3. Tentative *exact candidate* file allowlist — NOT YET EDIT PERMISSION

**Only the following files are plausible for a first Q0 implementation, after independent approved Work Contract + ACTIVE switch:**

| Path | Proposed responsibility, to verify before Owner approval |
| --- | --- |
| `src/kernel/include/simplesolid2/kernel/exact_bounded_edge_geometry.hpp` (**new header candidate**) | Narrow provider-neutral opaque scoped current-Edge binding, typed status, metric geometry result, query interface; keep existing projection/solid-modeling source-compatible |
| `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp` | Advertise separate exact-3D query interface on existing production provider |
| `src/kernel_occt/solid_modeling_kernel.cpp` | Exact current OCCT bounded analytic Line/open Arc computation, generation/provider/degeneracy/type checks, no screen/persistence identity |
| `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` | Existing registered `pm02p.e_kernel_lifecycle`: real OCCT Line/circular-segment Arc, positive/negative trims, repeated calls and wrong-generation/foreign-provider negative tests, no new CMake target |
| `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` | Existing registered `pm05c2b.part_edge_topology`: resolve strict stage-current Part material Edge **before** invoking runtime query, unchanged prior E0 evidence, cold provider rebuild + height edit; no CAD identity from transient returned endpoints |
| `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` (**verified existing canonical internal doc**) | Document actual new **3D geometry-only query** as a *distinct* interface from the already implemented 2D `IEdgeProjectionQuery`. Update existing `doc-id: internal.part-edge-projection-kernel` as-built text and stable section markers only when Q0 code is shipped; preserve old projection/Face-boundary current-state details |

**Forbidden without additional Owner amendment:** any `src/part/**` semantic resolver/Datum changes, `src/application/**`, `src/ui/**`, Viewer, `src/kernel/solid_modeling.cpp`, CMake/CI/workflows, new test executables, Part schema/version writer/loader, new Datum variants, Frame3 signed defaults, command grammar, Product Browser, `ACTIVE.yaml`, G1/Axis/PM-06. If a required production helper/OCCT library/test target falls outside the above candidates, **STOP and request an exact scope amendment** instead of expanding allowlists implicitly.

**Documentation Impact for FUTURE Q0 CODE CONTRACT (not permission here):** **Internal docs: required** under DOC-01, **User/Product docs: not required** only if Q0 remains invisible to users. **Verified exact as-built path:** `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` exists on current `main`, its canonical `doc-id` is `internal.part-edge-projection-kernel` and its current early sections define the **2D** `IEdgeProjectionQuery` and same-Body binding policy. A future approved Q0 implementation must add/update a narrowly scoped distinct 3D-query explanation **without falsely renaming the old 2D query** or appending a general implementation history. Q0's **candidate** allowlist is now the above exact six paths (three kernel/OCCT, two existing native tests, one existing internal as-built doc). No wildcard `docs/**`, Product Browser mutation or accepted D2 code permission is inferred from this work-only Draft. **For THIS plan PR only:** Internal docs: not required; User/Product docs: not required; Reason: planning file changes no current implementation.

## 4. Minimal finite Q0 test matrix — EVERY CASE UNRUN for the new query

| Case | Required real current native witness | Required result |
| --- | --- | --- |
| Q0-01 | Actual OCCT rectangular Extrude, strict `MaterialEdgeReference` at exact Part Body stage, 12 current Line Edges | Valid 3D metric length, two physical terminals, true half bounded length Point/tangent **line**, independent of mesh and source order |
| Q0-02 | Native planar circular-segment Extrude containing bounded real 3D open Arc (existing `pm02p_e_kernel_lifecycle` positive, negative and cross-zero trim fixtures) | Half actual **bounded arc length** 3D, exact tangent line, endpoint points, no projected Arc2 as 3D source; nondegenerate |
| Q0-03 | Native full circle cylinder / periodic seam + nonanalytic/ellipse Boolean intersection | Explicit `unsupported_curve`/`closed_curve`/typed failure, no fake MID |
| Q0-04 | Provider A current scoped token queried against different fresh Body B with colliding numeric token; invalid token / foreign provider | Reject provider/generation mismatch/edge missing, no accidental reattachment |
| Q0-05 | Real Part height edit and new OCCT instance, exact same stage semantic source re-resolved | Fresh binding gives correct **new geometry**; old bound body/edge never substituted; semantic reference compared by Part only |
| Q0-06 | Tiny/zero trim, finite checks, unsupported curve, OCCT exception | Consistent typed failure, no invalid point/tangent returned |
| Q0-07 | Regression of existing 2D `IEdgeProjectionQuery` and prior E0/G0 evidence | Unchanged status/semantics, no test weakening; transient endpoints never serialized as CAD source |

**FOCUSED test target registration:** only existing exact `pm02p.e_kernel_lifecycle` and `pm05c2b.part_edge_topology` may be used if source code changes; read current CI focus-trailer policy just before implementation. Require exact-source Windows FOCUSED each, Windows FAST, final Ready-for-Review Windows FULL with Core/native/Desktop/docs and aggregate. Record actual GitHub Actions URL, counts, status and source SHA. **All these Q0 tests presently UNRUN** and Q0 does not produce Datum planes, UI or signed preview.

## 5. Explicit D2 owner gates (not blanket implementation approval)

1. Owner accepts/revises the **Q0 split**: isolated exact current 3D source geometry query, separate durable Edge-local END provenance (future Q1) and signed preview/Reverse + Datum schema/UI (future Q2+), **while preserving ONE eventual user-visible Datum Plane Edge + Point constructor**.
2. **Exact internal-doc target VERIFIED:** `docs/internal/PART_EDGE_PROJECTION_KERNEL.md`, current canonical doc-id `internal.part-edge-projection-kernel`. Owner must still accept it as the sixth explicit future executable file and review exact C++ geometry API signature, invalid-provider/Body statuses, numerical tolerances and provider exception handling; settle whether current native Arc fixture is sufficient without a new build target. The existing document currently describes *2D* projection, not a working exact-3D query. If not feasible within the six candidates, amend or STOP.
3. Issue a separate **complete executable Q0 Work Contract** (not this proposal), with **six exact proposed paths** including verified `docs/internal/PART_EDGE_PROJECTION_KERNEL.md`, objective test fixtures, explicit D2 sign-off, DOC-01 compliance and no changes to accepted UX. Merge only under its own exact-HEAD Owner authorization, then independently Owner-approve and merge a dedicated `ACTIVE.yaml` activation **before ANY source/test mutation**.
4. When complete, record bounded evidence; do not mark durable END provenance, new Datum schema, MID preview, full circle support, orientation continuity or UI delivered. A future Q1/Q2+ phase requires independent acceptance, but **no splitting the single user-facing tool into separate modes/features**.

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Reason: This one-file work-only design/scope plan records the next D2 boundary and verifies the existing future documentation target `docs/internal/PART_EDGE_PROJECTION_KERNEL.md`; it does not change production behavior, current Kernel interfaces, as-built documentation or user workflow. The prospective Q0 API implementation must explicitly include that internal document in an independently accepted executable Work Contract under DOC-01.
