# RG-02B1/D2-Q0 — exact bounded 3D Edge geometry: proposed executable Work Contract

**STATUS: DRAFT FOR INDEPENDENT OWNER D2 REVIEW — NOT OWNER-ACCEPTED, NOT MERGED, NOT ACTIVE, NOT IMPLEMENTATION AUTHORITY.**  
**Date:** 2026-10-13  
**Program:** accepted Part Modeling roadmap v1.33; milestone RG-02B1 (Datum Plane Edge+Point), **Q0 geometry-enabling phase ONLY**.  
**Repository source baseline:** `main` `c346133790a644d44b529a1dd288b162f0158540`; `work/ACTIVE.yaml.active_work` still accepted **RG-02B1/E0 evidence-only Work Contract**. This candidate does NOT supersede ACTIVE until a separately authorized and merged activation.  
**Owner planning input:** 2026-10-13 Owner accepted **phase sequence** **Q0 → durable Edge-local END provenance → Datum persistence/orientation/one-tool UX**, **NOT** the exact six-file path set, API shape, contract, PR merge or code execution. Draft planning [PR #351](https://github.com/MechanicalCave/SimpleSolid2/pull/351) is independently OPEN/UNMERGED.  
**Accepted background:** E0 [PR #348](https://github.com/MechanicalCave/SimpleSolid2/pull/348) MERGED, native Windows FULL [#2828](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38392879099) 199/199 PASS, **E0 capability PARTIAL**; source identity design [#349](https://github.com/MechanicalCave/SimpleSolid2/pull/349), P4 default-orientation/Reverse [#350](https://github.com/MechanicalCave/SimpleSolid2/pull/350), complete UX basis [#345](https://github.com/MechanicalCave/SimpleSolid2/pull/345) individually MERGED as **design only**.  
**Authority order:** `AGENTS.md` → Constitution/Foundation/Architecture → accepted ADRs 0014/0016/0017 and relevant earlier ADRs → ACTIVE → roadmap → accepted active contract → source/tests → DOC-01. **Architecture D2 requires Owner acceptance, never agent inference.**

## 1. Single objective and explicit non-objectives

**Q0 objective:** provide one **provider-neutral, strictly current-Body-bound, read-only analytic 3D geometry query** for an already semantically certified **material Edge**. On a bounded nondegenerate **Line** or **open circular Arc**, return exact current physical endpoint positions, bounded length, exact **half-bounded-length MID position**, and **three unoriented analytic tangent lines: one at MID and one at EACH physical END**. Endpoint geometry/tangents are runtime-only, not durable endpoint IDs. Existing actual OCCT native solids and strict Part-stage edges must serve as proof.

**Q0 does NOT** author an Edge reference, select a Vertex for the user, persist END/MID or XYZ, decide which physical endpoint is the **durable** semantic terminal, add a Point source, create/preview/Edit a Datum, choose the signed U/V/N frame, implement the **Odwróć** control, change native Part schema v16/modeling-semantics version, change keyboard/Command Line/UI, G1, Axis, PM-06, generated Browser, or code owned by another phase. **One user-visible Datum Plane Edge+Point constructor** with both pick orders and default preview + Reverse remains the agreed future product; separating Q0 internal proof creates NO new end-user mode/tool.

**Important distinction:** an Edge may have two current native physical terminal positions, but these two points are **transient geometry data** only. They are NOT a durable `End0/End1` ID and their order from OCCT must never become a semantic source address. Q1 alone decides separately proven Edge-local terminal provenance.

## 2. Proposed public boundary — exact interface principle, signatures pending Owner D2 acceptance

New standalone `kernel::IExactBoundedEdgeGeometryQuery` (working interface name) in one new neutral header. Keep `ISolidModelingKernel`, existing `IEdgeProjectionQuery` in `edge_projection.hpp` and `IFaceBoundaryQuery` behavior **source-compatible**; no new pure virtual methods on the old base. Production OCCT provider implements the new interface as an additional base, with no OCCT types exposed to callers.

**Conceptual shape, NOT accepted C++ signature/ABI:**

```text
enum Edge3dCurveKind: { line, open_circular_arc }
enum Edge3dQueryStatus: { ok, invalid_input, provider_mismatch,
                          edge_unavailable, unsupported_curve,
                          degenerate_edge, kernel_failure }

class ScopedExactEdge { // opaque, only query interface may populate
  captures exact RuntimeSolidHandle object + valid current RuntimeEdgeToken;
  never persists, never rebinds to an equally numbered token in another Body;
}

bindCurrentEdge(RuntimeSolidHandle body, RuntimeEdgeToken token)
  -> optional<ScopedExactEdge> // rejects invalid Body, token, missing inventory member
queryCurrentEdge(RuntimeSolidHandle currentBody, const ScopedExactEdge&)
  -> {status, optional<ExactBoundedEdge3d>{
      kind, currentTerminalPositions[2] (unlabeled transient positions),
      currentTerminalTangents[2] (paired with respective positions; UNORIENTED),
      length_mm (> 0), midpointAtHalfBoundedLength_world,
      unitTangentLineAtMid_world (UNORIENTED: ±tangent equally legal)
  }}
```

**Owner D2 decisions to nail down in the acceptance of THIS contract, not guessed in D0 (concrete recommendation §2A):**
1. Exact public names/types, whether result's unit tangent is canonicalized for repeatability **as a geometry value only** (never persisted source orientation), and how to represent an **unoriented** tangent line without leaking a false sign guarantee. A `unit Vec3` without a sign contract may be an interim representation only if downstream callers are explicitly prohibited from using its sign as a persisted frame.
2. The precisely typed `unsupported_curve` and `degenerate_edge` conditions, and provider-failure exception boundary. OCCT `Precision::Confusion()` and `Precision::Angular()` are **existing tolerances**, but their direct reuse for 3D arc-length/zero-edge classification must be justified and approved by exact tests; do **not** introduce new global Sketch/OSNAP snap tolerance or undocumented `epsilon`.
3. Prove actual **trimmed circle arc metric**: half the current bounded geometric length, including positive/negative sweep and crossing the periodic parameter seam. A parametric interval average is NOT adequate for a general future curve; if chosen for certified OCCT Circle, prove equal arc-length on actual trims, otherwise use exact OCCT metric solver/analytical circle length. Reject periodic full circles/seam-only representation Edges.
4. Same-generation binding is by **exact Body object identity** (not numeric `RuntimeEdgeToken`, provider type name, equivalent B-Rep shape or source XYZ); `Part` must FIRST resolve a strict `MaterialEdgeReference` against one exact current `BodyStageRef` and catalog. `Kernel` only validates current runtime member and returns geometry; it must not assign semantic Edge/Point identity.
5. The new query is usable when current native Line/Arc is fully supported but ±normal is geometrically free; **do not return Unsupported for missing orientation preference**. The existing Owner-approved future UX will propose a deterministic signed preview and let user Reverse in later Q2, never in Q0.

**Failure contract:** `ok` implies finite endpoint points, finite midpoint, bounded length strictly positive under accepted modeling tolerance, finite nonzero tangent **line**, one complete verified source Body/Edge and supported curve kind. Every non-ok returns **no geometry payload**, no stale/partial/previously successful data; provider/OCCT failures map to `kernel_failure` without thrown provider exceptions through the public `noexcept` seam. This design must preserve existing 2D projection statuses, exception guards and ABI behavior.

## 2A. Source-verified exact API/metric recommendation — for Owner D2 decision only (2026-10-13)

**D2 DESIGN PROPOSAL — NOT APPROVED CODE / API / TOLERANCES.** Read-only audited `src/CMakeLists.txt`, `tests/kernel_native/CMakeLists.txt`, `src/kernel/include/simplesolid2/kernel/edge_projection.hpp`, `src/kernel_occt/solid_modeling_kernel.cpp`, and existing real `pm02p.e_kernel_lifecycle` / `pm05c2b.part_edge_topology` fixtures on `main` `c346133790a644d44b529a1dd288b162f0158540`.

### Q0-D2-A — precise interface candidate, three unoriented tangents (NOT ACCEPTED)

```cpp
// Illustrative C++ PUBLIC API only; requires independent Owner D2 approval.
enum class ExactEdge3dStatus {
    ok, invalid_input, provider_mismatch, edge_unavailable,
    unsupported_curve, closed_or_periodic, degenerate_edge,
    kernel_failure
};
enum class ExactEdge3dKind { bounded_line, open_circular_arc };
struct UnorientedUnitTangent3 {
    Point3 representative; // {+t,-t} defines ONE tangent line; NOT an N sign
};
struct ExactBoundedEdge3d {
    ExactEdge3dKind kind;
    std::array<Point3,2> current_terminal_positions_world;
    std::array<UnorientedUnitTangent3,2> terminal_tangent_lines;
    double bounded_length_mm;
    Point3 midpoint_at_half_bounded_length_world;
    UnorientedUnitTangent3 midpoint_tangent_line;
};
struct ExactEdge3dResult {
    ExactEdge3dStatus status{ExactEdge3dStatus::invalid_input};
    std::optional<ExactBoundedEdge3d> geometry;
    // ok() only when status==ok && geometry.has_value()
};
class ScopedExactEdge; // default invalid; opaque exact-Body+Edge lease
class IExactBoundedEdgeGeometryQuery {
    // protected scoped-binding factory modeled on IEdgeProjectionQuery;
public:
    virtual ~IExactBoundedEdgeGeometryQuery() = default;
    virtual std::optional<ScopedExactEdge> bindEdgeToBody(
        RuntimeSolidHandle body, RuntimeEdgeToken edge) noexcept = 0;
    virtual ExactEdge3dResult queryBoundedGeometry(
        RuntimeSolidHandle same_current_body,
        const ScopedExactEdge& edge) noexcept = 0;
};
```

The API is an **independent interface**, added to `OcctSolidModelingKernel` as a separate base; do not append a pure virtual method to the existing `ISolidModelingKernel` or alter 2D `IEdgeProjectionQuery`. Existing `RuntimeSolidHandle = shared_ptr<const RuntimeSolid>`, `RuntimeEdgeToken` and `Point3` already exist; no new persistence/Part source type. Endpoint `[i]` pairing is **ONLY for the same current query result**: position and tangent in slot i describe the same physical terminal. Native order may flip after rebuild or Edge reversal; slot i is NEVER an authored End0/End1, source key or canonical orientation. Compare **unordered pairs** and unsigned tangent lines in tests. Both END tangents are needed by future END-perpendicular Datum, not just the MID tangent. None carries authored ±sign; user-visible default orientation and Odwróć remain future Q2, not an error in Q0.

### Q0-D2-B — metric/OCCT algorithm candidate; exact modeling thresholds remain OPEN

Existing production `bindEdgeToBody` rejects wrong provider/missing inventory; `projectEdgeToPlane` binds the **exact Body object pointer** and uses `BRepAdaptor_Curve`, `TopExp::Vertices`, `BRep_Tool::Pnt`, `Precision::Confusion()` and `Precision::Angular()`. It specifically documents a physical endpoint discrepancy between analytic `Value(parameter)` and shared OCCT topological `BRep_Tool::Pnt(Vertex)`; **observed END positions should use the Edge-owned native Vertex coordinates, with analytic-carrier consistency validated**, not choose a point from another Edge by XYZ.

- For `GeomAbs_Line`: validate two non-null/distinct physical terminal vertices and finite coordinates; `bounded_length_mm = norm(P1-P0)`, `MID=(P0+P1)/2` on current bounded Line, plus normalized analytic derivative **tangent lines at each END and at MID**. Validate consistency with current carrier/trimming and numerical model-space tolerance; do not invent durable source orientation from native endpoint index.
- For `GeomAbs_Circle` **only if a genuine open trimmed arc**: finite `first,last` and `radius`, verified (0 < |last-first| < 2π) under accepted angular boundary. Circle speed is constant, so **only in this certified analytic circle case** `length_mm = radius * abs(last-first)` and `mid_parameter = (first+last)/2` are the exact **half bounded arc-length** values. Evaluate current `BRepAdaptor_Curve::Value/D1` at both trim boundaries and half-length point; compare END positions with actual owned Vertex points, accept both physical source sweep signs and trim crossing the circle's angle-0 periodic seam. **A periodic circle carrier does not make a genuinely open arc unsupported; a closed/full circle or representation-only seam is unsupported.**
- Other curves, nonfinite inputs, invalid span/radius, zero/short Edge, derivative failure or inconsistent carrier/Vertex accounting: typed fail closed with no payload. No sampled mesh, 2D projected `Arc2` reinterpreted as 3D, generic parameter averaging or proximity-based endpoint repair. The return tangent represents only an **unoriented line**, never auto-P4 sign or Reverse.
- Proposed status partition for review: `invalid_input` (null/default binding, invalid token); `provider_mismatch` (different current Body object or wrong native provider); `edge_unavailable` (not in current runtime inventory); `unsupported_curve` (non-Line/non-Circle); `closed_or_periodic` (full circle or representation-only closed seam); `degenerate_edge` (short bounded length, radius, tangent magnitude, invalid geometric continuity); `kernel_failure` (unexpected adaptor/native exception/accounting failure). Every non-ok **must have an empty optional** and no public throw under `noexcept`.

**Numeric D2 STOP (not decided by this document):** the current 2D provider uses `Precision::Confusion()` (model-space linear) and `Precision::Angular()` for angle/periodic classification. Q0 should first propose those *existing* values as explicit **comparison candidates**, then measure physical radius/arc/vertex residuals in actual current native fixtures and agree exact handling of near-zero length, tiny open gaps versus closed full circles, and angular/linear units before accepting the C++ contract. OCCT Vertex/Edge tolerances must not be ignored when checking carrier/actual END consistency. No new global Sketch/OSNAP tolerance or undocumented magic `epsilon` can be introduced at D0.

### Q0-D2-C — source-verified build/test boundary (NOT Windows compile proof)

`src/CMakeLists.txt` compiles a fixed list of **.cpp** units, exposes `src/kernel/include` PUBLIC for `simplesolid2_kernel` and `src/kernel_occt/include` PUBLIC for `simplesolid2_kernel_occt`. One NEW **header-only** query interface included from the existing OCCT provider header and implemented inside already compiled `src/kernel_occt/solid_modeling_kernel.cpp` **appears possible with no CMake edit**; target already links `TKernel/TKMath/TKG3d/TKGeomBase/TKBRep/TKTopAlgo` etc. This is a **source audit, not a verified build**. Adding an OCCT helper that actually requires an unlinked library or new .cpp would be **STOP/Owner allowlist amendment**, not presumed in-scope.

`tests/kernel_native/CMakeLists.txt` already registers exact `pm02p.e_kernel_lifecycle` and `pm05c2b.part_edge_topology` in the two allowed existing test files; each links `simplesolid2_kernel_occt`. The `pm02p` real OCCT circular-segment Extrude fixture already tests source sweep **`(0,+π)`, `(π,-π)`, `(5π/3,+π/2)`** for **2D projection only**; the **3D** distance/tangent/END assertions have NOT run. The `pm05c2b` native E0 fixture already confirms canonical 12 strict Line Edges, 24 independently certified END uses and stage editing/cold provider; no current global Point ambiguity may be overwritten by Q0.

Current projection's exception helper `detail::guardedExactEdgeProjection()` is typed to old `EdgeProjectionResult`. A new local provider `try/catch` inside permitted OCCT .cpp might handle Q0 `kernel_failure`, but a **deterministic native throw-injection regression** could require an adapter outside the six paths: mark `Q0-06 exception witness BLOCKED` and request **Owner scope amendment before code** if the existing native test cannot truly trigger that failure. Never declare Q0-06 fully PASS because a catch block merely exists.

### Remaining independent Owner D2 choices (not answered by “continue”)

**A:** approve/revise the above **eight** statuses, three unsigned tangent-line observations (END1, END2, MID) and opaque same-Body binding; whether the wrapped `Point3` vector type sufficiently guards against misuse of sign.

**B:** approve/revise **circle-only constant-speed half-length**, open/closed seam rules, model-space and angular tolerances, carrier-to-Vertex consistency diagnostics, and correct truly native Arc fixture expectations.

**C:** confirm the six exact source/test/as-built paths are a sufficient **future code scope**, with explicit Q0-06 exception-path proof and CMake/link feasibility, or amend the contract BEFORE code. None of this has been implemented or validated by Q0 native tests.

**This §2A is a precise engineering recommendation for Owner consultation, NOT an acceptance of exact C++ signatures, numerical precision or new tests.** E0 #2828 and 2D projection tests do not count as new Q0 proof.

## 3. Exact **proposed** future D0 mutation allowlist: six paths ONLY, pending Owner acceptance + ACTIVE

| Exact file | Permitted narrow mutation only AFTER accepted activation |
| --- | --- |
| `src/kernel/include/simplesolid2/kernel/exact_bounded_edge_geometry.hpp` (NEW candidate) | Provider-neutral geometry-only typed result, error enum, opaque scoped exact Body+Edge and independent exact 3D query interface |
| `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp` | Advertise/override independent interface on existing `OcctSolidModelingKernel`, no old interface semantic change |
| `src/kernel_occt/solid_modeling_kernel.cpp` | Guarded current-trim 3D Line/open Arc geometry, **paired transient terminal/tangent lines** and MID tangent, fail-closed provider/edge/generation validation; no durable source or sampling |
| `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` | Existing registered **CTest `pm02p.e_kernel_lifecycle`** real-OCCT analytic Line/open Arc, trim/seam, invalid provider, degenerate, exact metric/source-generation checks; retain old projection/lifecycle tests |
| `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` | Existing registered **CTest `pm05c2b.part_edge_topology`** bind only after Part certifies a real strict `MaterialEdgeReference` in the correct source Body stage; current provider, provider rebuild and height edit; preserve all E0 evidence |
| `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` | Existing verified DOC-01 canonical as-built `doc-id: internal.part-edge-projection-kernel`: describe **new distinct 3D geometry-only query** while preserving old exact **2D projection**, Face Boundary contracts and stable section markers |

**No seventh mutation path by inference.** NOT allowed: `src/part/**`, `src/application/**`, `src/ui/**`, `src/viewer/**`, `src/kernel/solid_modeling.cpp`, any other source/tests/internal doc, CMakeLists, new test target, workflow/CI, schema/persistence/UI/Product PL/EN or `docs/browser/index.html`, `work/ACTIVE.yaml`, new ADR or roadmap editing. Existing E0 tests cannot be removed, relaxed or rewritten to accommodate an implementation.

**Feasibility STOP:** Before accepting/activating this contract, verify that (a) the new public header participates in existing build/include rules **without editing CMake**, (b) Q0 real native Arc/Line fixtures can be obtained inside the two registered test sources **without other path edits**, and (c) the existing provider-private guarded metric implementation fits inside the one OCCT .cpp. If not, present exact additional path/change for a new Owner D2 scope amendment; **do not** bypass with a test-only production helper, synthetic 2D midpoint, or broad wildcard.

**Only future reporting:** if separately activated, actual run evidence belongs in a separately authorized exact `work/RG-02B1_D2_Q0_EXACT_3D_EDGE_QUERY_EVIDENCE.md` reporting file. That file is **NOT within the present six-path code/doc allowlist** and must be explicitly added by Owner before any report is written. Until then, use PR comments as evidence (no additional repository mutation path).

## 4. Finite native verification matrix — all NEW Q0 cases presently **UNRUN**

| Gate | Existing CTest | Future measured PASS / required negative |
| --- | --- | --- |
| **Q0-01: straight Line** | `pm02p.e_kernel_lifecycle`, `pm05c2b.part_edge_topology` | Real OCCT rectangular Extrude at exact Part Body stage; actual Line, two **paired native END/tangent** observations, bounded length, half-length 3D MID and its unsigned analytic tangent; no mesh or global XYZ used for durable identity |
| **Q0-02: open Arc** | `pm02p.e_kernel_lifecycle` | Real OCCT bounded open circular Arc with positive/negative trim and seam-crossing source; length = current physical bounded arc length; **paired END/tangent observations**, exact half-length MID + tangent; real signed sweep and angle-seam witnesses, no 2D `Arc2` substitute or generic parameter arithmetic |
| **Q0-03: negative topology/curve** | `pm02p.e_kernel_lifecycle` | Real full closed circle/seam, unsupported non-circular curve where fixture permits, collapsed/short trim: typed `closed_or_periodic` / `unsupported_curve` / `degenerate_edge`, **empty payload**, no invented END |
| **Q0-04: provider/generation** | `pm02p.e_kernel_lifecycle` | Valid binding with Body A, fresh Body B with same numeric Edge token: reject `provider_mismatch`; foreign/invalid binding and missing current inventory Edge: no geometry, no numeric rebinding |
| **Q0-05: stage/cold** | `pm05c2b.part_edge_topology` | Strict Part stage reference first, later Cut/Chamfer and upstream height edit; new provider re-resolves proper stage then rebinds; prior scoped token invalid against rebuilt Body, no last-good geometric fallback |
| **Q0-06: exception and numeric integrity** | `pm02p.e_kernel_lifecycle` | Non-finite/collapsed/degenerate input and native OCCT exception guard exercise **within allowed files**; all failures typed and empty with no thrown public C++ exception. Existing exception guard result type belongs to the OLD projection; if no reproducible **Q0** throw witness fits the allowed two tests, mark it BLOCKED and request Owner amendment, not a fake PASS |
| **Q0-07: strict regressions** | Both named native CTests + Windows FAST/FULL | Prior 2D exact projection, E0 strict semantic statuses/invalid Points, accepted refiner and stage tests remain identical and PASS; no fallback topology guess or newly persistent endpoint ID |

**Evidence syntax when a real test runs:** current committed source SHA, *actual* Windows workflow run id/URL, exact registered CTest/test binary, actual case counts, failure status plus input Body generation, optional reproducible geometry observation, no fabricated PASS for skipped/unrun sources. All gates currently **UNRUN**, including any new API implementation; earlier E0 #2828 or 2D PG-01 PASS cannot be repurposed as Q0 PASS.

**Windows closure sequence after independent D2 activation:** preserve accepted native G0/E0 tests; with CMake/CI unchanged, include the current commit message focused trailers for `pm02p_e_kernel_lifecycle_test` / `pm02p.e_kernel_lifecycle`, and `pm05c2b_part_edge_topology_test` / `pm05c2b.part_edge_topology`, e.g. `SS2-Focus-Mode: kernel` (verify exact current CI format before writing). Run real **FOCUSED** for each changed target, then **FAST** after sources settle, then final **Ready-for-Review exact HEAD FULL** with real Core, native Kernel, Desktop, DOC-01 and aggregate. Review workflow logs and all failure statuses, not just green badges. Issue #302 cannot be waived as unrelated flakiness.

## 5. Decision chain and hard STOP

**Q0 scope is not accepted by generic “continue.”** The source-audited §2A is an **unapproved candidate**, not a finalized API/precision decision. Independent gates:

1. **DONE only as plan:** Owner chose Q0 → Q1 → Q2+ sequence on 2026-10-13. [Plan PR #351](https://github.com/MechanicalCave/SimpleSolid2/pull/351) remains OPEN/DRAFT, a one-file planning artifact, **not yet Owner-approved as exact six-path code authority**.
2. **THIS draft executable-contract proposal:** Owner to review/accept exact six-file implementation scope (or amend), semantic 3D typed query contract, status/length/orientation/tolerance rules, existing fixture feasibility and documentation. **Before final exact-text approval** resolve the unresolved items in §2 and pre-activation build/test feasibility STOP in §3. No auto-accept based on this draft PR's green governance CI.
3. **Governance contract merge:** once Owner explicitly accepts the final exact Work Contract text, make the one-document Work Contract PR Ready for Review, require work-only exact-head Windows closure, request **separate PR-specific squash merge authorization**. The accepted scope alone is NOT merge permission.
4. **Separate dedicated activation:** propose minimal `work/ACTIVE.yaml.active_work` switch from the E0 evidence-only contract to this accepted Q0 contract. Preserve roadmap v1.33, Foundation and history pointers. Obtain Owner **scope approval**, Windows governance at exact HEAD, then **separate explicit squash merge consent**. No Q0 code/test mutations until merged activation.
5. **Only then D0 coding:** new code/test branch from active `main`, exact file allowlist, actual Windows FOCUSED/FAST/FULL, measured evidence and scope/source audit. If any FOCUSED or native test fails, diagnose; no extra helper, test weakening, ownerless API or broad refactor without new approval.
6. **Owner technical acceptance and independent implementation PR merge** after measured results. A successful Q0 geometry API is **not** proof of persistent Edge-local END (Q1), authored Datum/schema/frame/Undo/UX (Q2+) or release readiness.

**Must STOP and seek fresh D2 acceptance** for: exact public API or tolerance not settled, test fixture missing within allowlist, CMake header integration required, provider-private exception adapter outside scope, unknown/ambiguous current strict material Edge, out-of-stage input, actual full-circle unsupported semantics contradicted, request to persist runtime END positions, any change to accepted v16 Part schema/ModelingSemantics version or Owner user-facing one-constructor UX.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: **If separately accepted and activated**, Q0 adds a new provider-neutral exact 3D Kernel query boundary, runtime ownership/generation and typed failure semantics; therefore it requires the exact existing `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` current-state update under DOC-01. No user-visible Datum feature, new command, UI, persistence or Product Browser is changed within Q0; paired PL/EN Product updates are deferred to independently approved Q2+ implementation. Documentation verification and as-built current-state accuracy are completion gates, not optional append-only history.

**Owner decision requested in future:** accept/revise THIS six-file Q0 executable scope and settle the explicitly listed D2 API/numeric feasibility questions. This Draft itself, PR #351 plan and all Q0-01…07 tests are NOT approved as production work by the prior sequencing consent.
