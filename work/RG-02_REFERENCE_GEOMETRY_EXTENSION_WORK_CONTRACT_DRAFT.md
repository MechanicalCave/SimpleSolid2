# RG-02 — Construction Reference Geometry Extension (DRAFT Work Contract)

**Status:** DRAFT — design decisions recorded; **NOT accepted as an executable Work Contract; NOT active**  
**Recorded:** 2026-10-12 — Owner/assistant RG-02 design discussion  
**Decision boundary:** Owner has accepted the product behaviors summarized below and the proposed A→D planning sequence. This document records them for later contract finalization; it does **not** authorize production implementation, a roadmap amendment, or changes to `work/ACTIVE.yaml`.  
**Existing authority:** `governance/CONSTITUTION.md`, `governance/FOUNDATION.md`, accepted ADRs, `work/ACTIVE.yaml`, `work/PART_MODELING_V1_ROADMAP.md` v1.32 (Owner D2 sequencing approved and roadmap merged via PR #330), PM-03 Datum and PM-04 Axis contracts.

## 1. Purpose and limits

Extend the existing Part-owned Datum Plane and Axis facilities before future solid Mirror/Pattern, using the existing single-Body Part architecture. Preserve semantic, stage-aware, fail-closed references; do not create an independent construction-geometry identity system.

This is a **design record / prospective RG-02 production Work Contract only**. The separate **RG-02A/G0 evidence-only Work Contract scope** was Owner-accepted on 2026-10-12 and its placement in Part-v1 roadmap v1.32 was accepted and merged via PR #330, but the G0 contract is **not yet merged or activated**. Any broader RG-02 production implementation still requires its own technical evidence, bounded contract, separate Owner approval and formal activation. The existing completed UI-01 `ACTIVE.yaml` pointer must not be treated as RG-02 authority. No native G0 tests have been run under RG-02A.

## 2. Owner-accepted product behavior (design baseline)

### 2.1 Body points and selections

- Support viewport selection of `VERTEX` (real Body vertex), `MID` (half the actual length of a straight bounded Edge or open circular-arc Edge), and `CENTER` (geometric center of a circular Edge or open circular arc). A full closed circular Edge has no unique `MID` along its circumference; `CENTER` is separate.
- Such picks refer to the **durable semantic source Edge/Point and an explicit Body history stage**, never viewport position, tessellation midpoint, OCCT token, or a cached coordinate. Exact kernel geometry is required. Unsupported or ambiguous identity fails closed.
- Multi-point constructions using Body geometry select all Body points at **one common declared Body stage**, even where their carrier geometry originated in different Features. Only points visible/selectable in that stage are acquired; no implicit selecting from hidden older stages.
- Earlier upstream Feature edits update derived points. Later Chamfer/Fillet/other Features do **not** silently rebase an existing reference to the changed final-Body Edge. No speculative geometry-similarity matching.
- **No standalone Datum Point** is introduced.

### 2.2 Datum Plane constructors

Keep the existing Offset constructor and add the agreed behavior for:

1. **Through three points:** three Body picks in any valid `VERTEX/MID/CENTER` combination; reject coincident/collinear input. Selection order A→B→C is authored: A is local origin, A→B is +X, C selects the default +Y side, and normal follows the right-hand rule. `Reverse normal` retains X while reversing Y/normal. Frames are derived deterministically from semantic sources.
2. **Perpendicular to Edge at MID:** for a straight bounded Edge or an open circular arc; origin is MID, normal is along the straight Edge or the tangent to the circular arc at MID. `Reverse normal` preserves origin, with a consistent right-handed local frame. No camera/provider-order dependence.
3. **At angle:** straight hinge Edge **lying on** the selected base plane (Origin Plane, existing Datum Plane, or planar Body surface), plus a signed angle; reject inconsistent sources. Local origin defaults to the hinge Edge's MID, local X lies along the hinge, and local Y/normal are derived deterministically from the angled plane. No silent relocation to accommodate an invalid hinge.

For constructors based on multiple Body geometry picks, persist an explicit common Body stage and validate dependency ordering/cycles. Existing Datum sources and identities remain intact.

### 2.3 Axis constructors

Extend the existing Part-owned Axis facility, retaining current Origin axes and Sketch-Line Axis. New agreed source types:

1. Along one straight Body Edge.
2. Through two distinct Body `VERTEX/MID/CENTER` points from one Body stage.
3. Along the **exact axis of a genuinely cylindrical B-Rep Surface**, including an open/partial cylindrical patch, regardless of originating operation (Extrude, Revolve, Cut, Fillet, etc.). Recognition must inspect actual surface geometry, not Feature-name heuristics or tessellation. Non-cylindrical Fillet patches must not be accepted as cylinders.
4. At the intersection of two nonparallel, noncoincident **infinite geometric planes**. Sources can be Origin planes, Datum planes, planar Body surfaces, or valid mixed combinations. Bounded visible Face patches need not touch.

An Axis defines an **unoriented infinite line**. The consuming operation owns reversal/direction of rotation; changes to provider parameterization must not flip behavior of dependent Features.

### 2.4 Dependencies, editing and repair

- Every Datum Plane/Axis keeps its own durable identity and semantic references; previous Feature edits cause reevaluation, while later Features do not rewrite previously acquired stage-aware sources.
- Missing, ambiguous, blocked, or unsupported sources must report diagnostics and **never** reuse last-good geometry or auto-pick a replacement.
- Extend existing Edit Datum Plane / Edit Axis paths to replace individual broken source selections without destroying the object or its dependent relationships. Unresolved objects remain authored and editable. Dependent Features/Sketches fail closed until references are repaired. Structural deletes remain dependency-safe.
- Source acquisition should be coherent between **3D viewport and Tree**, including mixed Body/Origin/Datum picks where the constructor admits them; GUI and Command Line share semantic command authority.

## 3. MANDATORY OWNER UI CONSULTATION AND APPROVAL — HARD STOP BEFORE UI IMPLEMENTATION

**This is an explicit Owner requirement accepted on 2026-10-12. Before implementing ANY RG-02 user-interface behavior, the implementer MUST first present the intended interface design to the Owner, discuss it, and obtain an explicit separate approval of that UI design.** The prior acceptance of RG-02 functionality and of the A→D plan is **not** approval of a UI layout or interaction design.

The pre-implementation consultation must cover, at minimum:

- Datum Plane and Axis tool entry, constructor selection and coherent right-hand Operations/Properties/Tree workflows;
- acquiring/changing sources in the 3D viewport and Tree, including mixed reference types;
- hover/pick affordances and `VERTEX` / `MID` / `CENTER` marker appearance, eligibility and priority, ambiguity feedback, highlighting and keyboard/focus operation;
- angle/normal/reverse controls, live preview, Finish/Cancel/Edit and stage-awareness feedback;
- lost-reference diagnosis and source-by-source repair, and equivalent GUI/Command Line behavior.

Provide **reviewable UX artifacts** (e.g., sketches/wireframes or a focused workflow prototype) and interaction scenarios. Record the Owner's explicit approval (or requested amendments) in a dedicated D2 UI decision artifact / Work Contract amendment **before the first UI implementation change**.

**STOP boundary:** no RG-02 UI widgets, panels, pick markers, viewer presentation, Tree integration, user-facing interaction routing, or user-facing draft/Command Line behavior may be implemented until the above design approval is recorded. Do not assume that a backend-only RG-02A phase implicitly authorizes viewport snap/marker work; such presentation changes also require this gate. After approval, implement only the specific bounded UI scope accepted in the executable Work Contract. New UX behavior discovered mid-implementation must return to the Owner for consultation **before coding it**.

This gate is independent of the separate mandatory Owner approval and formal activation of the RG-02 **production Work Contract** itself.

## 4. Proposed staged delivery — planning only

- **RG-02A — precise point/reference foundation:** audit and prove durable, stage-specific Body references and exact `VERTEX/MID/CENTER` geometry. Presentation/picking aspects are subject to Section 3's prior UI approval.
- **RG-02B — Datum Plane constructors:** full semantic definitions, deterministic frames, dependencies and diagnostics; user-facing surfaces only after the Section 3 gate.
- **RG-02C — Axis constructors:** four source families, including exact partial-cylinder recognition and plane intersections; user-facing surfaces only after the Section 3 gate.
- **RG-02D — lifecycle and UI integration:** approved viewport/Tree/Operations/Command Line workflows, preview, repair, Edit, Finish/Cancel, Undo/Redo, Save/Reopen/cold rebuild and native automated/manual acceptance. Integrate and test each vertical slice earlier where approved; do not defer all testing to D.

Detailed numeric policy, precise durable selector representation, provider-neutral API, schema migration/version, exact test matrix and permitted paths remain to be specified and Owner-approved in the executable bounded contract. Preserve a single Body. Do not introduce Mirror, Pattern, standalone Datum Point, or independent CI fixes under RG-02.

## 5. Validation and activation gates

Before any production mutation: reconstruct authoritative `main`; collect a stage-aware reference survival/selection baseline; prove real native kernel behavior on supported/unsupported surfaces and edges; agree exact D2 technical contracts and schema consequences; finalize the allowed-file/test matrix and RED→GREEN gates; secure separate Owner Work Contract acceptance; follow formal `work/ACTIVE.yaml` activation. All UI/presentation implementation additionally requires the **prior Owner UX approval in Section 3**.

Do not claim test PASS, Owner UI approval, Work Contract activation, or implementation completion without independent evidence.

## Documentation impact

Internal docs: **required** for implementation (semantic references, stage/rebuild/lifecycle and provider boundaries).  
User/Product docs: **required** for implementation (PL/EN Datum Plane, Axis and picking/repair workflows).  
Generated Browser: **required** under the existing documentation rules at production completion.  
Reason: RG-02 affects durable model semantics and user-facing construction geometry and interaction. This **draft-only governance change** does not document unimplemented features as current product behavior and does not trigger as-built documentation updates.
