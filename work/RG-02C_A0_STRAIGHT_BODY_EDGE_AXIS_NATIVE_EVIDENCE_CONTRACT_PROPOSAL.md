# RG-02C/A0 — straight Body material Edge as a prospective Axis source: native evidence-only Work Contract PROPOSAL

**Status:** **OWNER D2 SCOPE ACCEPTED (2026-10-15), including binding NO-DUPLICATION condition**; governance PR #404 still **OPEN / UNMERGED / NOT ACTIVE**. No code/test mutation authority until separately Owner-authorized governance squash and dedicated ACTIVE-only activation.
**Date:** 2026-10-15.
**Baseline:** product squash PR #402 merged as `45cc16afe03692e130076b2d84ae1c2bc68eb595`. S4B historical-closeout PR #403 independently passed Windows CLOSURE #3217 and guarded squash-MERGED protected `main` as `d19c5396add2b37a0086d4bd6e2201a7eccaaf10`. `work/ACTIVE.yaml` keeps the completed S4B contract as a history-only pointer; #403 did not activate Axis. This D2 proposal PR #404 was created from the immediately preceding main; its independent exact-head governance and future merge/activation gates remain mandatory.
**Program:** Owner-accepted Part Modeling roadmap v1.33 §6/§20B, RG-02 Datum Plane + Axis next *contract planning* focus only.
**Authority:** Constitution → Foundation v1.0 FROZEN / Architecture → accepted ADR-0012/0013/0014/0016/0017 → roadmap v1.33 → active `work/ACTIVE.yaml` (still S4B historical pointer) → accepted bounded contract → tests. PM-04 Axis semantics and native Q0/Q1/Q2 strict material-Edge evidence are relevant precedent, not new Axis implementation authority.
**Design source:** `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` §2.3(1): Owner's agreed future constructor “Along one straight Body Edge.” That broad design record is NOT an executable contract.

## Owner D2 acceptance and binding reuse constraint — 2026-10-15

**Exact Owner decision:** In response to the explicit question whether the narrowly revised PR #404 A0 D2 scope was accepted as the basis of the future **read-only native** Axis source evidence contract, Owner stated **`tak, akceptuje - nie powinnismy duplikować testów i istniejącej logiki ..`**. The exact updated proposal at PR #404 HEAD `70233036451874e670f087dd60a5494bb0ea12ff` is therefore **D2 SCOPE ACCEPTED**, with *nonduplication of existing tests and logic* an affirmative acceptance condition, not an optional code-style suggestion.

**Mandatory test/logic reuse:**
- Reuse existing `verifyQ0StrictPartStageGeometry`, `verifyQ2CanonicalSelectedMids`, production `authorMaterialEdgeReference` / `resolveMaterialEdgeReference`, scoped exact native `IExactBoundedEdgeGeometryQuery`, and existing native test fixtures wherever applicable. Do not copy their line/Edge/MID validation into a parallel test implementation, synthesize a duplicate resolver or reclassify their established PASS as new evidence.
- Add only assertions for **genuinely missing Axis-specific invariants**: geometry of the infinite **unoriented** line (including physical terminal/direction independence), preservation of same stage-bound semantic source through independent replay, and consumer-level fail-closed behavior for ambiguous/stale/curved material edges.
- Test setup may minimally invoke existing production APIs and fixtures; do not rebuild a second evaluator, helper library or fixture framework. If the incremental A0 proof cannot be delivered inside the accepted pre-registered test source plus one factual `work/**` report **without duplication**, STOP and request a narrowed Owner D2 amendment rather than creating parallel production logic, new targets, or unverifiable tests.
- State in A0 acceptance evidence **which checks were pre-existing Q0/Q2 witnesses** and **which new assertions uniquely prove A0**; retained tests must not be weakened, deleted or skipped.

**Consent boundary:** This Owner response accepted this bounded A0 D2 **proposal and reuse constraint only**; it did **not** authorize protected-main squash merge of PR #404, activation via `work/ACTIVE.yaml`, edits to the native test target, new `PartAxis` persistence, or UI/Command Line changes. All those require distinct governance / Owner gates below.

## 1. Why this one small next slice

The already delivered PM-04 authored Axis source is a **Sketch Line only** (`PartAxis::source = SketchLineAxisSource`), plus intrinsic Origin X/Y/Z axes. A directly Body-Edge-derived Axis requires new Part/domain meaning, an explicit Body-stage dependency and eventually separate D2/UX/schema/lifecycle authorization.

Before any such product change, prove **read-only** that the already accepted strict `part::MaterialEdgeReference` and current exact `kernel::IExactBoundedEdgeGeometryQuery` can provide one unambiguous **unoriented infinite analytic line** at the authored Body stage. This avoids making an unsupported promise about all native Edges or adding a new Point subsystem. The test is deliberately smaller than full RG-02C Axis delivery.

**No new Axis capability is delivered in A0.** This is a proposed feasibility gate, not D2 acceptance of a future Axis persistent source variant.

## 2. Exact Owner-accepted A0 evidence scope — NOT executable until governance merge + ACTIVE activation

Use one **already registered** real OCCT Part native target `pm05c2b.part_edge_topology`. Add cases to its existing source, preserving every prior test:

1. **A0-01 / novel Axis-line admission:** REUSE (do not clone) existing strict stage-bound native material-Edge author/query checks (Q0 already asserts 12 resolved bounded Line Edges in its rectangle fixture). For at least one previously certified straight material Edge, derive an **evaluation-local candidate infinite unoriented Axis line** from its exact bounded native Line (point on the Line + tangent modulo ± sign). Prove that this candidate has finite nondegenerate analytic support and is invariant under choosing either actual terminal as the representative point, with no provider slot/edge direction becoming authored meaning. Classify all selected candidates and rejected ones; no false Resolved. This adds an Axis-specific *line equivalence* assertion, not another Q0 geometry test.
2. **A0-02 / source authority:** prove that the line is derived solely from the strict semantic Edge at the explicit `BodyStageRef`; forbid viewport XYZ, geometry-similarity rebinding, runtime token or native endpoint array index as durable source. Rebind the provider token only after fresh semantic resolution.
3. **A0-03 / orientation invariance:** compare infinite line geometry under equivalent native Edge reversal/realization **up to a ± direction sign**. No arbitrary “positive Axis” orientation; downstream Revolve retains its own direction/reversal meaning. If a reversal fixture cannot be produced without forbidden edits, report the gap, not a fabricated PASS.
4. **A0-04 / stage/revision:** test upstream feature edit while preserving the selected semantic Edge where strict resolution survives, and one genuinely changed/removed/ambiguous material-Edge case. On stale, missing or ambiguous stage/source, no line result may be published. Never silently rebind to the final-Body visually nearest Edge.
5. **A0-05 / Axis-specific line-only refusal:** REUSE already registered Q0/Q2 negative assertions for closed/periodic/mismatched-provider/native inventory cases. Add only the **new** admission assertion that an otherwise valid Q0 open circular arc is still **not a straight-Line Axis**; unresolved, ambiguous, nonmaterial or seam Edge can never publish this candidate Axis line. Distinguish upstream Q0 typed native failure from the proposed *consumer-level* line-only rejection; do not change existing Q0 statuses or provider tests.
6. **A0-06 / independent semantic Edge-to-line replay:** REUSE the existing test target's native Save/Close/Reopen mechanics (Q2 already exercises them for existing Datum source, which is **not** proof of a hypothetical Axis source). Retain an approved test-only stage-qualified `MaterialEdgeReference` value and rebuild the corresponding native Part from persisted authored Part data with a fresh OCCT provider; re-resolve that semantic Edge and assert **equivalent unoriented infinite Line**, not merely equivalence of current MID or existing Datum Frame. Test-only source retention is NOT serialization of a new Axis or a new product schema. If the strictly same Edge cannot be recovered, record the typed gap and STOP.
7. **A0-07 / existing functionality:** retain legacy PM-04 Sketch-Line Axis/Revolve, S4A/S4B Datum, PM-05F and native topology checks unchanged. A0 adds assertions but cannot delete or weaken them.

All measurement is read-only in executable test code. No new `PartAxis`, `AxisReference` alternative, Command/Undo, persistence schema, GUI interaction, or product documentation claim.

## 2A. Existing upstream witnesses and the required NEW delta (Owner no-duplication constraint, 2026-10-15)

**Do not relabel completed native Q0/Q1/Q2 tests as a new Axis A0 proof.** Source audit of the already registered `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` shows:

- `verifyQ0StrictPartStageGeometry` (around source lines 1070–1125) already authors strictly Resolved `MaterialEdgeReference` from the current Body-stage catalog, re-resolves before native scoped exact geometry query, certifies **12** current analytic bounded straight Edges on a rectangular Extrude and checks their MID against physical terminals. This is a **pre-existing PASS witness for Q0**, not a new A0 deliverable.
- `verifyQ2CanonicalSelectedMids` (around lines 1365–1430) already certifies native Edge-local MID and unoriented unit tangent for the same 12 source Edges. Do not copy its checks into A0 and call them original.
- Existing native `PartDocumentStore` cold Save/Reopen regressions (including Q2 and S4A) establish that provider-native tokens are not authored in **existing Datum** records. They do **not** by themselves prove replay of the proposed **infinite Axis Line** sourced directly from a semantic Body Edge.
- Delivered `src/part/include/simplesolid2/part/axis.hpp` still defines `PartAxis::source` solely as `SketchLineAxisSource`; A0 must not change this type.

**Net-new A0 acceptance criterion:** a test-only `MaterialEdgeReference + BodyStageRef` resolves to a precise **infinite unoriented line**, insensitive to native Edge direction/terminal index and reproducible from the same semantic source through provider replacement and authentic upstream edit/cold rebuild; every unavailable/ambiguous source refuses that line without last-good fallback. No A0 PASS is justified merely by Q0 12/12 or Q2 MID results. Prefer calling existing helpers/fixtures in the same registered source to duplicating algorithms; new independent assertions must specifically target this Axis use. The absence of a reproducible reversed-native-edge fixture is an explicit evidence GAP, not silent proof.

## 3. Fixed Owner-accepted executable allowlist — NOT usable until separately activated

1. `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — **additive assertions in the pre-registered kernel-native target only**.
2. `work/RG-02C_A0_STRAIGHT_BODY_EDGE_AXIS_NATIVE_ACCEPTANCE_EVIDENCE.md` — new factual evidence report after tests run.

No additional files, test executable/CTest registration, `src/**`, Kernel/OCCT public or private API, `tests/CMakeLists.txt`, `.github/**`, scripts, schema, DOC-01 product Markdown or generated Browser, `ACTIVE.yaml`, roadmap or ADR modifications are permitted in execution. Any needed third path or wider API/change requires **STOP + independent Owner D2 amendment**, not a clever helper outside the allowlist.

## 4. Required acceptance and lifecycle gates

- **Before executable A0:** Owner separately accepts the precise A0-01…A0-07 scope and two-file allowlist; governance-only proposal passes exact-head Windows CI and gets separately authorized guarded squash. A distinct Owner-approved `ACTIVE.yaml`-only activation PR points to this merged contract, passes exact-head governance, and is separately squash-merged. Until then, not one test-source line may change.
- **After activation:** fresh test-only branch from protected `main`, real native OCCT FOCUSED `pm05c2b.part_edge_topology`, Draft FAST and **Ready FULL Core+Kernel+Desktop** on exact code/tests SHA, all original registered tests unmodified and running. Any RED or test fixture limitation remains visible in the evidence report. Work-only final HEAD receives exact-head governance CLOSURE if code/evidence SHAs differ.
- **Owner A0 technical PASS** and a separate guarded production-test PR merge are mandatory. No successful A0 evidence automatically creates a usable Axis constructor.
- **Future production/UX:** only after A0 and a further narrow Owner D2 decision about Axis source grammar, diagnostics, dependency cycle, typed persistence/migration, edit/repair and Revolve consumption can a separate vertical-slice Work Contract be proposed. The Owner-mandated **prior explicit RG-02 Axis UI consultation and approval** remains a hard STOP before any Workbench/Tree/Viewer/hover/Command Line edit.

## 4A. Owner Axis UI consistency directive — future scope only, not A0 UX approval (2026-10-15)

**Owner explicit requirement:** when separately authorizing the guarded squash of this A0 contract, Owner directed: `UI musi byś spójne z aplikacją i istniejącym "datum"`. Any later Axis UI must therefore use the application's existing, shared **Datum Plane and Reference Geometry conventions** for tool entry, right-side Operations/Properties and source controls, viewport hover/selection feedback, preview and signed Reverse (only where semantically applicable), Finish/Cancel/Edit, Undo/Redo, persistent Tree/Properties/visibility and contextual CAD Command Line/keyboard/focus lifecycle. **Reuse established components and logic; do not create parallel UI systems or duplicate tests.** Axis remains a distinct Part-owned semantic object and must not be disguised as a Datum Plane constructor.

This product/interaction consistency is an **approved design constraint, NOT approval of any concrete layout/flow**. Per accepted RG-02 roadmap v1.33, reviewable Axis UX flows must be presented to the Owner and **separately explicitly approved before any UI mutation**. A0 itself is read-only native evidence: NO UI/Viewer/Tree/CAD CLI change, independently of this future requirement.

## 5. Deliberately excluded or unproven

OUT: Axis through two Points, cylindrical patch Axis, two-plane intersection, curved Edge Axis, a `MID`/CENTER picker, new generic Point meaning, Datum Plane constructor extension, speculative universal Axis framework, solid Mirror/Pattern, Assembly/Drawing, PM-06 and CI #302 remediation. A0 cannot guarantee that a strict Edge survives every upstream edit or that every visually straight edge is admissible; ambiguous/unsupported classification is a valid fail-closed result, not a reason to relax the contract.

**Single point of failure:** an apparently valid selected material Edge has no uniquely reproducible stage-bound semantic identity or the current native query cannot recover a consistent exact line on independent rebuild. If this occurs, **STOP for Owner D2**, do not persist a token or infer source from matching geometry.

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Generated Browser: not required for A0 evidence-only tests.  
Reason: A0 adds read-only native evidence, not new current product semantics or UI. This governance-only D2 proposal itself modifies one `work/**` file. A separately accepted future executable Axis vertical slice must set its own DOC-01 obligations.
