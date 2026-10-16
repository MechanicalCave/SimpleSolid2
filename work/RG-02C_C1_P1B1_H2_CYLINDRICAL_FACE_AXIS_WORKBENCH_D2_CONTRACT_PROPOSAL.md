# RG-02C/C1-P1-B1/H2 — existing Work Features Axis: Cylindrical Face constructor D2 Work Contract PROPOSAL

**Status: DRAFT FOR EXACT OWNER D2 REVIEW — NOT OWNER-ACCEPTED, NOT ACTIVE, NOT PRODUCTION MUTATION AUTHORITY.** Date: 2026-10-16.

**Baseline:** protected `main` after accepted, independently gated and guarded-squash merged H1 [#444](https://github.com/MechanicalCave/SimpleSolid2/pull/444): `e2b4aa3db7026c589052ef2c5789d5b01c2da92e`. The active pointer `work/ACTIVE.yaml` still names completed H1; this H2 governance-only proposal **does not** activate or close it. H1 retains native Part **v20**, durable `StrictCylindricalFaceAxisSource{SurfaceReference}`, strict policy A and the inherited explicit native fixture GAPs.

**Already approved, distinct UX authority:** [#439](https://github.com/MechanicalCave/SimpleSolid2/pull/439), merged `e4fa14e9bd9cdcf02a0951da1f123f152ba75c45`, records the Owner's **separate approval** of its exact W1–W3/UX-01…14 new `Cylindrical Face` existing-tool UX. This is not acceptance of the **executable file scope, testing plan, integration, or activation** below.

**Governing sources:** `AGENTS.md`; `governance/CONSTITUTION.md`, `FOUNDATION.md`, `ARCHITECTURE.md`, `DOCUMENTATION.md`, accepted ADR-0014/-0016/-0017; Part Modeling v1.33 RG-02; `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` mandatory prior UI signoff; merged #439 UX; activated H1 contract and merged #444 as-built.

## 1. Bounded H2 deliverable and non-goals

Add exactly ONE `Cylindrical Face` item after the existing default `Edge` in the **same** `Work Features → Axis → Operations → Constructor` combo; keep one Axis toolbar button and the same `AxisDraft`, `AxisId`, semantic `CreateAxisCommand`/`EditAxisCommand`, Undo/Redo and Revolve `AxisReference`. Sketch-Line Axis remains authored under Sketch and is **not** a third Work Features constructor. No second Axis tool, alternative CAD source definition or copy of H1 Face admission.

A Face gesture selects **exactly one current bounded native Body Face** and remembers at most a disposable generation-bound pick until immediately re-admitted by H1's `part::authorStrictCylindricalFaceAxisSource(document,stage,token,current_evaluation,exact_provider)`. The source staged in `AxisDraft` is **only** the returned H1 `StrictCylindricalFaceAxisSource{SurfaceReference}`; its BodyStageRef and semantic FeatureSurfaceAddress must be resolved on the exact selected stage. Read the actual current native OCCT cylinder query, strict semantic carrier `Resolved` and `strict_face_status=resolved`, with exactly ONE current Face. Refuse ambiguous carrier, multiple Face realizations (even coaxial), planar/torus/cone, wrong body, stale revision/provider generation, unsupported native query, missing/suppressed producer, wrong stage and invalid dependency with a typed diagnostic, no stale last-good preview.

**No persistence or native schema modifications in H2.** Reuse H1 v20 writer/reader, shared `AxisDraft::setSource`, `evaluateAxisDraft`, `finishAxisDraft` and existing Revolve evaluation. No new OCCT/kernel public interface, `src/part/**`, `src/application/**`, CMake/CI workflow, Viewer engine, generic topology identity, Face-token persistence, closest geometry recovery, source orientation, sign, Reverse, finite axis semantics or Part-wide selection heuristics. If a need for any arises, **STOP and request an exact Owner D2 amendment**.

## 2. Interaction and lifecycle, acceptance W1–W3

**Create:** the unchanged Work Features Axis button opens Edge by default; switching to Cylindrical Face clears any incompatible Edge source/selection and preview with **zero authored mutation** and enables Face-only current Body picking using the existing `PartViewportController` selection/generation model. One valid click stages one H1-certified Surface source and presents the exact current **unoriented infinite line as a finite Viewer cue**; no preview for unsupported/ambiguous Face. Multiple simultaneous Face selections are rejected, not silently reduced to the first. Changing back to Edge likewise drops the previous Face draft source.

**Finish/Cancel:** Finish revalidates current `DocumentRevision`, native stage/provider lease and single-Face admission via existing H1 AxisDraft/application transaction; one valid Finish allocates one AxisId and Undo, while rejected Finish, hover, Escape/Cancel or constructor switch allocates none. GUI button and owning CAD Command Line `SOURCE`, `FINISH`, empty Enter, `CANCEL`, Escape share the existing draft authority; no new alias or global parser grammar. A provider/body scene rebuild invalidates ephemeral picks and previews.

**Edit/Replace/Repair:** opening an existing Face Axis preselects the Cylindrical Face constructor and displays its semantic stage/source and typed current evaluation, not a cached Face token. Explicit Replace Face can stage a different currently certified strict Face at a permitted stage; Finish preserves AxisId, name, visibility, downstream Revolve AxisReference and one Undo transaction. Cancel/no-op retains the original source. Missing/suppressed/partitioned source is repairably unresolved and never triggers nearest-Face rebinding. Reuse existing Delete, Hide/Show and Undo/Redo; an absent Axis remains an authored dependent Revolve reference until repair.

**Tree/Properties/Preview:** Face- and Edge-sourced Axes share the **existing** `Reference Geometry` group, once per AxisId. Sketch-Line Axes remain nested under Sketch; Origin XYZ unchanged. Properties show typed Face source, stage and meaningful missing/ambiguous/native diagnostics (no phantom SketchId/Edge label). The existing `setAxisDraftPreview` disposable reference scene and Face highlight are reused without rebuilding Body while previewing. Preserve all existing responsive toolbars, Work Features, Edge/Datum/Sketch/Revolve routes and viewport focus.

## 3. Exact proposed implementation allowlist — CONDITIONAL ONLY

These **18 exact paths** would become editable **only after** this precise D2 Work Contract receives separate explicit Owner acceptance, its governance PR is independently Windows-validated and independently authorized for guarded-squash merge, and a separate `work/ACTIVE.yaml` switch to this contract is Owner-approved, Windows-validated and merged. Until then NONE of the following paths are authorized by this proposal:

1. `src/ui/cad_workbench.cpp` — constructor state, Face acquisition, H1 draft validation, user-facing status/Repair, CAD input routing.
2. `src/ui/cad_workbench.hpp` — existing-tool bounded state/helper declarations only.
3. `src/ui/part_viewport_controller.cpp` — exact scene-generation Face selection admission via existing H1 Part semantic/native API.
4. `src/ui/part_viewport_controller.hpp` — narrowly typed, runtime-only current Face acquisition method; no durable pick identity.
5. `src/ui/part_document_tree_controller.cpp` — display Face Axis once under Reference Geometry with typed diagnostics and selection.
6. `src/ui/part_document_tree_controller.hpp` — only declarations directly required by the previous Tree change.
7. `tests/pm04b2_axis_tree_viewer_ui_test.cpp` — existing Axis widget/Tree/visibility/Edit regressions.
8. `tests/pm04b2_axis_tree_viewer_test.cpp` — mixed Sketch/Edge/Face Axis ownership, selection and presentation.
9. `tests/pm05f_r2_native_workbench_edit_test.cpp` — existing native Qt/OCCT Workbench Edit and cold-lifecycle witness.
10. `tests/pm05e2_cad_workbench_edge_lifecycle_test.cpp` — existing Edge constructor and mixed-source regression.
11. `tests/pm04d2_cad_workbench_revolve_test.cpp` — consuming selected Face Axis in existing Revolve user workflow.
12. `tests/sk04b_part_viewport_selection_bridge_test.cpp` — native scene generation/Face-only pick routing with no stale pick.
13. `tests/wb02_global_cad_input_ui_test.cpp` — unchanged unified AXIS SOURCE/FINISH/CANCEL and focus/Esc grammar.
14. `docs/internal/PART_DOCUMENTS.md` — canonical shipped interactive as-built, distinct from completed H1 headless.
15. `docs/product/pl/PARTS.md` — default Polish Axis constructor, source selection, Edit/Repair/diagnostics.
16. `docs/product/en/PARTS.md` — exact English peer of Polish product behavior.
17. `docs/browser/index.html` — **generated only** by official `.\ss2.ps1 docs`, never manual HTML editing.
18. `work/RG-02C_C1_P1B1_H2_CYLINDRICAL_FACE_AXIS_WORKBENCH_ACCEPTANCE_EVIDENCE.md` — factual exact-SHA PASS/RED/GAP ledger and Owner manual result.

No `work/ACTIVE.yaml` alteration is allowed **inside H2 implementation PR**. Activation is a distinct governance-only PR after independently accepted contract integration; the current proposal PR only creates **this single document**. Tests outside the 18 exact paths, changes to v20 fixtures, public kernel/Part source interfaces, CAD grammar, new CMake test registration or new UI are not implicitly authorized.

## 4. Native/GUI acceptance and strict STOP conditions

- Real Windows Qt+OCCT Workbench native input with a full/partial genuine cylindrical Face from circular Extrude and Revolve; Boolean-trimmed singleton if genuinely available. Use actual native Face selection, exact Body stage/revision, true H1 strict resolution and bounded generation tokens; no mocked-only success.
- W1: single existing Axis tool, `Edge` default, `Cylindrical Face` second, no Sketch constructor. W2: valid Face highlight, finite unoriented cue and Finish; noncylinders refused with no preview/Undo. W3: Face Axis under Reference Geometry, Edit/Replace retains AxisId, source identity and downstream Revolve, missing source gives explicit repair.
- Negative real states: wrong Face kind; selection multiplicity; invalid/suppressed stage; stale DocumentRevision or Viewer generation; partition-ambiguous Face **where genuinely witnessed**, never pretend a synthetic ambiguous result was produced. A still rejects all multi-Face carriers by H1 code; inherited absent partition-positive native fixture stays a disclosed GAP.
- Cold Save/Close/Reopen using **native v20** with freshly created OCCT Body and current Face Axis; dependent Revolve must remain legal or fail closed without last-good geometry. Check Undo/Redo, Cancel/Esc, constructor switching, Edge regressions, Sketch Axis, existing Datum Plane and one-row/responsive Work Features.
- **Mandatory independent gates:** MSVC true native **FOCUSED** on already-registered tests with valid selectors; separate **Draft FAST**; genuine **Ready FULL** Core-only+Kernel-native Release+Qt/OCCT Desktop; official **DOC-01** with canonical internal + both PL/EN product documents and generated Browser; factual **exact-head CLOSURE**. Never infer native success from a Core-only widget test or a previous revision's green checks. Do not delete/weaken tests to force PASS.
- **Independent Owner manual Windows GUI review** of W1–W3/UX-01…14 with explicit pass/fail and inherited GAP list; followed by separate **Owner technical FINAL PASS** on exact implementation SHA, then separate explicitly authorized guarded-squash implementation merge.
- If actual native evidence cannot support unique stable Face source/scene, needs a new dependency/source selector, exposes a schema/provider boundary issue, or the 18-path list is insufficient: **STOP** for new Owner D2; do not broaden to carrier variant B/C or invent missing tests.

## 5. Owner decision and activation sequence

1. **CLOSED:** prior exact W1–W3/UX-01…14 product UX accepted and merged through #439; H1 strict Face semantic source and v20 native persistence accepted and merged through #444.
2. **PENDING:** Owner must explicitly accept/reject **this exact H2 executable Work Contract, 18 paths, tests, documentation and native manual gates** as a separate D2 decision. Creating/reviewing this draft **is not that acceptance**.
3. **PENDING:** run actual governance Windows CLOSURE for this one-file Draft proposal and secure independently authorized guarded-squash **governance-only** merge.
4. **PENDING:** separately Owner-authorized `work/ACTIVE.yaml` activation PR, exact-head Windows CLOSURE, guarded squash merge; only then may the H2 implementation branch mutate files in §3.
5. **PENDING:** independent product Windows FOCUSED/FAST/FULL/DOC-01, Owner manual GUI signoff, exact-head CLOSURE, separate technical FINAL PASS and later independently authorized implementation guarded merge. Earlier H1 acceptance does not substitute for these.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: this proposal changes only `work/**` and ships no user-visible behavior. **The eventual activated H2 implementation contract explicitly REQUIRES** updates to `docs/internal/PART_DOCUMENTS.md` and BOTH `docs/product/pl/PARTS.md` and `docs/product/en/PARTS.md`, with `docs/browser/index.html` generated solely through the official DOC-01 tool, before claiming H2 completion.
