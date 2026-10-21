# SimpleSolid 2.0 — Sketcher R13-A2
## Relations UI + Driving Dimensions — D2 Work Contract (governance DRAFT)

**Status:** **OWNER-ACCEPTED D2 DESIGN AND CONDITIONAL EXECUTION SCOPE (2026-10-21), GOVERNANCE INTEGRATION PENDING**. Owner explicitly accepted the post-audit D2 dispositions and requested continuation (`akceptuję - kontynuuj`). This status records product/architecture acceptance, **not** an authorization to guarded-squash merge, activate `ACTIVE.yaml`, ship PlaneGCS or modify production files. The additional strict v22 codec, solver distribution and real viewer interface proofs below remain independent technical gates.  
**Prepared:** 2026-10-21 (Europe/Warsaw).  
**SS2 authoritative readback:** `MechanicalCave/SimpleSolid2`, protected `main@30a17150552fe67e30fe84a7f17e0dce30ef619c`; `work/ACTIVE.yaml` still points to completed `work/SKETCH_R13_A1_LOCAL_RELATIONS_D2_WORK_CONTRACT_PROPOSAL.md`.  
**Program:** `work/SKETCH_ROADMAP.md` v1.11; companion Part Modeling roadmap v1.35; R13-A2 after technically accepted/merged R13-A1 PR #506.  
**Donor:** `MechanicalCave/SimpleSolid` `main@b6c010b1af4a03ab130b09f25b6e61b1d9f421ec`; specifically `components/SimpleSolid_Sketcher`, `SimpleSolid_Sketcher_Qt`, SK-02 S2 evidence/tests.  
**Historical design reference:** SS2 Draft PR #493, `work/SKETCH_R13_CONSTRAINTS_DIMENSIONS_D2_CONTRACT_PROPOSAL.md` on PR head `1b63591053cf92b1c18954f7e32d713bd486388b`. The newer Owner-accepted click-to-create UX below **supersedes the older suggestion that creating a dimension must confirm its placement and type a value before authoring**. This document is a new A2 work-contract proposal, not a silent mutation or automatic merge of #493.

## 0. Authority, purpose, STOP

Normative precedence: `AGENTS.md` → `governance/CONSTITUTION.md` → `governance/FOUNDATION.md` v1.0 FROZEN → `governance/ARCHITECTURE.md` → accepted ADRs (especially ADR-0008/0009/0012–0017) → `work/ACTIVE.yaml` → `work/SKETCH_ROADMAP.md` v1.11 → active Work Contract → `governance/DOCUMENTATION.md`. Existing native Part v21/modeling-v2 and DOC-01 remain authoritative until separately approved migration/implementation.

**Deliver a real, coherent A2 user workflow in the existing Part Sketch Edit 3D viewport:** interactive creation, visibility, inspection, selection and deletion of six R13-A1 manual relations; production solver-backed authorship; three Driving dimensions (Line Length, Circle Radius, Circle Diameter); native technical annotations, direct on-label value editing, independent durable drag placement and complete Undo/Redo/Save→Close→new-process Reopen. Auto-Constraint remains OFF; no implicit authoring from Object Snap, Polar, Tracking, Dynamic Input or ordinary drawing.

**Absolutely no production mutation** follows from Owner acceptance of this governance draft. The exact D2 contract, finite path allowlist, third-party solver distribution disposition, governance PR, Windows governance CLOSURE, separate Owner guarded-squash and subsequent distinct **one-file `ACTIVE.yaml` activation** each require their own explicit gates. G0/A1 success never implicitly authorizes a shipped PlaneGCS solver, A2 schema or UI. Stop on D2 conflict, unavailable dependency, unproven Part stage identity, stale revision, invalid solution, unsupported semantically selected source, missing license/relink path or unapproved files.

## 1. Owner-accepted interaction grammar (D3 product direction)

### 1.1 Layout — two buttons, types only in Operations

Inside **the existing Sketch-mode `Create` toolbar group**, immediately alongside the existing `Line`, `Circle`, `Arc`, `Rectangle` controls, add exactly **`Relations`** and **`Dimensions`** as ordinary tool buttons. They have **no menu/dropdown**. Activating either displays that mode's content in the **existing right-side `Operations` panel**. Do not create a separate window, dock/workbench, floating modal, duplicate toolbar tools, or replace the current Property Tree. Preserve existing responsive toolbar overflow/grouping and existing workbench focus. Leave the existing Object Snap, Tracking, Polar, Dynamic Input, Delete Selection and bottom `Finish Sketch` controls intact and reachable; do not push `Finish Sketch` out of the panel.

In `Operations / Relations`, six type choices: `Horizontal`, `Vertical`, `Coincident`, `Parallel`, `Perpendicular`, `Fix`. In `Operations / Dimensions`, three choices: `Length`, `Radius`, `Diameter` (one independent Circle size driver); no A3 Reference/Angle/Arc/point-to-point in A2. The selection widget is mutually exclusive; active type/required target count/help/typed diagnostic appear adjacent to it. `Show Constraints` and `Show Dimensions` **only in `Operations`** as independent visibility toggles. No Apply/Confirm/Cancel buttons are used for either **creation** mode. `Esc` and switching tools continue to supply cancellation.

Show/hide is an application/view presentation preference for the current editor context (not an authored Sketch constraint or document dirty/Undo event); hidden symbols/labels are not pickable. The visibility choice must not silently drop committed relations or dimensions, and should be consistent with sketch-context transitions and redraw.

### 1.2 Zero-extra-confirm click-to-author

The **last required valid target click** automatically performs `semantic draft → validation → actual accepted solver evaluation → single Part Command/Transaction commit`. No second click, Apply, Finish, confirmation dialog, numeric prompt or label-placement approval is needed for creating either a relation or a dimension. The command must recompute legality and expected revision at execution; a green preview/pick alone is not authority. Invalid/duplicate/redundant/conflicting/nonconvergent/unsupported requests produce typed, actionable Operations status and **zero mutation, zero consumed IDs, zero Undo entry**; the same mode stays active for correction.

After successful creation, retain selected **type** and active `Relations`/`Dimensions` tool for repeated creation; clear only completed candidate target acquisition. Single `Esc` during partial target capture drops only the draft and restores that tool's idle stage; another `Esc` can return to Select per established Sketch Edit routing. Switch mode/type clears incompatible pending targets, without mutation. `Esc` must not Finish Sketch while local dimension edit/drag/capture owns focus. No required global RMB command convergence or new shortcut scheme is introduced.

**Selection-first + command-first are supported equivalently.** If an unambiguous legal exact selection already exists, choosing the type immediately attempts the same one transaction without asking for repeat picks. If not enough targets are selected, remain in acquisition mode and collect missing target(s). If the selection is ambiguous or contains the wrong semantic element kinds, show an explicit diagnostic and require precise acquisition; never guess the missing endpoint, infer a different relation type, or select a nearest hidden subelement. Picking sources uses durable `SketchId / EntityId / ConstraintElement` within the current document revision and Sketch stage; Qt/OCCT hit tokens are ephemeral.

### 1.3 Relations gesture table

| Kind | Required semantic targets | User gesture and effect |
|---|---|---|
| Horizontal | one local Line | choose Horizontal, click Line, auto solve/commit |
| Vertical | one local Line | choose Vertical, click Line, auto solve/commit |
| Coincident | two explicit Line endpoints (Start/End) | choose Coincident, click exact first endpoint then exact second endpoint; no geometric-nearest guessing |
| Parallel | two distinct local Lines | click two Lines, auto solve/commit |
| Perpendicular | two distinct local Lines | click two Lines, auto solve/commit |
| Fix (Current Position) | one local Line | click Line, atomically capture verified effective endpoints as authored seeds and author `Fix(Line)` frozen at that position |

`Fix(Line)` retains its A1 solver meaning **fixed parameter mask, not a numeric sixth equation**. The new **Fix Current Position** command semantics are an explicit D2 extension, not a reinterpretation performed by the renderer or an unreviewed change to A1 storage. Linked projected geometry is fixed at evaluation time and is **not silently authored as Fix**. Ordinary `AddSketchRelationCommand` currently rejects linked projected targets: keep this exclusion in A2 unless a *separately accepted exact D2* adds safe provider-backed relation referencing.

### 1.4 Dimension gesture and first value

`Length(Line)`, `Radius(Circle)`, and `Diameter(Circle)` each require **one** explicitly selected semantic source. On final target click, compute the current accepted evaluated source measure from the exact current Part/Sketch stage. Create the Driving dimension **with that same initial numeric value** and a deterministic initial model-relative graphic placement, solving and committing atomically. No geometry jump, no intermediate value dialog, no placement confirmation. The annotation is immediately visible and editable. A type plus selection-first source may create the dimension without another viewport click. If one independent Circle radius or diameter driver already exists, do not silently add a second; choose the existing annotation for editing/inspection, or report typed `already_driven`, without consuming an ID.

Numeric truth is explicit length (internal canonical millimetres, displayed in current Part length unit); `R` and `Ø` are label presentation only. Decimal and units follow the **existing shared CAD quantity parser**, not a Sketch-local parser. Nonfinite/degenerate/negative-or-zero geometries cannot form a Driving value. Editing a value later, not creating it, is an explicit `Enter` commit.

### 1.5 Native 3D annotation, on-label edit and move

- Project true dimension anatomy (witness lines, dimension line, arrowheads, radius/diameter leader and value text) into **the active 3D Part Sketch Edit viewer** from stage-valid effective geometry. Annotation belongs to one durable semantic Driving owner `ConstraintId`, not a numeric screen token. Any selected glyph/label targets its owner, not the underlying Line/Circle. A1 nonnumeric relations show selectable glyphs by `ConstraintId` and a clear icon/text diagnostic.
- LMB selects the glyph/dimension; double LMB **on the visible numeric label** opens a keyboard-accessible Qt text editor over that label, not in the Operations toolbar. The selected existing `ConstraintId` persists across a compatible change of value. Initial editor content is the current authored value, formatted with existing units and precision. `Enter`: validate → numeric solve/independent residuals and DoF checks → commit one revision and Undo entry; equivalent same-value input is no-op. `Esc`: abort editor and its preview with **zero** mutation; invalid text leaves editor open and shows typed correction/status. No implicit commit on focus loss, selection change, Save/Close, document switch or tool switch. `Delete`/`Backspace` while editing text edits text, **not** the CAD dimension.
- LMB drag an existing label or explicit placement handle beyond defined logical-pixel threshold: live **placement-only** preview; release commits one independent placement transaction/Undo step, preserving numeric value, solver candidate, ID, attachment, DoF and geometry. Esc during drag drops preview without mutation. A click below threshold selects only; selected body-grip/direct-manipulation and 3D camera input retain established priority when no annotation is hit. Draw hit testing with accessible selection, overlap handling, HiDPI and rotated/isometric camera tests; ambiguous exact overlay hit must not mutate the wrong dimension.
- Persist placement/format as Sketch-owned **semantic-relative intent**, e.g. donor-inspired `LinearDimensionPlacement(along_offset, normal_offset)` or `RadialDimensionPlacement(radial_offset, tangential_offset)` relative to attached source in Sketch U/V; not absolute viewport px, OCCT handle or world-screen position. On camera rotation, zoom, pan, frame change, full cold reconstruct, display text remains attached and readable; occluded/offscreen annotation remains retrievable through a semantic Properties/Operations list or equivalent agreed UI route. Moving existing label is required, not merely picking initial placement.
- When current effective geometry or Part stage is failed/unresolved, do not display stale labels as current measurements or create hidden interactive editor over last-good geometry; keep authored constraint/placement inspectable with explicit unresolved/blocked diagnostic.

### 1.6 Removal, frozen geometry, Undo and focus

`Select glyph / Driving annotation → Delete` removes **only the selected semantic constraint and its associated annotation**. It never deletes the Line/Circle. For both relation and Driving deletion, **materialize the exact last accepted, current-incarnation, current-revision/stage effective geometry of the affected local Sketch into valid authored seeds** and remove the selected constraint/annotation **within one guarded Part transaction**, leaving all other authored relations, entities, profile/feature intent and linked PG-01 targets intact. Remaining constraints must still pass full acceptance; if not, reject atomically. Undo restores the exact previous authored seeds/constraint ID/placement; Redo returns to deterministic deletion. No unexpected geometric jump, no ID reuse or consumed ID on rejected action. `Fix Current Position` similarly captures the verified current solution *before* fixing it; it does not freeze historical raw seeds by accident.

Deleting **underlying Sketch geometry**, as opposed to the selected annotation/glyph, follows existing A1 cascade + owner-approved dependency-impact confirmation and Part repair-required policy B. Do not add new confirmation to ordinary constraint/dimension Delete: the user explicitly rejected such workflow friction. For a geometry deletion with downstream impact, do not silently remove dependent Part Features or override the existing impact gate.

## 2. Architecture and donor-adaptation boundary (D2 decisions to ratify)

### 2.1 One model and one history

Keep Part-owned Sketch; `sketch::SketchModelState` owns authored line/circle/arc seeds and durable constraints, while `DocumentSession` and Part transactions own revision, native Save, Undo/Redo and stale state validation. The Viewer, Qt controls and Operations are **input/projection only**. Do not instantiate donor `SketchSession` as a second Part history; do not persist solver data, native graph, runtime values, Qt glyph handles, raw OCCT IDs or screen positions. No global parametric dependency graph or Assembly/Drawings.

**Identity choice:** Driving numeric constraints retain `ConstraintId` (and A1's monotonic high-water). Its `DrivingDimensionPresentationRecord` is keyed by that `ConstraintId` with format and model-relative placement, rather than creating a second authoritative `DimensionId` for the same Driving equation. SS1 has this proven design (`DrivingDimensionOwner{ConstraintId}`). **`DimensionId` is reserved for durable Reference dimensions in A3**, unless Owner approves a conflicting D2 schema choice. Numeric kinds must preserve existing six A1 kind encodings; use explicit enum/kind version mapping and strictly validate all fields, not casually reorder enum ordinals.

### 2.2 Extend the solver *beyond* A1's Line-only gateway

The actual SS2 `LocalSolveProblem` in A1 accepts only `LocalSolveLineInput` and five non-Fix line equations; its `PlaneGcsDevSolver` does not solve Circles/Length/Radius/Diameter. A2 must **extend the solver-neutral problem/candidate/residual/contracts to Circle**, add semantic numeric equation records and typed units, then bring in tested SS1 mappings (`DistanceConstraint`, `RadiusConstraint`, `DiameterConstraint`). Represent circle center/radius coherently, maintain type/role/identity, preserve fixed/linked immobility and full residual review; use stage-aware disposable solve, never `apply` a numerically unchecked candidate.

The existing A1 `AddSketchRelationCommand` merely authors and checks some structural conflicts; an A2 interactive UI must not treat its return value alone as proof that the new relation is numerically acceptable. Either introduce **one revision-guarded create-and-validate transaction** or a caller-neutral precheck plus a final repeated semantic/stage-aware execution check before commit. Acceptance must explicitly prove `Converged`, residual tolerance (units supplied by caller), fixed-source preservation, no conflicting/redundant relation, supported DoF diagnostics, and exact closure for Coincident going into Profile/OCCT. Re-validate after snapshot/revision changes; no candidate publication on error, even when the solver reports convergence.

When there is no licensed/approved/distributable backend in the actual Windows desktop app, **stop claiming end-user A2 is complete**. Optional developer-only solver mode is acceptable for an evidence gate but not full product acceptance. Pin and verify PlaneGCS 0.8.0 source/library, LGPL/relink/source-notice implications, Eigen/Boost linkage, installed consumer and real Windows CMake branch; external dependency sources may not be silently copied or network-fetched. SS1 native tests are donors, not a new SS2 Windows PASS.

### 2.3 Native schema proposal

Part `.ss2part` **v21** remains current until exact D2 freeze. **Candidate successor: v22** with strict additive authored numeric constraint kinds and numeric quantity, `driving_dimension_presentations` keyed to existing `ConstraintId` with explicit source/placement/format; preserving A1 `next_constraint_id` high-water across Undo/Redo and cold reopen. No separate Driving `DimensionId`. The exact serialized key names, version conversion, validation, ordering, and missing/extra-field policy must be frozen as a standalone **schema addendum** before writing the store. A legacy v21 valid file opens without silent rewrite; only explicit guarded Save creates v22. ModelingSemanticsVersion stays v2. Reopen uses a fresh Part incarnation and provider; equality/dirty/Undo/Redo must incorporate all newly authored presentation intent. Strict file corruption tests reject stale/cross-Sketch/reused IDs, broken source references, invalid placement and invalid unit values.

### 2.4 Qt/Viewer and Operations integration

Reuse SS2's existing `CadWorkbench` sketch Create group, `CadWorkbenchShell` responsive toolbar/Operations layout, `PartSketchInteractionController` semantic input, `PartViewportController` stage-valid current interaction model, `viewer::IDocumentViewport` neutral presentation contract and `qt_occt_viewer_widget` renderer/input. Add the narrowest new **provider-neutral SketchConstraintGlyphScene / SketchDimensionAnnotationScene** with durable semantic IDs in their input records and transient geometry/screen hit projections, rather than designing an unrelated drawing engine. A dedicated Qt inline editor is an ephemeral overlay anchored to current projected dimension-label bounds. Existing `SketchDynamicInputOverlay`, `Measure` cues and CAD Input semantics must not be hijacked as durable dimensions. Annotations and geometry must derive from the **same exact effective stage/candidate** during preview/commit, never a mix of old labels and new geometry.

The `Operations` panel has one active `Relations` or `Dimensions` tool; clicking a type sets its semantic kind. Do not add a new source of command meaning via Qt callbacks. Keep `Show Constraints` and `Show Dimensions` there, with independent view-only visibility, accessible labels and true focus handling. Support selected owner status/diagnostics/DoF and precise delete without relying on provider tokens.

### 2.5 Donor map — reuse narrowly

| Donor SS1 proof/code family | Adapt in SS2 | Not portable wholesale |
|---|---|---|
| `SimpleSolid_Sketcher/.../dimension_state.hpp` | Driving `ConstraintId` owner, typed linear/radial placement, format | Donor's separate Sketch model and all A3 Reference catalog |
| `.../interaction/dimension_tool.hpp` | Typed target acquisition, duplicate detection, solving edit, Esc | Old create-with-value/placement-confirm interaction; Owner chose immediate current-value creation |
| `.../interaction/dimension_annotation.hpp` | Dimension geometry anchors, witness/leader calculations | Donor standalone 2D canvas and direct screen coordinates |
| `.../src/solver/planegcs_sketch_solver.cpp` | Numeric equation mapping, typed DoF/conflict/redundancy, residual tests | Donor ownership, packaging and direct vendor dependency |
| `.../src/application/sketch_controller.cpp` | Stable glyph/annotation selection, create/delete/edit state | Second history; inferred relations on snap |
| `.../Sketcher_Qt/src/sketch_canvas*.cpp` | Technical glyphs/text rendering, selection affordances | Old toolbar menus and 2D input implementation |
| Donor `dimension_tool_test.cpp`, native solver, Qt tests | Adapt minimal focused fixtures + targeted regression | Donor PASS as proof of SS2 product readiness |

**Known donor gap:** numeric Qt `QLineEdit` currently lives in SketchEditorWidget's **toolbar**, not as a proven edit widget directly anchored to a label in SS2's 3D viewport; existing-label dragging is not established as a ready-to-copy shipped gesture. Both are new A2 work requiring real native tests.

## 3. Controlled slices and hard gates

**A2.0 — interactive A1 relations plus solver availability.** Register real Windows RED tests for Create group controls and six Operations kinds, selected→type / type→pick semantics, endpoint roles, glyphs, Delete with geometry freeze, Fix Current Position, independent visibility and error messages. Integrate actual Part-stage solver validation (not only a headless Add). Once the solver packaging decision permits end-user distribution, demonstrate real Qt/OCCT Sketch Edit gestures and all existing A1 lifecycle guards. Native controlled developer-only evidence can precede but cannot replace the shipped solver gate.

**A2.1 — Driving domain / numeric equations / strict native schema.** RED first: exactly Length(Line), Radius(Circle), Diameter(Circle); one size driver per Circle, initial value = effective measured geometry, no extra gesture; numeric edit stable ConstraintId; conflict/redundancy/DoF/residual; partial solve fails atomically. Exact Owner D2 schema addendum first, then real v21→v22 read+explicit Save, v22 strict codec, cold original-provider/Part-incarnation tests, Undo/Redo and downstream Profile/Extrude/Revolve repair.

**A2.2 — real 3D annotations & inline editing.** Real Windows desktop: native label/arrow/witness and selectable glyphs; double-click **visible numeric text** opens editor at that label; Enter valid, Esc cancels, invalid input retained, no focus-loss auto commit; existing-label drag commits placement only; HiDPI and camera pan/zoom/isometric/reopen; overlapping/hidden hit targets never mutate the wrong item. No interim Operations-only editor substitution allowed.

**A2.3 — integrated closure and documentation.** Full end-to-end representative Part Sketch + downstream Profile + Extrude/Revolve cold rebuild and failed-geometry repair; real registered Windows FOCUSED→FAST→Ready FULL Core/Kernel/Qt without reducing the pre-existing complete regression suite; DOC-01 as-built internal plus both canonical PL/EN product peers + regenerated Browser + `.\ss2.ps1 verify`; Owner Windows manual **Functional + Visual PASS**, independent technical FINAL, exact product PR-head proof and separate guarded merge approval. Follow accepted closure chain; successful donor tests are supplementary only.

### Mandatory acceptance fixtures

1. **Workflow speed:** one Line Horizontal click commits one relation/Undo, tool remains active; Parallel and Coincident require exactly two correctly typed semantic hits; no Apply/Cancel control exists; repeated authoring, selection-first and command-first yield equivalent authored output.
2. **No hidden automation:** OSNAP, Polar, Rectangle, inferred tangent/midpoint guide create no relation implicitly; R13-A1 v21 files with authored zero relations reconstruct unchanged.
3. **Fix & deletion:** update after a valid solver move → Fix Current Position pins evaluated endpoints; unfix/delete preserves last valid geometry and restores exact original on Undo. Unrelated linked PG-01 and remaining constraints stay semantically stable. Stale/wrong incarnation rejects with no state/ID change.
4. **Numeric create:** current 35 mm Line → `Length` auto-creates 35 mm with no geometry jump or value dialog; current Circle Radius 10 mm → `Radius` auto 10; alternative `Diameter` 20; second independent Circle size driver blocked, does not spend ID.
5. **Numeric edit:** double click directly on real label → set `50 mm` → valid Enter re-solves, same ConstraintId, one Undo, current Profile/Extrude; invalid units/0/conflicting/NaN reject, editor remains repairable, Esc cancels editor only, no ghost revision.
6. **Placement:** drag label across sketch changes only relative placement and one Undo step, value and resolved geometry/DoF unchanged; Esc during drag zero changes; camera rotate/zoom/pan/DPI preserves selected semantic owner and readability.
7. **Delete Driving:** edit to 50 mm, Delete label → still 50 mm as unconstrained seed; no deleted Line/Circle; Undo returns driver and previous authored seeds; Redo deterministic; dirty/save/reopen retain correct state.
8. **Context:** multiple open Parts/sketches including same durable DocumentId/revision from different cold incarnations; stale provider/Sketch stage changes, failed projections, dialog focus and active command change never publish prior candidate/annotation or last-good BRep.
9. **Native lifecycle:** v21 read untouched until explicit guarded Save to v22; new process v22 reopen preserves constraint IDs/value/format/relative placement and recomputes derived geometry with a new provider; malformed data refuses with typed diagnostic.
10. **UI/Docs:** check 100/125/150/200% display scale, small viewport/Operations resize, toolbar overflow, group membership, Finish Sketch accessibility, native 3D glyph overlaps, both PL/EN section parity and Browser freshness.

## 4. Proposed exact finite path allowlist — not yet permission

**Sketch/domain and numerical bridge:**
- `src/sketch/include/simplesolid2/sketch/local_constraints.hpp`
- `src/sketch/include/simplesolid2/sketch/sketch_model.hpp`
- `src/sketch/sketch_model.cpp`
- `src/sketch/include/simplesolid2/sketch/interaction_state.hpp` (**A2 SketchTool semantic modes; discovered by current-code audit**)
- `src/sketch/interaction_state.cpp` (**candidate only if accepted SketchTool state design requires implementation**)
- `src/sketch/include/simplesolid2/sketch/local_solver.hpp`
- `src/sketch/local_solver.cpp`
- `src/sketch/include/simplesolid2/sketch/planegcs_dev_solver.hpp` (only if approved backend seam remains exactly this)
- `src/sketch/planegcs_dev_solver.cpp` (only if approved backend seam remains exactly this)
- **New, exact-name candidates:** `src/sketch/include/simplesolid2/sketch/driving_dimensions.hpp`, `src/sketch/driving_dimensions.cpp` (include/source only if added through approved exact-path amendment).

**Part/DocumentSession and strict store:**
- `src/application/include/simplesolid2/application/document_session.hpp`
- `src/application/document_session.cpp`
- `src/part/part_document.cpp`
- `src/part/part_document_store.cpp`
- `src/part/include/simplesolid2/part/part_document_store.hpp` (**current_schema_version=21 is defined here; required v22 change**)
- `src/part/feature_evaluation.cpp`
- `src/part/profile_kernel_input.cpp`
- `src/part/include/simplesolid2/part/effective_sketch_projection.hpp`

**UI/Viewer:**
- `src/ui/cad_workbench.hpp`
- `src/ui/cad_workbench.cpp`
- `src/ui/cad_workbench_shell.hpp`
- `src/ui/cad_workbench_shell.cpp`
- `src/ui/part_sketch_interaction_controller.hpp`
- `src/ui/part_sketch_interaction_controller.cpp`
- `src/ui/part_viewport_controller.hpp`
- `src/ui/part_viewport_controller.cpp`
- `src/viewer/include/simplesolid2/viewer/document_viewport.hpp`
- `src/viewer/include/simplesolid2/viewer/sketch_presentation.hpp`
- `src/viewer_qt_occt/include/simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp`
- `src/viewer_qt_occt/qt_occt_viewer_widget.cpp`

**Registered tests/CMake (exact existing plus proposed new names):**
- `tests/r13_a1_local_constraints_test.cpp`
- `tests/r13_a2_relation_ui_test.cpp` (**NEW candidate; explicit D2 exact-name approval required**)
- `tests/r13_a2_driving_dimensions_test.cpp` (**NEW candidate**)
- `tests/r13_a2_native_lifecycle_test.cpp` (**NEW candidate**)
- `tests/sk02b_part_sketch_model_test.cpp`
- `tests/sk02b_part_sketch_persistence_test.cpp`
- `tests/part_document_store_test.cpp`
- `tests/pm02f_stage_aware_profile_evaluation_test.cpp`
- `tests/sk03a_part_viewport_controller_test.cpp`
- `tests/sk04c_part_sketch_interaction_controller_test.cpp`
- `tests/sk04a_sketch_interaction_state_test.cpp` (**new authoring-mode state regression only if SketchTool extended**)
- `tests/sk03a_sketch_viewport_mapping_test.cpp` (**existing camera/Sketch frame projection regression candidate**)
- `tests/cad_workbench_test.cpp`
- `tests/sk03a_viewer_native_input_test.cpp`
- `src/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/core_only/CMakeLists.txt`
- `tests/kernel_native/CMakeLists.txt`
- `.github/workflows/windows-pr-gate.yml` (**only a separately justified exact D2 CI-matrix adjustment, not blanket permission**)

**Documentation candidates, exact DOC-01 scope:**
- `docs/internal/SHARED_2D.md`
- `docs/internal/PART_DOCUMENTS.md`
- `docs/internal/CAD_WORKBENCH_VIEWER.md`
- `docs/internal/PERSISTENCE.md`
- `docs/product/pl/PARTS.md`
- `docs/product/en/PARTS.md`
- `docs/browser/index.html` (**generated only, never manually edited**)

**Explicit exclusions:** `work/ACTIVE.yaml` in product PR, unlisted files, third_party vendoring, auto-constraints/inference authoring, A3 Reference + conversions, dimensions beyond three, Arc driver, 2D host replacement, unsupported projected-source relations, novel global parameter graph, Assembly/Drawings, unrelated PM/RG/OCCT topology policy, disabling or removing tests and unrestricted CMake/CI modifications. File candidates are **NOT** a presently accepted path allowlist; enumerate precisely with Owner at freeze and use an explicit addendum for further scope.

## 5. Documentation impact

**Internal docs: required.** Sketch solver numeric ownership, command lifecycle, typed status/DoF, Stage/Part incumbent provenance, strict v22 native format and 3D overlay seam change internal as-built meaning.  
**User/Product docs: required.** New Relations and Dimensions tool flows, click-to-create, inline label editing, dragging, visibility, Delete/Fix/Undo and failure/repair are user-visible behavior. PL/EN canonical peers must retain matching `doc-id` and ordered `section-id`; generated Product Browser regenerated and verified from Markdown. The old A1 headless-only current-state text must be updated as features actually ship; not falsely before.

## 6. Decisions to separately freeze before D2 execution

1. **Production solver gate:** exact PlaneGCS original 0.8.0 pin, Windows build integration, static/dynamic and LGPL-2.1-or-later distribution/notice/relink path, pinned Eigen/Boost, explicit Owner authorization. DEV opt-in + donor proof ≠ accepted end-user runtime.
2. **Strict v22 addendum:** exact representation of numeric driving constraints, label placement/precision, ordering/cursors, v21 read and guarded write, preservation of modeling semantics v2 and versioned reader tests.
3. **Annotation input and stage seam:** narrow provider-neutral scene records, exact-hit priority against grips/camera gestures, dimension text focus, viewport label editor widget and current Part incarnation/stage lease; no duplicated ownership.
4. **Delete/Freeze semantics for *all* relations:** formal Owner D2 extension to currently narrower A1 `EraseSketchConstraintCommand`; must be proven against surviving constraints, projection sources and downstream Body repair.
5. **Finite exact file-path allowlist + native Windows lanes + manual UX suite**, independently approved governance contract merge, later **ACTIVE-only** switch.

### 6A. Source-audited D2 freeze candidate — separate Owner gate, 2026-10-21

This **non-effective engineering clarification** closes file-path omissions discovered on SS2 `main@30a1715`. It is not production authorization and does not replace independent Owner approval of the exact full §4 allowlist. Evidence: `SketchTool` lives in `src/sketch/include/simplesolid2/sketch/interaction_state.hpp` and its implementation in `src/sketch/interaction_state.cpp`; the strict native file version constant lives in `src/part/include/simplesolid2/part/part_document_store.hpp` (`current_schema_version = 21`). §4 now names those paths and targeted existing tests. An implementation that needs a further path must STOP for an exact D2 addendum rather than silently expand the list.

**Candidate v22 shape, requiring Owner schema freeze before implementation**:

- The five-key `sketches[].model` shape would be exactly `next_entity_id`, `entities`, `next_constraint_id`, `constraints` and `driving_dimension_presentations`. A legacy **v21** record remains **exactly four-key**, and **v20** exactly two-key, with their original strict readers; v21/v20 reads never silently rewrite the file.
- Existing six A1 relation records retain their exact `{id,kind,first,second}` encoding, including `Fix(Line)`; do not reinterpret an old serialized `kind`. Three **new typed numeric variants** are proposed: `length` targeting one Line; `radius` and `diameter` targeting one Circle. Numeric records would have exact `{id,kind,target,value_mm}` fields. `target` encodes semantic `EntityId` plus `element = line | circle`, never a screen hit, index, OCCT token, projected geometry handle, or duplicated geometry. The numeric unit is an explicitly finite, strictly positive **canonical millimetre** length (also for radial dimensions); no opaque formatted string as numeric truth.
- `driving_dimension_presentations` would contain one record per numeric `ConstraintId`, with strictly validated typed `format` (Auto or fixed decimal precision) and `placement` (linear along/normal or radial/tangential relative to the referenced source in Sketch U/V). No separate Driving `DimensionId`; records for deleted/missing/non-numeric/wrong-Sketch IDs fail closed, as do duplicates and nonfinite/out-of-range placement or precision. Existing relations without numeric annotations must not fabricate presentations. Explicitly freeze the exact nested serialization grammar, allowed precision limits and record ordering in the dedicated schema addendum; this paragraph proposes the **shape**, not a final codec.
- `SketchModel::restore`, JSON reader and Part reconstruction validate numeric target kinds, stable monotonic cursor/high-water, one independent Circle size driver, same-Sketch ownership, v21 backward reader, and cold-new-process rebuild. Neither solved geometry, DoF, residuals, runtime Part incarnation, UI visibility preference nor Qt/editor state may be serialized.
- Deletion requires **one guarded staged command** using current evaluated geometry and current Part incarnation/revision; copy valid local solution seeds first, erase exactly selected constraint and associated presentation, re-evaluate all surviving constraints, then commit or discard the entire candidate. For `Fix Current Position`, copy current valid evaluated local endpoints before authoring a Fix parameter mask. Never mutate projected/linked authored sources, and never save a partial result or stale BRep. Undo restores the original authored seeds and stable IDs.

**A2.0/A2.1 solver and UI dependency split**: Developer-only PlaneGCS-backed Qt tests may provide early evidence, but cannot count as end-user A2 acceptance. A2.0 must demonstrate a real current-stage numerical solve for all six relations; A2.1 additionally requires Line Length and Circle Radius/Diameter mapped through the SS2-neutral solver with independent residual and immutable-source checks. **No shipped claim until an Owner-reviewed Windows packaging/license/relink decision is actually implemented and tested**, independent of A1's optional DEV lane. If licensing is blocked, leave the UI or model slice explicitly provisional and STOP at the product gate.

**Native GUI evidence**: exercise the actual 3D Qt/OCCT viewport, not a synthetic 2D host. Verify creation with one/two clicks, precise Start/End selection, selection-first behavior, same-gesture undo, no popup Apply, repeated authoring, `Esc` without Finish Sketch, label double-click editing, camera navigation vs drag arbitration, overlapping annotation hit-test, focus/Backspace ownership, 100–200% DPI and `Finish Sketch` footer on the screenshot's narrow Operations panel. These scenarios are mandatory before Owner manual acceptance.

**Independence of permissions**: A2 product execution remains blocked until the exact D2 contract and finite path list are accepted, a governance-only PR is merged with a separate explicit Owner guarded-squash authorization, and a **separate, one-file ACTIVE-only PR** passes closure and merges under its own separate authorization. Product readiness later also requires independent licensing/schema/visual D2 gates and exact-head Windows product tests. A successful governance CLOSURE proves only the governance lane, not the full product test suite.

### 6B. Owner disposition of the D2 audit — ACCEPTED DESIGN, CONDITIONAL FILE SCOPE (2026-10-21)

**Recorded Owner response:** after review of the exact PR #521 audit outcome (one-file governance PR `16aa7c7dd6fa3b63590385e117977523d1d00700`, Windows governance CLOSURE `39655543647` **SUCCESS**), the Owner explicitly answered `akceptuję - kontynuuj`. This accepts the documented A2 workflow, six interactive A1 relations, three Driving dimension kinds, SS1 donor adaptation boundary, **Freeze Current Position / Delete preserves accepted solved geometry**, and the D2 implementation direction of §2, §4 and §6A. For execution, the **finite set of concrete paths named in §4 is accepted as the conditional maximum**, including the audited `interaction_state.hpp/.cpp` and `part_document_store.hpp` additions, existing tests and specifically named new candidate files. This is **NOT** carte blanche for all `src/**`, `tests/**`, `third_party/**` or the complete list to be edited in every slice. Change only those paths necessary for each approved A2 slice; no new paths except a separately Owner-accepted D2 exact-path addendum.

**Acceptance does not assert unproven implementation details:** exact strict v22 nested fields/ordering/precision bounds must be separately reviewed and frozen before modifying the store; v21 remains authoritative until then. The provider-neutral annotation data/selection/focus contract requires its concrete types and validation to be reviewed against the actual native viewer before implementing that seam. PlaneGCS production linking, LGPL notice/relink/source pathway, library pin and installed-consumer run require independent Owner D2 disposition before shipping; `SS2_R13_PLANEGCS_DEV` is not production clearance. All are hard STOP points, not silently resolved by this acceptance.

**Governance activation remains two independent future changes**:

1. This one-file contract PR may be marked **Ready for review** after successful exact-head governance CLOSURE, but **must not** be merged without a *separate, explicit Owner guarded-squash authorization referring to PR #521 and its final HEAD*.
2. Once the contract is guarded-merged to protected main and read back, propose a **distinct one-file `work/ACTIVE.yaml`** activation PR pointing to the integrated contract (no production source in that PR), obtain its own exact-head governance CLOSURE and **separate explicit Owner guarded-squash authorization**. Only after verified activation may accepted A2.0 product implementation start on its own bounded branch. Subsequent A2.1/A2.2 remain subject to their specific D2/solver/schema/GUI gates and Owner review, even though the full program scope is now accepted.

The unmerged historical PR #493 remains non-authoritative for any conflicting older authoring gesture; this accepted A2 contract expressly supersedes its placement/value-before-create proposal. The already merged A1 implementation is not retroactively treated as a user-facing Qt relations UI.

## 7. Closure statement

This is an **Owner-accepted D2 work-contract design in an unmerged governance PR**, not an activated executable contract. It makes **no representation** that `ACTIVE.yaml` changed or that production solver licensing/build/Windows tests passed for A2. This draft governance PR changes only this `work/` contract; no product source, tests, CI, as-built/product documentation, `ACTIVE.yaml` or CAD document is altered.

### Primary references

- SS2: https://github.com/MechanicalCave/SimpleSolid2/blob/main/work/SKETCH_ROADMAP.md
- SS2: https://github.com/MechanicalCave/SimpleSolid2/blob/main/work/ACTIVE.yaml
- SS2: https://github.com/MechanicalCave/SimpleSolid2/blob/main/src/sketch/include/simplesolid2/sketch/local_solver.hpp
- SS2: https://github.com/MechanicalCave/SimpleSolid2/blob/main/src/application/document_session.cpp
- SS2: https://github.com/MechanicalCave/SimpleSolid2/pull/506
- SS2 historical draft: https://github.com/MechanicalCave/SimpleSolid2/pull/493
- Donor: https://github.com/MechanicalCave/SimpleSolid/blob/main/components/SimpleSolid_Sketcher/include/simplesolid/sketch/dimension_state.hpp
- Donor: https://github.com/MechanicalCave/SimpleSolid/blob/main/components/SimpleSolid_Sketcher/include/simplesolid/sketch/interaction/dimension_tool.hpp
- Donor: https://github.com/MechanicalCave/SimpleSolid/blob/main/components/SimpleSolid_Sketcher/src/solver/planegcs_sketch_solver.cpp
- Donor: https://github.com/MechanicalCave/SimpleSolid/blob/main/components/SimpleSolid_Sketcher_Qt/src/sketch_editor_widget.cpp
- Donor: https://github.com/MechanicalCave/SimpleSolid/blob/main/docs/work/SK-02/S2_CONTRACT.md
