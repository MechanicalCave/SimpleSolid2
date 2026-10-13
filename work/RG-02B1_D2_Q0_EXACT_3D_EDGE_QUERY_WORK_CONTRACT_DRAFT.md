# RG-02B1/D2-Q0 — exact bounded 3D Edge geometry: proposed executable Work Contract

**STATUS 2026-10-13: Owner accepted Q0 phase order; three unoriented END/END/MID tangents, eight statuses and analytic bounded Line/open-Arc MID IN PRINCIPLE; N1a-P1..P5/E1a-P as PROVISIONAL test hypotheses; and separately §2D complete scoped Edge interface DIRECTION plus SIX EXACT prospective code/test/docs paths AS BASIS FOR THE FINAL EXECUTABLE CONTRACT. Exact final C++ public API/Work Contract text STILL REQUIRES independent Owner acceptance. No new Q0 native tests run, no new-Q0 Standard_Failure direct injection proven. PR #352 OPEN/DRAFT/UNMERGED, ACTIVE E0 evidence-only, NO CODE AUTHORITY.**  
**Date:** 2026-10-13  
**Program:** accepted Part Modeling roadmap v1.33; milestone RG-02B1 (Datum Plane Edge+Point), **Q0 geometry-enabling phase ONLY**.  
**Repository source baseline:** `main` `c346133790a644d44b529a1dd288b162f0158540`; `work/ACTIVE.yaml.active_work` still accepted **RG-02B1/E0 evidence-only Work Contract**. This candidate does NOT supersede ACTIVE until a separately authorized and merged activation.  
**Owner planning and technical direction:** Owner accepted Q0 → Q1 durable Edge-local END → Q2+ Datum/UX ordering, Q0 three-tangent/eight-status bounded analytic Line/open-Arc geometry in principle, N1a/E1a preferred methods and §2C exact provisional test hypotheses. Most recently Owner replied “akceptuję - kontynuuj” to a question expressly limited to accepting §2D **interface direction and six exact proposed paths as the basis of a future executable Q0 contract**, not the complete final contract text, PR #352 merge, Q0 ACTIVE or code/test/docs changes. The native new-Q0 Standard_Failure injection remains NOT DIRECTLY TESTED. Planning PR #351 remains OPEN/DRAFT/UNMERGED.
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

bindExactEdgeToBody(RuntimeSolidHandle body, RuntimeEdgeToken token)
  -> optional<ScopedExactEdge> // rejects invalid Body, token, missing inventory member
queryExactBoundedEdge3d(RuntimeSolidHandle currentBody, const ScopedExactEdge&)
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

### Q0-D2-A — three tangents / eight-status interface DIRECTION OWNER-ACCEPTED; exact C++ remains DRAFT

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
    virtual std::optional<ScopedExactEdge> bindExactEdgeToBody(
        RuntimeSolidHandle body, RuntimeEdgeToken edge) noexcept = 0;
    virtual ExactEdge3dResult queryExactBoundedEdge3d(
        RuntimeSolidHandle same_current_body,
        const ScopedExactEdge& edge) noexcept = 0;
};
```

**Source-verified C++ name collision correction (UNAPPROVED SPELLING):** existing 2D `IEdgeProjectionQuery::bindEdgeToBody(RuntimeSolidHandle,RuntimeEdgeToken)` already returns `optional<ScopedProjectionEdge>`. A second identically parameterized inherited method returning `optional<ScopedExactEdge>` cannot coexist in `OcctSolidModelingKernel`: C++ does not overload by return type. Propose DISTINCT names `bindExactEdgeToBody` and `queryExactBoundedEdge3d`; the existing 2D method remains unchanged. Exact new names require separate Owner signature approval. The new interface is a separate base; no new old-base pure virtual method. Existing `RuntimeSolidHandle = shared_ptr<const RuntimeSolid>`, `RuntimeEdgeToken` and `Point3` already exist; no new persistence/Part source type. Endpoint `[i]` pairing is **ONLY for the same current query result**: position and tangent in slot i describe the same physical terminal. Native order may flip after rebuild or Edge reversal; slot i is NEVER an authored End0/End1, source key or canonical orientation. Compare **unordered pairs** and unsigned tangent lines in tests. Both END tangents are needed by future END-perpendicular Datum, not just the MID tangent. None carries authored ±sign; user-visible default orientation and Odwróć remain future Q2, not an error in Q0.

### Q0-D2-B — analytic Line/Open-Circle metric DIRECTION OWNER-ACCEPTED; exact modeling thresholds OPEN

Existing production `bindEdgeToBody` rejects wrong provider/missing inventory; `projectEdgeToPlane` binds the **exact Body object pointer** and uses `BRepAdaptor_Curve`, `TopExp::Vertices`, `BRep_Tool::Pnt`, `Precision::Confusion()` and `Precision::Angular()`. It specifically documents a physical endpoint discrepancy between analytic `Value(parameter)` and shared OCCT topological `BRep_Tool::Pnt(Vertex)`; **observed END positions should use the Edge-owned native Vertex coordinates, with analytic-carrier consistency validated**, not choose a point from another Edge by XYZ.

- For `GeomAbs_Line`: validate two non-null/distinct physical native terminal vertices with finite coordinates. **For the analytic bounded material Edge geometry**, evaluate the current `BRepAdaptor_Curve` at its **actual first/last trim parameters**, and compute `bounded_length_mm = norm(Value(last)-Value(first))` and `MID = Value((first+last)/2)` **only after proving this is a linear, constant-speed adaptor**; validate that the two Edge-owned `BRep_Tool::Pnt(Vertex)` coordinates are within accepted OCCT model/Vertex/Edge tolerance of the corresponding curve endpoints (either transient order). Their arithmetic midpoint **must not silently replace the exact current curve MID** if the carrier and Vertex points differ by tolerance. Evaluate normalized analytic derivative **tangent lines at both END and MID**; avoid persisting any native parameter-order sign, geometric coordinates or vertex index. If carrier/Vertex mismatch exceeds the accepted tolerance: typed fail-closed, no proximity repair.
- For `GeomAbs_Circle` **only if a genuine open trimmed arc**: finite `first,last` and `radius`, verified (0 < |last-first| < 2π) under accepted angular boundary. Circle speed is constant, so **only in this certified analytic circle case** `length_mm = radius * abs(last-first)` and `mid_parameter = (first+last)/2` are the exact **half bounded arc-length** values. Evaluate current `BRepAdaptor_Curve::Value/D1` at both trim boundaries and half-length point; compare END positions with actual owned Vertex points, accept both physical source sweep signs and trim crossing the circle's angle-0 periodic seam. **A periodic circle carrier does not make a genuinely open arc unsupported; a closed/full circle or representation-only seam is unsupported.**
- Other curves, nonfinite inputs, invalid span/radius, zero/short Edge, derivative failure or inconsistent carrier/Vertex accounting: typed fail closed with no payload. No sampled mesh, 2D projected `Arc2` reinterpreted as 3D, generic parameter averaging or proximity-based endpoint repair. The return tangent represents only an **unoriented line**, never auto-P4 sign or Reverse.
- Proposed status partition for review: `invalid_input` (null/default binding, invalid token); `provider_mismatch` (different current Body object or wrong native provider); `edge_unavailable` (not in current runtime inventory); `unsupported_curve` (non-Line/non-Circle); `closed_or_periodic` (full circle or representation-only closed seam); `degenerate_edge` (short bounded length, radius, tangent magnitude, invalid geometric continuity); `kernel_failure` (unexpected adaptor/native exception/accounting failure). Every non-ok **must have an empty optional** and no public throw under `noexcept`.

**Numeric D2 STOP (not decided by this document):** the current 2D provider uses `Precision::Confusion()` (model-space linear) and `Precision::Angular()` for angle/periodic classification. Q0 should first propose those *existing* values as explicit **comparison candidates**, then measure physical radius/arc/vertex residuals in actual current native fixtures and agree exact handling of near-zero length, tiny open gaps versus closed full circles, and angular/linear units before accepting the C++ contract. OCCT Vertex/Edge tolerances must not be ignored when checking carrier/actual END consistency. No new global Sketch/OSNAP tolerance or undocumented magic `epsilon` can be introduced at D0.


**Critical exactness distinction for Q0-D2-B:** an OCCT B-Rep topological Vertex position and `BRepAdaptor_Curve::Value(first/last)` can differ by kernel tolerance (confirmed in the existing 2D projection implementation comments). Endpoint **hit observation** may use the native Vertex coordinates, while the **metric MID/tangent** should follow the bounded analytic curve. Their consistency threshold is an explicit D2 numeric decision, not license to shift source identity, approximate from a polyline, or treat topological tolerance as arbitrary UI snap. The new Q0 test must intentionally exercise this carrier-versus-topological-Vertex discrepancy where constructible within the current two native test targets; otherwise mark that specific stress case NOT DEMONSTRATED rather than treating a rectangle alone as proof.

### Q0-D2-C — source-verified build/test boundary (NOT Windows compile proof)

`src/CMakeLists.txt` compiles a fixed list of **.cpp** units, exposes `src/kernel/include` PUBLIC for `simplesolid2_kernel` and `src/kernel_occt/include` PUBLIC for `simplesolid2_kernel_occt`. One NEW **header-only** query interface included from the existing OCCT provider header and implemented inside already compiled `src/kernel_occt/solid_modeling_kernel.cpp` **appears possible with no CMake edit**; target already links `TKernel/TKMath/TKG3d/TKGeomBase/TKBRep/TKTopAlgo` etc. This is a **source audit, not a verified build**. Adding an OCCT helper that actually requires an unlinked library or new .cpp would be **STOP/Owner allowlist amendment**, not presumed in-scope.

`tests/kernel_native/CMakeLists.txt` already registers exact `pm02p.e_kernel_lifecycle` and `pm05c2b.part_edge_topology` in the two allowed existing test files; each links `simplesolid2_kernel_occt`. The `pm02p` real OCCT circular-segment Extrude fixture already tests source sweep **`(0,+π)`, `(π,-π)`, `(5π/3,+π/2)`** for **2D projection only**; the **3D** distance/tangent/END assertions have NOT run. The `pm05c2b` native E0 fixture already confirms canonical 12 strict Line Edges, 24 independently certified END uses and stage editing/cold provider; no current global Point ambiguity may be overwritten by Q0.

Current projection's exception helper `detail::guardedExactEdgeProjection()` is typed to old `EdgeProjectionResult`. A new local provider `try/catch` inside permitted OCCT .cpp might handle Q0 `kernel_failure`, but a **deterministic native throw-injection regression** could require an adapter outside the six paths: mark `Q0-06 exception witness BLOCKED` and request **Owner scope amendment before code** if the existing native test cannot truly trigger that failure. Never declare Q0-06 fully PASS because a catch block merely exists.

### Remaining independent Owner D2 choices after IN-PRINCIPLE ACCEPTANCE (not exact executable approval)

**A — ACCEPTED IN PRINCIPLE:** eight conceptual statuses, three **unoriented** tangent-line observations (END, END, MID), paired physical terminal positions and opaque same-Body binding. **Still open:** exact C++ symbol signatures and whether wrapped `Point3` is sufficiently sign-safe for future consumers; no persisted native slot ID.

**B — ANALYTIC DIRECTION ACCEPTED IN PRINCIPLE:** constant-speed certified **open** Circle Arc half-bounded-length metric and analytic Line MID. **Still open:** exact near-closed seam/short Edge status thresholds, model-space/Vertex/Edge/angular tolerance policy and proof in real native fixtures.

**C:** confirm the six exact source/test/as-built paths are a sufficient **future code scope**, with explicit Q0-06 exception-path proof and CMake/link feasibility, or amend the contract BEFORE code. None of this has been implemented or validated by Q0 native tests.

**This §2A is the Owner-accepted IN-PRINCIPLE engineering direction, NOT acceptance of exact C++ signatures, numerical tolerance, Q0 exception witness, allowed code edits or new tests.** E0 #2828 and 2D projection runs do not count as Q0 proof.

## 2B. Owner-preferred N1a/E1a directions — exact numeric and native exception proof STILL OPEN (2026-10-13)

Owner explicitly accepted **N1a and E1a as the preferred technical directions**, conditional on the future **real Q0 code/tests**, not as complete numeric tolerances, exact public ABI, full exception coverage or authorization to edit anything. These test matrices are **design obligations, not executed evidence**. No silent promotion of OCCT's internal tolerances to a global SS2 modeling tolerance is authorized. If actual source-backed Q0 evidence invalidates either preferred direction, STOP and present a concrete Owner D2 amendment.

### N1 — N1a PREFERRED IN PRINCIPLE by Owner; still UNPROVEN and not a frozen numeric threshold

Distinct physical predicates, each with its own **explicit units and threshold**:

| Check | Proposed mathematical comparison | Evidence/Owner decision needed |
| --- | --- | --- |
| Finite/typed input | All source `Point3`, native Curve trim bounds, Circle radius and analytic `D1` derivative finite; positive bounded interval | Reject nonfinite/invalid without geometry, before normalization |
| Geometric Line length | `L = norm(Value(last) - Value(first))` in **mm**, positive beyond an explicitly accepted **geometric-length** lower bound `T_length` | Do not substitute topological Vertex distance for the analytic trimmed carrier; sweep known Line lengths against threshold |
| Geometric Circle Arc length | Certified `GeomAbs_Circle`, `L = r * abs(last-first)` (**mm**) and `0 < span < 2π` (**radians**) | Prove constant speed, valid physical end trims, both source sweep signs and angle-0 seam crossing |
| Physical carrier↔Vertex consistency | For each Edge-owned Vertex matched to the corresponding analytic trim boundary, `distance(Pvertex, Value(trim)) <= T_endpoint` (**mm**), allowing **only** the two candidate native pairing orders within the single current Edge | Candidate **`T_endpoint = max(Precision::Confusion(), BRep_Tool::Tolerance(vertex), BRep_Tool::Tolerance(edge))`** for the specific Vertex/Edge; **NOT** snapping to another Edge or creating persistent XYZ identity; verify OCCT Edge tolerance accessor compile availability and physical residuals |
| Unoriented tangent | `|D1| > T_derivative` in derivative's actual native parameter units; finite normalized representative. Two signs represent one line | `T_derivative` cannot be presumed equal to geometric `T_length` for arbitrary parameterization; exact parameter/derivative-units policy requires Owner review |
| Near full-circle/closed arc | Native topological END identity and finite bounded span; compare angular residual `delta = 2π - span` (radians) **and** physical near-closure gap `2*r*sin(delta/2)` (**mm**) | Decide whether geometrically open but nearly closed with indistinguishable physical terminals must be `closed_or_periodic` or `degenerate_edge`; never round up a tiny valid gap to a full closed circle without stated policy |

**Owner chose N1a as the preferred policy to prove first; N1b is a contingency only, NOT authorized by that preference. Neither option contains Owner-accepted exact values for every predicate:**

- **N1a — OWNER-PREFERRED COMPARISON POLICY TO TEST (not implemented):** `T_length = Precision::Confusion()` (length in mm), `T_angle = Precision::Angular()` (radians), per Vertex/Edge `T_endpoint = max(provider linear precision, native Vertex tolerance, native Edge tolerance)`; derivative validity uses a separately justified scale or analytical `D1` regularity test, **not a casual mm epsilon**.
- **N1b — minimum model-domain policy (alternative requiring broader modeling decision):** explicit SS2 D2 model-unit length and angular thresholds independent of OCCT defaults but still never overriding source-authorized native topology/Vertex tolerances. This alters modeled acceptance semantics across scale and must be reviewed against ADR-0016/0017 and existing E10 measurements. **Do not introduce it automatically under Q0.**

**Read-only evidence provenance:** `src/kernel_occt/solid_modeling_kernel.cpp` already uses `Precision::Angular()` to compare planar normals and `max(Precision::Confusion(), BRep_Tool::Tolerance(Face1), BRep_Tool::Tolerance(Face2))` for planar Face distance; current 2D Edge projection uses OCCT precision for collapse/circular span. Native `tests/kernel_native/pm00a_e10_numerical_policy_evidence_test.cpp` already sweeps gaps around `Precision::Confusion()`, but explicitly calls the findings **evidence, NOT an accepted SS2 modeling tolerance**. Neither proves the new 3D Q0 carrier/Vertex checks.

**Finite future numeric witnesses within allowed two native tests:** (N-01) actual Line lengths above/below selected threshold, provider-updated height and carrier↔Vertex residuals; (N-02) true circular Arc radii across e.g. sub-mm, 10 mm and large mm scales, positive/negative trims and seam crossing, exact half-bounded length; (N-03) residual span `delta` on both sides of candidate `T_angle`, and closure gap on both sides of candidate `T_endpoint`, real native topology distinct/closed; (N-04) same Body/generation replay determinism and changed Body refusal; (N-05) genuinely invalid `D1`/zero-length if constructible without out-of-scope helper. All are **UNRUN** for Q0; no numeric acceptance can be asserted by citing historical E10 or E0.

**N1 direction chosen, but numeric acceptance STOP remains:** Owner prefers **N1a**. Before executable Work Contract acceptance, Owner must first approve an **exact provisional testable N1a comparison rule** for `T_length`, `T_derivative`, `T_endpoint`, `T_angle`, Vertex/Edge tolerance validity/units and statuses for near-closed but distinct terminals. Actual new Q0 native witnesses and threshold results require **separately accepted/merged Work Contract + dedicated ACTIVE activation**: they must NOT be falsely demanded before code/test authority exists. N1a uses OCCT precision as a **testable comparison-policy candidate**, not an already accepted threshold for all modeling scales. If any witness contradicts this policy, return the concrete result for Owner D2 revision; do not silently switch to N1b.

### E1 — E1a PREFERRED IN PRINCIPLE by Owner; test coverage/sufficiency still UNPROVEN

**Existing proof, NOT transferable:** `src/kernel_occt/include/.../detail/edge_projection_exception_guard.hpp` implements a templated `guardedExactEdgeProjection` returning **old `EdgeProjectionResult`**; `src/kernel_occt/profile_face_evidence.cpp::buildEdgeProjectionExceptionEvidence()` throws actual `Standard_Failure` and `std::runtime_error` through that same old guard, and `pm02p.e_kernel_lifecycle` asserts `kernel_failure`/empty. The as-built `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` documents this previous 2D proof. **None is Q0's new `ExactEdge3dResult` exception test**, and those provider evidence adapter files are NOT in the proposed six-file allowlist.

- **E1a (OWNER-PREFERRED TO TRY FIRST, still conditional on proof within six candidate paths):** put ONE small pure, exception-to-typed-result **provider detail guard helper** inside the already allowed **existing OCCT provider header** (or the new allowed header where it can remain provider-neutral), and invoke the **same exact helper** in the new `queryExactBoundedEdge3d()` production body and the already allowed `pm02p_e_kernel_lifecycle_test.cpp`. Test a deterministic throwing `std::runtime_error` callable plus success passthrough and typed empty failure. `catch(...)` is type-agnostic for C++ exceptions including `Standard_Failure`, but **without a real new-Q0 Standard_Failure injection test, report that precise subcase as NOT DIRECTLY TESTED**, not as native OCCT PASS. The helper is not a new user-facing mode, UI setting or persisted fault knob. **Owner must specifically accept this limited witness scope if sufficient.**
- **E1b (stronger OCCT-type-specific proof, NEEDS OWNER SCOPE AMENDMENT if E1a insufficient):** place a provider-private Q0 exception evidence adapter where native OCCT `Standard_Failure` and unrelated exceptions can be deliberately raised through the **same Q0 guard** without adding a public test-injection switch to `IExactBoundedEdgeGeometryQuery`. The existing `profile_face_evidence.cpp` and its header are a proven pattern, but they are **outside** the six proposed paths. Any required extra paths must be enumerated and Owner-approved explicitly **before** D0 coding; do not simply mutate this existing module under the current Draft.
- **E1c (not acceptable as completion):** merely add `try/catch` to the OCCT .cpp and exercise `invalid_input`/degenerate curve without ever throwing. These exercise typed negative results but **do not test exception conversion**. Report `Q0-06 exception_injection=BLOCKED`, not PASS.

**E1 direction chosen, but test-evidence STOP remains:** Owner prefers **E1a**, with one **shared exact new-Q0 production guard** and a future deterministic throw/success witness. Test must demonstrate new typed `kernel_failure`, **empty result**, no escaped exception and preserved success, not just input rejection. A real OCCT `Standard_Failure` injection through that *same new guard* remains **NOT DIRECTLY TESTED** unless independently measured; explicitly record that limitation in Q0-06. If the contract's later accepted acceptance criteria require actual new-Q0 Standard_Failure proof, or E1a cannot be built/tested within the six paths, STOP and seek an exact Owner E1b allowlist amendment *before* production/test mutation. No runtime fault toggle, new command, hidden test backdoor or test weakening.

### N1a/E1a proof ledger — accepted direction, ZERO new Q0 test executions

| Gate | Owner-preferred method | Required observable proof when separately activated | Status now |
| --- | --- | --- | --- |
| N1a-01 | OCCT model-length/angular comparisons + actual Edge/Vertex tolerance | Report native analytic curve lengths, terminal↔carrier residual in **mm**, configured `Precision::Confusion()` and `Precision::Angular()` values, `BRep_Tool` tolerances and predicted vs measured accept/reject result for both paired endpoints | **UNRUN** |
| N1a-02 | Analytic Line / circular bounded Arc only | Real OCCT 3D MID, three unoriented tangents, open seam-positive/negative orientation, radius/length scaling; physical terminal order independent | **UNRUN** |
| N1a-03 | Closed/near-closed rejection | Vary 3D physical END gap and angular span around candidate bounds; record rejected vs supported curve/status before choosing accepted strict policy | **UNRUN** |
| E1a-01 | Same guard used by production and test | Actual future Q0 result guard returns `kernel_failure` / null payload for deliberate `std::runtime_error`, no exception escapes; success result roundtrips; **prove identity of guarded callable in production source** | **UNRUN** |
| E1a-02 | OCCT-specific throw via same Q0 guard | If no directly instrumented `Standard_Failure` test fits six files, record **NOT DIRECTLY TESTED**. If direct proof is mandatory, request E1b exact-path amendment; never infer from old 2D tests | **OPEN — NOT PROVEN** |

**Q0-06 is not a single unconditional green checkbox:** separate results for numeric/degenerate cases, generic exception conversion, explicit OCCT-specific exception coverage and no escaped exception. The existing historic E10/PG-01A runs remain separate background, not Q0 PASS. If Q0-06 is essential for closure, no final Q0-06 PASS may be asserted while a required subcase remains UNRUN/BLOCKED.

### N/E gate outcome for present Draft

The Q0 three-tangent/eight-status/analytic-MID **direction is accepted in principle**; **N1a and E1a are Owner-preferred methods, not finished validation**. The six-file set remains a candidate and is **NOT ACTIVATED**. Exact **provisional and testable** N1a thresholds / statuses and E1a exception-coverage obligations must receive separate Owner D2 approval in the complete Work Contract. Then PR-specific exact-HEAD merge and separately accepted/merged dedicated `ACTIVE.yaml` activation authorize limited coding/tests; **actual numeric witnesses and Q0-06 exception results are required for downstream implementation acceptance**, not falsely pre-claimed before activation.

### Phase-correct evidence ordering — no impossible pre-activation Q0 test gate

**Before implementation authority:** Owner must accept exact *hypotheses and observable acceptance criteria*, not non-existent new Q0 native test results. The six proposed writable file paths and precise shared-guard location, model-unit/angle thresholds, near-closed status and exception-coverage limits must be independently approved in a complete Work Contract and merged by separate exact-head consent. A subsequent separately authorized `ACTIVE.yaml` PR must be merged before code/test mutation.

**After independently merged activation ONLY:** a bounded implementation branch may add the provider-neutral Q0 interface and native witnesses in its approved paths, actually measure `N1a-01..03` and `E1a-01..02`, run FOCUSED/FAST/Ready-FULL on exact committed heads, and report physical geometry/error/status results. An earlier `work/**` governance PASS measures **no** native Q0 tests. If actual results require revising an accepted N1a threshold or E1a coverage contract, STOP, request Owner D2 amendment and rerun. **Do not reclassify unsupported evidence as PASS.**

**Distinguish the two approvals:** accepting a *provisional, testable* D2 numeric/exception procedure allows a future test only **after ACTIVE**; accepting measured D0 behavior comes **after those future tests**. These are not equivalent.

## 2C. Owner-accepted PROVISIONAL N1a/E1a test criteria — NOT IMPLEMENTED OR TESTED

Owner explicitly accepted the following **§2C N1a-P1..P5 and E1a-P test hypotheses** on 2026-10-13, including that **direct new-Q0 Standard_Failure injection is NOT DIRECTLY TESTED**. They are provisionally accepted for a future precise work contract, not as numeric test PASS, C++ API freeze or permission to edit sources. If native evidence contradicts a rule after activation, STOP and seek Owner D2 amendment.

### N1a-P — provisional Owner-accepted numerical hypotheses, Q0 native tests UNRUN

**Units:** model length and physical residuals in **mm**, angles in **radians**. Let `p=Precision::Confusion()` (mm), `a=Precision::Angular()` (radians), require both finite and strictly positive. For each physical owned endpoint Vertex `v_i`, propose `T_endpoint_i=max(p, BRep_Tool::Tolerance(v_i), BRep_Tool::Tolerance(edge))` mm, checking native tolerance values finite/nonnegative. OCCT **public reference headers** declare `BRep_Tool::Tolerance(const TopoDS_Edge&)` and `Tolerance(const TopoDS_Vertex&)` ([OCCT API reference](https://occt3d.com/dev/doc/refman/html/class_b_rep___tool.html)); therefore the proposed Edge/Vertex overloads are **API-grounded, but SimpleSolid2's exact source/dependency version still requires future actual Windows compile and native measurement**. Neither public API existence nor old Face-tolerance usage validates the new Q0 numerical acceptance rules. These constants are provider-derived *local comparison hypotheses*, **not** a new global SS2 Sketch/OSNAP/modeling tolerance.

1. **Native Vertex ↔ analytic carrier pairing:** for current bounded Edge only, obtain two native physical terminal points `P_i` and `BRepAdaptor_Curve::Value(first/last)` endpoints. Allow the **two permutations only within that same Edge**; require one unequivocally valid position-to-trim assignment with per-Vertex `|P_i-Value(boundary_j)|<=T_endpoint_i`. If neither fits, `kernel_failure` (inconsistent native accounting); if **both permutations fit**, reject the ambiguous END↔analytic tangent correspondence as `degenerate_edge`, even if the runtime reported two different native Vertices. No cross-Edge proximity inference, persisted XYZ or native slot identity. Audit `T_endpoint_i` and residuals at both ends.
2. **Bounded `GeomAbs_Line`:** require finite trim and analytic endpoints, `L=norm(Value(last)-Value(first))>p` mm, distinct native current Vertices and physical separation `>max(T_endpoint_0,T_endpoint_1)`. Derive **true curve MID** via `Value((first+last)/2)` on this certified constant-speed Line; do **not** take an unvalidated arithmetic midpoint of topological Vertex coordinates. Obtain analytic unoriented tangent lines at both trims and MID.
3. **Genuinely open `GeomAbs_Circle` Arc:** require finite `radius>p`, native `span=last-first` with `0<span<2π`, `L=radius*span>p`, two distinct owned current terminal Vertices, valid carrier/Vertex pairing, and END/MID `D1`. Constant Circle speed makes `MID=Value((first+last)/2)` a half-**bounded-arclength** point. Reject actual topologically closed/same-Vertex or full-turn Edge as `closed_or_periodic`; do **not** reject a distinct open trimmed Edge solely because its Circle carrier is periodic. For a mathematically open `span<2π` whose native **`END gap <= max(T_endpoint_i)`**, propose `degenerate_edge`; if END gap is larger, allow near-full open arc even when `2π-span<a`, subject to strict native trim proof. Native invalid spans (>full turn, negative or nonfinite) are `kernel_failure`, not an implicit opened circle.
4. **Derivative validity with correct dimensions:** the native `D1` is a derivative with respect to its curve parameter, **not a generic length in mm**. For certified analytic Line/Circle require finite `D1` and a strictly positive norm, then normalize to a sign-free tangent-line representative. Do not compare `|D1|` with `p` mm or invent `T_derivative=p`; Line has regular constant direction, Circle has regular derivative when its radius is valid. If real supported geometry underflows or `D1` is invalid, mark `degenerate_edge` and STOP to review a unit-correct additional rule.
5. **Atomic status mapping:** `invalid_input` for null/bad capture; `provider_mismatch` for wrong provider/Body object; `edge_unavailable` for missing inventory member; `unsupported_curve` for non-Line/non-Circle; `closed_or_periodic` for genuine closed/full-turn circle; `degenerate_edge` for length/radius/END gap/derivative failure; `kernel_failure` for malformed carrier/topological accounting or exception. `ok` only with finite complete geometry; **all non-ok require empty payload** and no public C++ throw. Tangent representative ±sign and endpoint indices are never persisted.

**Numerical witness gate after activation:** sample real Line lengths/radii both sides of `p`, native carrier↔Vertex residuals both sides of `T_endpoint_i`, open Arc trim span/END gap near full-turn/angle seam, clockwise/counterclockwise trims and cold Body edit. Log actual parameter units, provider tolerances and status. In any contradiction: STOP, ask Owner D2 for a policy amendment; do not quietly change magic thresholds. **All Q0 numeric tests are currently UNRUN.**

### E1a-P — provisional Owner-accepted shared-guard test plan, Q0 native test UNRUN

In the existing allowed `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp`, propose a **distinct provider-detail** templated `guardExactBoundedEdge3dResult(callable)` mapping any escaped C++ exception to a **new-Q0** `{ExactEdge3dStatus::kernel_failure, nullopt}` under `noexcept`. The new production `queryExactBoundedEdge3d()` in the existing OCCT `.cpp` **must use this same helper** around actual native analytic evaluation. Never alter the existing old `guardedExactEdgeProjection()` or its `EdgeProjectionResult` ABI.

In the existing allowed `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp`, invoke **that exact same helper** with one deterministic `std::runtime_error`-throwing callable and one successful well-typed Q0 result; assert no escaped exception, typed `kernel_failure` with **no geometry**, success passthrough, and verify the production method truly uses the shared helper. This tests new Q0's **generic C++ exception conversion**, not native OCCT fault injection. `catch(...)` syntactically includes `Standard_Failure`, but unless directly injected through **this same new guard**, document `Standard_Failure = NOT_DIRECTLY_TESTED`, never claim it passed due to earlier 2D tests. If direct OCCT-specific fault injection is mandatory for Q0 closure and the six-file scope cannot perform it, request **Owner E1b exact-path scope amendment before editing product or tests**. Do not introduce a runtime fault toggle or test-only public API.

### §2C sign-off: provisional test rule design complete; executable gate independent

A. **OWNER-ACCEPTED PROVISIONALLY:** N1a-P1..P5 (physical native END vs analytic carrier, actual per-Edge/Vertex tolerance, mm/radian distinctions, distinct-open-vs-truly-closed Arc and sign-free tangent). The acceptance is of **falsifiable test hypotheses only**, not results. All native new-Q0 cases UNRUN.

B. **OWNER-ACCEPTED PROVISIONALLY:** E1a-P calls the **same Q0-specific** production/test exception guard and tests deliberate generic C++ throw and passthrough. The direct **new-Q0 OCCT Standard_Failure** case remains explicitly **NOT DIRECTLY TESTED**; it may not be reported as PASS without new proof. Scope amendment E1b only if direct native-specific injection becomes mandatory.

C. **STILL OPEN:** final exact C++ declarations (including fully defined scoped class and distinct collision-free method names), optional result and sign-invariant tangent implementation, complete six exact production/native-test/DOC-01 writable paths and documented scope feasibility. §2C acceptance did NOT approve the executable contract, specific PR merge or ACTIVE switch.

## 2D. Source-verified final C++ header feasibility — PREPARATION, not accepted executable contract

**Read-only C++ issue:** the §2A illustrative `class ScopedExactEdge;` declaration alone is NOT a final complete definition. The real existing `edge_projection.hpp` defines a **complete** `ScopedProjectionEdge` value, with default-invalid state, private `RuntimeSolidHandle`+token constructor, friend query interface and protected factory, **before** it returns `std::optional<ScopedProjectionEdge>`. The proposed new neutral `std::optional<ScopedExactEdge>` similarly needs a *complete* scoped type, proper public `valid()/sourceBody()/edge()`, private constructor and a factory accessible only to the new query interface. Do not implement a merely forward-declared class as a finished return-value API.

**Candidate implementation detail for the later independent exact-header Owner gate:**
- `ScopedExactEdge final` encapsulates `RuntimeSolidHandle` and `RuntimeEdgeToken`; default invalid, never persisted. Only `IExactBoundedEdgeGeometryQuery` can create a valid binding via a protected static `makeScopedExactEdge`.
- `IExactBoundedEdgeGeometryQuery` declares `bindExactEdgeToBody(Body,token)` and `queryExactBoundedEdge3d(Body,ScopedExactEdge)`; both names are distinct from existing **2D** `IEdgeProjectionQuery::bindEdgeToBody`. No return-type-overload collision or change to `ISolidModelingKernel` ABI.
- `ExactBoundedEdge3d` contains `std::array<Point3,2>` paired native transient terminal positions and `UnorientedUnitTangent3` representatives for each END plus MID. All unit-tangent comparisons must be **sign-insensitive**; the representative's sign is NOT a persistent Frame3 or P4 preview choice. Include the necessary standard headers (`<array>`, `<optional>`, `<utility>`) explicitly.
- Only one new neutral header is proposed and can be exposed through already existing PUBLIC kernel include directory; existing OCCT .cpp and header must implement the additional interface without CMake/CI/source growth. This is **source inspected, not compiled or tested**. If the exact C++ design cannot be implemented inside six prospective allowed files, STOP for Owner D2 amendment.

**Remaining independent Owner decision:** §2D interface direction and six exact proposed future source/test/docs paths were accepted by Owner **FOR CONTRACT PREPARATION ONLY**. The complete exact-header proposal and integrated executable Q0 Work Contract text still need **independent exact-text acceptance**. Do NOT mark PR #352 Ready just because §2C/§2D directions were accepted; PR-specific exact-HEAD squash merge and dedicated ACTIVE activation remain separate and future.

## 2E. Full C++ neutral-header candidate for separate exact-text Owner review — NOT COMPILED

This *illustrative complete declaration set* supersedes the incomplete `class ScopedExactEdge;` shorthand in §2A for final interface review. The Owner accepted §2D's **direction** and the six future file paths as a proposed bounded scope, **not this specific API text or the full Work Contract as executable**.

```cpp
// PROPOSAL ONLY: src/kernel/include/simplesolid2/kernel/exact_bounded_edge_geometry.hpp
#pragma once

#include <simplesolid2/kernel/solid_modeling.hpp>
#include <array>
#include <optional>
#include <utility>

namespace simplesolid2::kernel {

enum class ExactEdge3dStatus {
    ok,
    invalid_input,
    provider_mismatch,
    edge_unavailable,
    unsupported_curve,
    closed_or_periodic,
    degenerate_edge,
    kernel_failure,
};
enum class ExactEdge3dKind {
    bounded_line,
    open_circular_arc,
};

// The geometry is one unoriented line {+t,-t}.
// Point3 merely stores three components, NOT a source point or signed frame.
struct UnorientedUnitTangent3 final {
    Point3 representative;
};
struct ExactBoundedEdge3d final {
    ExactEdge3dKind kind{ExactEdge3dKind::bounded_line};
    // Position[i] and tangent[i] belong to the SAME current physical terminal.
    // The index is NEVER a durable END key; order may change on rebuild.
    std::array<Point3, 2> current_terminal_positions_world{};
    std::array<UnorientedUnitTangent3, 2> terminal_tangent_lines{};
    double bounded_length_mm{};
    Point3 midpoint_at_half_bounded_length_world{};
    UnorientedUnitTangent3 midpoint_tangent_line{};
};
struct ExactEdge3dResult final {
    ExactEdge3dStatus status{ExactEdge3dStatus::invalid_input};
    std::optional<ExactBoundedEdge3d> geometry;

    [[nodiscard]] bool ok() const noexcept {
        return status == ExactEdge3dStatus::ok && geometry.has_value();
    }
};

class ScopedExactEdge final {
public:
    ScopedExactEdge() = default;

    [[nodiscard]] bool valid() const noexcept {
        return source_body_ != nullptr && edge_.valid();
    }
    [[nodiscard]] const RuntimeSolidHandle& sourceBody() const noexcept {
        return source_body_;
    }
    [[nodiscard]] RuntimeEdgeToken edge() const noexcept {
        return edge_;
    }

private:
    friend class IExactBoundedEdgeGeometryQuery;
    ScopedExactEdge(RuntimeSolidHandle body, RuntimeEdgeToken token) noexcept
        : source_body_{std::move(body)}, edge_{token} {}

    RuntimeSolidHandle source_body_;
    RuntimeEdgeToken edge_;
};

class IExactBoundedEdgeGeometryQuery {
protected:
    [[nodiscard]] static ScopedExactEdge makeScopedExactEdge(
        RuntimeSolidHandle body, RuntimeEdgeToken token) noexcept {
        return {std::move(body), token};
    }
public:
    virtual ~IExactBoundedEdgeGeometryQuery() = default;

    [[nodiscard]] virtual std::optional<ScopedExactEdge>
    bindExactEdgeToBody(
        RuntimeSolidHandle source_body,
        RuntimeEdgeToken current_edge) noexcept = 0;

    [[nodiscard]] virtual ExactEdge3dResult queryExactBoundedEdge3d(
        RuntimeSolidHandle current_body,
        const ScopedExactEdge& edge) noexcept = 0;
};

} // namespace simplesolid2::kernel
```

**Review obligations, not proof:**
- This complete value declaration precedes `std::optional<ScopedExactEdge>` and copies the valid/default-invalid scoped-binding pattern from already shipped 2D `ScopedProjectionEdge` without persisting Body/tokens. Correct generation authorization still requires runtime Body object equality and actual strict Part-stage source provenance before binding.
- The independently named `bindExactEdgeToBody` does NOT collide with existing 2D `bindEdgeToBody`. `OcctSolidModelingKernel` would inherit this new interface without altering the older interface or `ISolidModelingKernel`. The binder's `nullopt` is deliberately untyped like the 2D binder; the eight statuses belong to **query results**, not binding failures. If typed binding failure becomes required, STOP for Owner D2 amendment.
- `UnorientedUnitTangent3` is a sign-invariant CONTRACT, although its three numeric components have an arbitrary representative sign. The provider MUST check finite unit norm and compare tangent lines up to ±; no Q0 output determines persisted Datum `N`, END key or user Reverse.
- Every unsuccessful query returns no geometry, source/Body ownership is current-generation only, and the shared **E1a** guard controls all native exceptions. The direct new-Q0 `Standard_Failure` case remains `NOT DIRECTLY TESTED` unless later actual evidence is obtained; old 2D injection evidence cannot count.
- This is a **source-grounded declaration proposal, not an actual compiled header**. The six paths remain exactly the accepted prospective set from §3. New Q0 native FOCUSED/FAST/FULL, E1a and N1a measurements, link feasibility and current DOC-01 as-built update are all **NOT RUN**; these occur only after separately approved executable contract merge and ACTIVE activation.

**Next Owner gate:** separately accept/revise this exact header proposal and the full integrated Q0 executable Work Contract before Ready-for-Review. This does not authorize PR #352 squash merge or Q0 ACTIVE switch.

## 3. Six Owner-accepted prospective future source/test/docs paths — executable Work Contract and ACTIVE still gated

| Exact file | Permitted narrow mutation only AFTER accepted activation |
| --- | --- |
| `src/kernel/include/simplesolid2/kernel/exact_bounded_edge_geometry.hpp` (NEW candidate) | Provider-neutral geometry-only typed result, error enum, opaque scoped exact Body+Edge and independent exact 3D query interface |
| `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp` | Advertise/override independent interface on existing `OcctSolidModelingKernel`, no old interface semantic change |
| `src/kernel_occt/solid_modeling_kernel.cpp` | Guarded current-trim 3D Line/open Arc geometry, **paired transient terminal/tangent lines** and MID tangent, fail-closed provider/edge/generation validation; no durable source or sampling |
| `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` | Existing registered **CTest `pm02p.e_kernel_lifecycle`** real-OCCT analytic Line/open Arc, trim/seam, invalid provider, degenerate, exact metric/source-generation checks; retain old projection/lifecycle tests |
| `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` | Existing registered **CTest `pm05c2b.part_edge_topology`** bind only after Part certifies a real strict `MaterialEdgeReference` in the correct source Body stage; current provider, provider rebuild and height edit; preserve all E0 evidence |
| `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` | Existing verified DOC-01 canonical as-built `doc-id: internal.part-edge-projection-kernel`: describe **new distinct 3D geometry-only query** while preserving old exact **2D projection**, Face Boundary contracts and stable section markers |

**Owner accepted exactly the six paths listed above **as proposed future contract scope**, NOT permission to edit them before a separately accepted full Work Contract and independently merged Q0 ACTIVE activation. No seventh mutation path by inference.** Not allowed: src/part, src/application, UI/Viewer, Part schema, other production tests/docs, CMake, CI, generated Browser, ACTIVE, ADR or roadmap. Existing E0 tests must not be weakened.

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

**Owner accepted the §2A three-tangent/eight-status/analytic-MID DIRECTION and separately N1a/E1a as preferred, conditional methods.** Exact C++ types, numerical thresholds, exception coverage proof, six-file executable authority and PR merge/activation are still independent. Generic “continue” does NOT authorize code or these further gates. Independent gates:

1. **DONE only as planning/architecture gates:** Owner accepted Q0→Q1→Q2+ order, §2C provisional test hypotheses, §2D interface direction and six exact future-scope paths on 2026-10-13. This is NOT exact final Work Contract approval. Planning PR #351 remains OPEN/DRAFT/UNMERGED.
2. **THIS draft executable-contract proposal:** Owner still must independently accept the complete integrated C++ header/API contract and exact final Work Contract text. Six paths are accepted as candidate FUTURE scope but not writable now. Verify remaining native fixtures/build/test feasibility STOPs and declared E1a Standard_Failure direct-coverage limitation; work-only CI is not code/test proof.
3. **Governance contract merge:** once Owner explicitly accepts the final exact Work Contract text, make the one-document Work Contract PR Ready for Review, require work-only exact-head Windows closure, request **separate PR-specific squash merge authorization**. The accepted scope alone is NOT merge permission.
4. **Separate dedicated activation:** propose minimal `work/ACTIVE.yaml.active_work` switch from the E0 evidence-only contract to this accepted Q0 contract. Preserve roadmap v1.33, Foundation and history pointers. Obtain Owner **scope approval**, Windows governance at exact HEAD, then **separate explicit squash merge consent**. No Q0 code/test mutations until merged activation.
5. **Only then D0 coding:** new code/test branch from active `main`, exact file allowlist, actual Windows FOCUSED/FAST/FULL, measured evidence and scope/source audit. If any FOCUSED or native test fails, diagnose; no extra helper, test weakening, ownerless API or broad refactor without new approval.
6. **Owner technical acceptance and independent implementation PR merge** after measured results. A successful Q0 geometry API is **not** proof of persistent Edge-local END (Q1), authored Datum/schema/frame/Undo/UX (Q2+) or release readiness.

**Must STOP and seek fresh D2 acceptance** for: exact public API or tolerance not settled, test fixture missing within allowlist, CMake header integration required, provider-private exception adapter outside scope, unknown/ambiguous current strict material Edge, out-of-stage input, actual full-circle unsupported semantics contradicted, request to persist runtime END positions, any change to accepted v16 Part schema/ModelingSemantics version or Owner user-facing one-constructor UX.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: **If separately accepted and activated**, Q0 adds a new provider-neutral exact 3D Kernel query boundary, runtime ownership/generation and typed failure semantics; therefore it requires the exact existing `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` current-state update under DOC-01. No user-visible Datum feature, new command, UI, persistence or Product Browser is changed within Q0; paired PL/EN Product updates are deferred to independently approved Q2+ implementation. Documentation verification and as-built current-state accuracy are completion gates, not optional append-only history.

**Next distinct Owner decision:** accept/revise the COMPLETE exact C++ header and integrated executable Q0 Work Contract text (not another vote on previously accepted §2C/§2D principles). This is separate from PR #352 exact-HEAD squash merge permission, independently merged ACTIVE and any coding.
