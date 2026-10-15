# RG-02C/A0 — straight Body material Edge as a prospective Axis source: native evidence-only Work Contract PROPOSAL

**Status:** PROPOSED FOR SEPARATE OWNER D2 REVIEW — NOT ACCEPTED / NOT MERGED / NOT ACTIVE / NO TEST OR PRODUCT MUTATION AUTHORITY.
**Date:** 2026-10-15.
**Baseline:** protected `main` `45cc16afe03692e130076b2d84ae1c2bc68eb595` after S4B Owner technical FINAL PASS and guarded production PR #402 squash. Independent ACTIVE-only S4B historical-closeout Draft PR #403 remains a separate governance gate; neither that PR nor this proposal activates an Axis package.
**Program:** Owner-accepted Part Modeling roadmap v1.33 §6/§20B, RG-02 Datum Plane + Axis next *contract planning* focus only.
**Authority:** Constitution → Foundation v1.0 FROZEN / Architecture → accepted ADR-0012/0013/0014/0016/0017 → roadmap v1.33 → active `work/ACTIVE.yaml` (still S4B historical pointer) → accepted bounded contract → tests. PM-04 Axis semantics and native Q0/Q1/Q2 strict material-Edge evidence are relevant precedent, not new Axis implementation authority.
**Design source:** `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` §2.3(1): Owner's agreed future constructor “Along one straight Body Edge.” That broad design record is NOT an executable contract.

## 1. Why this one small next slice

The already delivered PM-04 authored Axis source is a **Sketch Line only** (`PartAxis::source = SketchLineAxisSource`), plus intrinsic Origin X/Y/Z axes. A directly Body-Edge-derived Axis requires new Part/domain meaning, an explicit Body-stage dependency and eventually separate D2/UX/schema/lifecycle authorization.

Before any such product change, prove **read-only** that the already accepted strict `part::MaterialEdgeReference` and current exact `kernel::IExactBoundedEdgeGeometryQuery` can provide one unambiguous **unoriented infinite analytic line** at the authored Body stage. This avoids making an unsupported promise about all native Edges or adding a new Point subsystem. The test is deliberately smaller than full RG-02C Axis delivery.

**No new Axis capability is delivered in A0.** This is a proposed feasibility gate, not D2 acceptance of a future Axis persistent source variant.

## 2. Exact prospective A0 evidence scope — subject to Owner acceptance

Use one **already registered** real OCCT Part native target `pm05c2b.part_edge_topology`. Add cases to its existing source, preserving every prior test:

1. **A0-01 / positive strict line:** evaluate an actual ordered one-Body Extrude stage and enumerate strictly admissible material Edges; author the Part-owned `MaterialEdgeReference` only where existing resolver is uniquely Resolved and query the same current native Body object with `bindExactEdgeToBody` / `queryExactBoundedEdge3d`. Require `ExactEdge3dKind::bounded_line`, two finite/distinct physical terminals, positive finite bounded length, finite midpoint and a valid nonzero unoriented unit tangent line. Record counts and reasons for every skipped candidate; zero false Resolved.
2. **A0-02 / source authority:** prove that the line is derived solely from the strict semantic Edge at the explicit `BodyStageRef`; forbid viewport XYZ, geometry-similarity rebinding, runtime token or native endpoint array index as durable source. Rebind the provider token only after fresh semantic resolution.
3. **A0-03 / orientation invariance:** compare infinite line geometry under equivalent native Edge reversal/realization **up to a ± direction sign**. No arbitrary “positive Axis” orientation; downstream Revolve retains its own direction/reversal meaning. If a reversal fixture cannot be produced without forbidden edits, report the gap, not a fabricated PASS.
4. **A0-04 / stage/revision:** test upstream feature edit while preserving the selected semantic Edge where strict resolution survives, and one genuinely changed/removed/ambiguous material-Edge case. On stale, missing or ambiguous stage/source, no line result may be published. Never silently rebind to the final-Body visually nearest Edge.
5. **A0-05 / unsupported and wrong provider:** demonstrate exact-provider rejection of non-linear analytic arcs, periodic/closed or nonmaterial/seam edges for **this line-only admission**; mismatched Body/scoped native lease must fail typed. Existing Q0 may return valid circular-arc geometry; A0 must still classify it as **not admissible for straight-Edge Axis**, not mutate Q0 into an error.
6. **A0-06 / independent rebuild:** recompute authored source and stage after native Part Save/Close/Reopen with a fresh OCCT provider; require strictly equivalent *unoriented* infinite line where source is resolvable. No native handle, token, endpoint ordinal, cached coordinate or “last-good Axis” survives into the persisted source.
7. **A0-07 / existing functionality:** retain legacy PM-04 Sketch-Line Axis/Revolve, S4A/S4B Datum, PM-05F and native topology checks unchanged. A0 adds assertions but cannot delete or weaken them.

All measurement is read-only in executable test code. No new `PartAxis`, `AxisReference` alternative, Command/Undo, persistence schema, GUI interaction, or product documentation claim.

## 3. Fixed proposed executable allowlist — NOT usable until separately activated

1. `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — **additive assertions in the pre-registered kernel-native target only**.
2. `work/RG-02C_A0_STRAIGHT_BODY_EDGE_AXIS_NATIVE_ACCEPTANCE_EVIDENCE.md` — new factual evidence report after tests run.

No additional files, test executable/CTest registration, `src/**`, Kernel/OCCT public or private API, `tests/CMakeLists.txt`, `.github/**`, scripts, schema, DOC-01 product Markdown or generated Browser, `ACTIVE.yaml`, roadmap or ADR modifications are permitted in execution. Any needed third path or wider API/change requires **STOP + independent Owner D2 amendment**, not a clever helper outside the allowlist.

## 4. Required acceptance and lifecycle gates

- **Before executable A0:** Owner separately accepts the precise A0-01…A0-07 scope and two-file allowlist; governance-only proposal passes exact-head Windows CI and gets separately authorized guarded squash. A distinct Owner-approved `ACTIVE.yaml`-only activation PR points to this merged contract, passes exact-head governance, and is separately squash-merged. Until then, not one test-source line may change.
- **After activation:** fresh test-only branch from protected `main`, real native OCCT FOCUSED `pm05c2b.part_edge_topology`, Draft FAST and **Ready FULL Core+Kernel+Desktop** on exact code/tests SHA, all original registered tests unmodified and running. Any RED or test fixture limitation remains visible in the evidence report. Work-only final HEAD receives exact-head governance CLOSURE if code/evidence SHAs differ.
- **Owner A0 technical PASS** and a separate guarded production-test PR merge are mandatory. No successful A0 evidence automatically creates a usable Axis constructor.
- **Future production/UX:** only after A0 and a further narrow Owner D2 decision about Axis source grammar, diagnostics, dependency cycle, typed persistence/migration, edit/repair and Revolve consumption can a separate vertical-slice Work Contract be proposed. The Owner-mandated **prior explicit RG-02 Axis UI consultation and approval** remains a hard STOP before any Workbench/Tree/Viewer/hover/Command Line edit.

## 5. Deliberately excluded or unproven

OUT: Axis through two Points, cylindrical patch Axis, two-plane intersection, curved Edge Axis, a `MID`/CENTER picker, new generic Point meaning, Datum Plane constructor extension, speculative universal Axis framework, solid Mirror/Pattern, Assembly/Drawing, PM-06 and CI #302 remediation. A0 cannot guarantee that a strict Edge survives every upstream edit or that every visually straight edge is admissible; ambiguous/unsupported classification is a valid fail-closed result, not a reason to relax the contract.

**Single point of failure:** an apparently valid selected material Edge has no uniquely reproducible stage-bound semantic identity or the current native query cannot recover a consistent exact line on independent rebuild. If this occurs, **STOP for Owner D2**, do not persist a token or infer source from matching geometry.

## Documentation impact

Internal docs: **not required** for A0 evidence-only tests.  
User/Product docs: **not required** for A0 evidence-only tests.  
Generated Browser: **not required** for A0 evidence-only tests.  
Reason: A0 adds read-only native evidence, not new current product semantics or UI. This governance-only D2 proposal itself modifies one `work/**` file. A separately accepted future executable Axis vertical slice must set its own DOC-01 obligations.
