# Sketcher R13-A1 — Local relations and native Sketch evaluation — D2 Work Contract PROPOSAL

**Status:** DRAFT FOR EXACT OWNER D2 ACCEPTANCE; **NOT ACTIVE**, NOT a product authorization. Prepared 2026-10-19 after Owner approved a functional-first SS1-donor adaptation and explicitly requested implementation progress. This records that direction without falsely asserting independent exact-path/ACTIVE authorization. No source/vendor/schema/UI change is authorized by this draft alone.

**Baseline:** SS2 protected `main@c0aa91f5abdbc2ba9a997bc53b99cebe873d335f`; ACTIVE currently `work/SKETCH_R13_G0_FEASIBILITY_D2_WORK_CONTRACT_PROPOSAL.md`. R13 G0 ledger in Draft PR #498; Windows existing-product FULL #3710 on product baseline `21205a3b879bde80650eef2ef857a858eaa50f21` (Core 25/25, Kernel 57/57, Qt/Desktop 117/117). The performance D2 relief in merged #499 is **DEFERRED-BY-OWNER-D2, not PASS**. Accepted Sketch v1.11 and Part Modeling v1.35 roadmaps schedule R13-A1 next; their old headers can contain superseded merge statuses.

## 0. Purpose, authority, activation

Implement the five Owner-approved **explicit manual** local geometric relations by adapting proven SS1 ideas, not inventing an unrelated solver: **Coincident(Line Start/End, Line Start/End), Horizontal(Line), Vertical(Line), Parallel(Line, Line), Perpendicular(Line, Line)**. Auto-Constraint OFF. Source `MechanicalCave/SimpleSolid@b6c010b1af4a03ab130b09f25b6e61b1d9f421ec` is an Owner-owned, tested functional prototype: its historical Windows M17 solver + parametric Sketch→Part lifecycle counts as **donor evidence**, not a new SS2 run. Prefer translating donor acceptance fixtures and solver-neutral contracts over re-proving donor algorithms. SS2 Foundation 1.0 / ADR-0008, 0009, 0012, Part/Projection ADRs and active governance own integration semantics.

**Activation is separate from this proposal:** first independent technical/Owner disposition of remaining G0 evidence and separate permission to merge #498 if needed; Owner exact acceptance of THIS D2 contract and exact conditional path allowlist; one governance-only contract PR passing its own exact-head Windows CLOSURE and separate Owner guarded-squash authorization; then a **distinct one-file `work/ACTIVE.yaml` PR**, separately CLOSURE-verified, explicitly Owner-authorized, merged and read back from protected main. Product RED/production changes start only after this switch. No implicit approval of contract text, solver distribution, merge or ACTIVE from a general “kontynuuj”. PM-06/RG-02 stay deferred, not cancelled.

## 1. Delivery strategy — small, real vertical slices

**A1.1 semantic RED → GREEN:** first `Horizontal(Line)` with Sketch-local typed monotonic `ConstraintId`, semantic references, authored-only persistent intent, duplicate/missing/stale rejection, derived evaluated Sketch snapshot separate from authored seeds; a real `DocumentSession` revision-bound Create/Remove Command with atomic Part transaction and Undo/Redo. This first green must use the actual SS2 native build, not the external smoke surrogate. The prior separate `local_constraints.hpp` spike is design input only (its ad hoc horizontal evaluator and vertical-seed rejection are **not accepted solver semantics**).

**A1.2 donor numeric solver adoption:** bring the remaining four relations through the **SS2-owned solver-neutral adapter**, translating the donor PlaneGCS acceptance policy and diagnostics; verify only material integration risks in actual Windows VS2022 x64 (compile/link, fixed/projected entities, conflict/redundancy, finite residual/DoF, installed consumer). Do not write a second general solver or add a global dependency graph. Do not install new vendor code until a pinned-rights/relink/distribution plan is explicitly Owner accepted. If that dependency decision blocks A1.2, record the STOP without pretending A1.1 is the five-relation implementation.

**A1.3 Part and file lifecycle:** new future strict native Part store version (**candidate v21, not effective before exact approval**), v20 read migration, cold Save/Close/Reopen, same-revision Part stage/projection → constrained evaluated Sketch → Profile/Extrude/Revolve; local Delete cascade, repair-required downstream Features preserved, Undo/Redo exact identities; native 3D Sketch/Part tests. No persisted solved state.

An A1.1 milestone is not A1 FINAL. Finish all five relations plus persistence/Part integration and Windows acceptance before A1 technical FINAL or product merge. Owner UI review is required **before** implementation of any A1 relation controls/Qt UI; A2 dimensions and A3 Reference/Driving are separately contracted.

## 2. Nonnegotiable minimal semantics

1. **Ownership:** Shared 2D owns authored local `ConstraintId`, kind, semantic `EntityId + Start/End/Line` reference, optional user intent; Part retains Sketch support/PG-01 projection/Profile/Feature/native file ownership; DocumentSession owns transaction, revisions, history and dirty state. Constraint-only changes participate in equality/state Save/Undo.
2. **Geometry:** authored Line/Circle/Arc coordinates remain seeds; solve runs on disposable revision-and-stage-bound geometry. Projected PG-01 targets are fixed source references; do not mutate them or break links through ordinary Delete. Reject stale stage/revision, singular or nonfinite results; no general tolerance-welding. A numerically converged Coincident must feed exactly matching evaluated endpoints to the Profile resolver, or explicitly fail.
3. **Identity/lifecycle:** create unique Sketch-local IDs without burning cursors on failed commands; restored Undo resurrects exact semantic IDs; Redo deterministic; removing user geometry cascades only dependent Sketch local relations with impact confirmation when applicable. Do not delete dependent Part Profile/Feature records; set derived repair-required when evaluation invalidates.
4. **Evaluation:** explicit user relations only; Snaps, inference, dynamic input and Grip never persist constraints. Underconstrained is legal, contradictory/redundant/fixed-incompatible/ambiguous constraints reject atomically with structured diagnostics. Respect caller-owned length/angle units; accept no nonfinite solution or global arbitrary epsilon.
5. **Release license:** SS1 donor ownership removes needless donor-prototype feasibility repetition; third-party PlaneGCS LGPL is a **distinct** dependency. Actual archive checksum and required source/notice/replace-or-relink pathway must be proven before vendoring/distribution as appropriate. External verification is permitted; no runtime network fetch as part of SS2 builds.

## 3. Finite candidate path allowlist — applies ONLY AFTER contract acceptance + ACTIVE

All paths below are **conditional**, for the purposes stated, with no generic directory wildcard authority. Owner acceptance freezes a finite list; adding further paths requires explicit D2 addendum. Use the smallest subset required by each slice.

**Sketch model + neutral solver:**
- `src/sketch/include/simplesolid2/sketch/sketch_model.hpp`
- `src/sketch/sketch_model.cpp`
- `src/sketch/include/simplesolid2/sketch/local_constraints.hpp` (new; exact name)
- `src/sketch/local_constraints.cpp` (new, only if implementation requires)
- `src/sketch/include/simplesolid2/sketch/local_solver.hpp` (new neutral adapter contract, only in A1.2)
- `src/sketch/local_solver.cpp` (new neutral adaptation, only in A1.2)

**Part/store/stage evaluation:**
- `src/part/part_document_store.cpp`
- `src/part/part_document.cpp`
- `src/part/feature_evaluation.cpp`
- `src/part/profile_kernel_input.cpp`
- `src/part/include/simplesolid2/part/effective_sketch_projection.hpp`
- `src/part/include/simplesolid2/part/part_sketch.hpp`

**Commands and existing Sketch-facing integration:**
- `src/application/document_session.cpp`
- `src/application/include/simplesolid2/application/document_session.hpp`
- `src/ui/part_sketch_interaction_controller.cpp` **only AFTER Owner real SS2 UI walkthrough**
- `src/ui/part_viewport_controller.cpp` **only AFTER Owner real SS2 UI walkthrough**

**Registered test candidates:**
- `tests/sk02b_part_sketch_model_test.cpp`
- `tests/sk02b_part_sketch_persistence_test.cpp`
- `tests/r12_structural_edit_document_session_test.cpp`
- `tests/pm02f_stage_aware_profile_evaluation_test.cpp`
- `tests/sk03a_part_viewport_controller_test.cpp` **UI-gated**
- `tests/sk04c_part_sketch_interaction_controller_test.cpp` **UI-gated**
- `tests/r13_a1_local_constraints_test.cpp` (new native registered test)
- `CMakeLists.txt` (target/test registration only if no existing build registration path; no unrelated settings/CI rewrite)

**As-built documentation and DOC-01 (exact existing file paths to be certified before activation):** omitted from this initial executable allowlist intentionally; if internal ownership/native schema or product UI changes require DOC-01 updates, accept a finite D2 addendum listing the confirmed existing Markdown paths and generated Browser before production completion. The absence is **not** permission to skip mandatory docs.

**Not included / STOP:** any other `src/**`, other `tests/**`, `third_party/**`, `cmake/**`, `.github/**`, CMake vendoring/FetchContent, general `docs/**`, `work/ACTIVE.yaml`, runtime autoconstraint UI, dimensions A2/A3, unrelated PM/RG, new public Kernel API, arbitrary solution-cache persistence. If dependency build needs an additional path, explicitly request exact D2 allowlist extension; do not silently add it.

## 4. Compact acceptance — no benchmark inflation

- **RED before GREEN** on actual registered C++20 SS2 tests; Horizontal semantic identity, command revision, dirty/equality, Undo/Redo; five relations and fixed/projection/conflict rejection; v20 backward read + candidate successor strict round trip/corrupt record rejection; cold Part Profile/Feature repair and exact evaluated vertex closure. Tests stay small, representative and source-tied.
- Real **Windows FOCUSED / FAST / Ready FULL** (Core, Kernel-native, Qt/Desktop when relevant) at recorded exact PR product HEAD, plus relevant DOC-01 verify/browser and existing released regressions. Never call donor M17 a new SS2 PASS.
- Manual Owner inspection of user-visible 3D constraint controls after Owner UI walkthrough; native Save/Reopen and representative Sketch→Profile/Extrude. Product technical FINAL and explicit guarded squash decision remain distinct.
- **Performance:** functional-first; no p50/p95 benchmark or 13-feature stress test as a prerequisite. Still fail-closed on nontermination, numerical corruption, excessive obvious hangs and stale publication. Future profiling can be contracted after functional implementation.

## 5. Decision points to freeze before first SS2 product edit

The precise D2 acceptance of this draft approves: the five relation kinds, typed Sketch-local identity, separate authored/evaluated state, one SS2 DocumentSession history, stage-aware Part semantics, **candidate** v21 native format only when the exact schema disposition is accepted in A1 execution, and conditional paths above. The initial native RED/Horizontal slice can proceed **after formal ACTIVE**, without importing the PlaneGCS binary; approval of G0 feasibility evidence does not constitute distribution sign-off.

The independent Owner must still decide: (a) PlaneGCS 0.8.0 DLL/STATIC licensed packaging or approved alternative before third-party import; (b) actual schema successor version/field ownership before writing it; (c) A1 UI layout after the requested joint UX review. These are finite, precisely deferred decisions, not permission for uncontrolled architecture changes.

## 6. Documentation impact

Internal docs: **required** (authored constraints, evaluation ownership, commands and native migration).
User/Product docs: **required** (once five relations appear in UI; PL/EN canonical Markdown and generated Browser).
Reason: meaningful persistent CAD intent and a new user-facing capability. Identify and separately approve **exact** documentation paths before documentation mutation; no G0 documentation files are repurposed.

## 7. Handoff / status

This PR is **governance/design only**. No R13-A1 tracked product source/test/schema/vendor/UI modifications, no `ACTIVE.yaml` edit, no protected-main merge, and no assumption of exact contract acceptance. Next: independently accept/freeze this finite contract, close G0 per its actual process, guarded-merge contract, then an **independently authorized** one-file ACTIVE transition. Build the donor-based A1.1 native RED then GREEN only after verified activation.
