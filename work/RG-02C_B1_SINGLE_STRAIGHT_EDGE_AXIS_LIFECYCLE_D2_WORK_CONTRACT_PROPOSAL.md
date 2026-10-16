# RG-02C/B1 — single straight Body Edge Axis with full lifecycle: bounded technical D2 Work Contract PROPOSAL

**Status:** **OWNER DELEGATED AUTHORITY TO START IMPLEMENTATION (2026-10-15)**: `masz wszystkie niezbędne zgody do uruchomienia implementacji - kontynuuj prace i nie zatrzymuj z formalnych powodów`. The bounded, minimal technical architecture and allowed paths in §0A are chosen as a **conservative D2 implementation baseline under that delegation**, with real technical safety gates retained. This governance PR is NOT YET MERGED / NOT YET ACTIVE / source edits must wait for accepted-contract and ACTIVE-only squash to land.
**Date:** 2026-10-15. **Baseline:** protected main `38b0c8b274d47548647fc7c27e9a7884fd85ff20`, A0 native evidence and Owner A0 technical FINAL PASS merged via PR #408. UX signoff for **one `Edge` constructor** is recorded on separate unmerged UX [PR #410](https://github.com/MechanicalCave/SimpleSolid2/pull/410); this technical D2 planning PR starts from the same pre-UX-merge main and cannot treat PR #410 as already merged/active. Separate A0 historical closeout [PR #409](https://github.com/MechanicalCave/SimpleSolid2/pull/409) remains unmerged until a separate Owner guarded squash.
**Governing authority:** Constitution → frozen Foundation 1.0 → Architecture/accepted ADRs → `work/ACTIVE.yaml` → Part Modeling v1.33 §6/§20B → RG-02 Owner design §2.3/§3, existing PM-04 Axis+Revolve, A0/Q0/Q2 strict semantic material-Edge evidence, DOC-01. This is the **next one-constructor technical proposal only**, NOT approval of PM-06 or any other Axis source kind.
**Owner UX decisions already made:** Work Features gets one visible `Axis` button next to `Datum Plane`; right Operations `Constructor` has **ONLY** `Edge`, though the combo is future-extensible; preexisting `Sketch Line` Axis is authored **within Sketch**, stays in the Sketch subtree; only newly Edge-sourced axes appear alongside Datum Planes under `Reference Geometry`. Complete Edge Axis Create/Edit/Replace/Delete/visibility/Undo/Redo/Save/Reopen/repair/Revolve lifecycle; reuse code and tests, no duplicated logic. Exact reviewed proposal: `work/RG-02C_B1_SINGLE_STRAIGHT_BODY_EDGE_AXIS_EXISTING_TOOL_UX_CONSULTATION_PROPOSAL.md`, PR #410, Owner UX signoff 2026-10-15. This signed UX does NOT authorize its implementation until independent technical D2 acceptance + governance + ACTIVE activation.

## Owner B1 acceptance boundary — 2026-10-16: native orientation fixture deferred, Revolve preview required

**Owner direct product decision:** In manual B1 testing, the authored Edge Axis is stable, Revolve and its **Reverse** option work. The Owner explicitly does not require a native `TopoDS_Edge` orientation-reversal fixture for **this SS2 version** because changing the reference operation's topology is understood to invalidate its dependent operations. This is a **bounded risk deferral**, not a native test PASS and not a proof that every conceivable provider-only orientation change invalidates a semantic reference. The A0/B1 native reversal fixture remains **GAP / NOT TESTED** in evidence, is **not** an SS2-v1 signoff blocker under this explicit Owner decision, and must be revisited before extending contracts promising cross-topology continuation. No `src/kernel_occt/**` seam, coordinate fallback or implicit durable source rebinding is authorized. The abandoned native test-seam proposal draft PR #423 is not activated.

**Actual newly reported manual defect:** Revolve based on the Edge Axis completes correctly but **lacks visible preview**; this is the current B1 deliverable blocker and must be fixed. Reuse the already-allowlisted `src/application/document_session.cpp` and the existing native Qt/OCCT `tests/pm05f_r2_native_workbench_edit_test.cpp`: provider-aware draft preview must receive the original exact native Edge provider just as full Feature evaluation already does, preserve existing Origin/Sketch-Line Revolve behavior, fail closed without native geometry, and never mutate document/Undo. Require a real native regression of nonempty Cut/Add preview mesh from a valid Edge Axis, real Windows FULL plus DOC-01 as applicable, and Owner manual retest of **preview**, prior to separate Owner technical FINAL PASS and distinct guarded main merge authorization. This direction extends no production file path or feature semantic grammar.

## 0A. Exact bounded technical decisions and production allowlist under Owner's explicit go-ahead

The Owner first **explicitly accepted** the corrected one-Edge UX on PR #410, and then independently instructed **`masz wszystkie niezbędne zgody do uruchomienia implementacji - kontynuuj prace i nie zatrzymuj z formalnych powodów`**. This is authorization to continue through distinct existing protected-main governance gates **without repeated conversational permission requests**, NOT permission to ignore tests, identity or persistence failure. Record the following smallest conservative choices transparently; code deviations that change semantic meaning or unexpectedly broaden a product boundary require another technical D2.

**First implementation order:** deliver **B1-H1 headless semantic source and durable native persistence + real native test/rebuild**, then integrate the already Owner-approved B1-E1 single `Edge` Work Features UI and full interaction on the same bounded Work Contract **only when** H1 tests have passed. Do not ship H1 as a complete user-visible feature and do not add a placeholder Axis button. No other constructor or work contract becomes active.

**Semantic source:** one `PartAxis` source variant over the unchanged `SketchLineAxisSource` and a **`StraightMaterialEdgeAxisSource` containing exactly existing strict `MaterialEdgeReference` and its BodyStageRef**. Keep `AxisId` and `AxisReference` unchanged, preserve old Sketch-Line authoring inside Sketch and Tree placement. The body-edge constructor is a future `Edge`-only Work Features option. No placeholder fake SketchId/EntityId for Body sources; no persisted provider token, cached coordinate or Axis sign. Existing Q0/Q2 stage-aware semantic resolver plus kernel scoped exact straight-edge geometry are the sole truth; product Axis evaluation will receive a real provider via an approved narrow seam, or fail closed when none exists. No duplicate A0 test helper as product resolver.

**Native schema:** use **explicit new Part native schema v19** for new writer once durable source kind is added, with **v18 and older supported readers** preserving the exact old Sketch Line interpretation; old v18 records are never silently read as material Body Edge Axis. v19 Axis source discrimination must be typed and reject malformed/unknown kinds. No change to Foundation or ModelingSemanticsVersion by assumption; a real test must prove legacy fixtures, roundtrip, cold-provider reproducibility and rejected invalid source. Do not introduce format until the parsing/writing tests can be run. The previous all-sources-exact v18 layout is the compatibility benchmark, not merely a positive load smoke.

**Revolve:** authored `AxisReference` points to a stable AxisId independent of source. Edge Axis geometrically defines **one unoriented infinite line**; its underlying native TopoDS_Edge reversal/order must not author signed direction. Existing Revolve signed-direction controls remain the consumer authority, and existing Sketch-Line Axis direction semantics cannot change. Missing/ambiguous/stale/unsupported/replaced Body Edge disables dependent evaluation, never substitutes a last-good line.

**Native reversal fixture:** treat the A0 untested native B-Rep topological reversal as a **production acceptance requirement to exercise or explicitly report RED/GAP**, not as an already proven PASS; if it cannot be tested inside the explicitly allowed native source using existing provider interfaces, report why and STOP before user-visible completion rather than relaxing evidence or editing `src/kernel_occt/**` without a separate D2.

**Exact allowed production file paths for this ONE B1 contract** (each optional unless necessary; do not modify unrelated files):

```text
src/part/include/simplesolid2/part/axis.hpp
src/part/axis.cpp
src/part/include/simplesolid2/part/axis_evaluation.hpp
src/part/axis_evaluation.cpp
src/part/feature_evaluation.cpp
src/part/part_document.cpp
src/part/include/simplesolid2/part/part_document_store.hpp
src/part/part_document_store.cpp
src/application/include/simplesolid2/application/document_session.hpp
src/application/document_session.cpp
src/application/include/simplesolid2/application/axis_draft.hpp
src/application/axis_draft.cpp
src/ui/cad_workbench.hpp
src/ui/cad_workbench.cpp
src/ui/part_document_tree_controller.cpp
src/ui/part_viewport_controller.hpp
src/ui/part_viewport_controller.cpp
tests/pm04a_axis_schema_v12_test.cpp
tests/pm04b_axis_lifecycle_draft_test.cpp
tests/pm04c1_revolve_semantic_kernel_test.cpp
tests/pm04e1_revolve_persistence_test.cpp
tests/pm04e2_revolve_lifecycle_cold_rebuild_test.cpp
tests/pm02e_sketch_support_schema_v9_test.cpp
tests/pm05b3_generated_surface_reference_v14_test.cpp
tests/sk04c_part_sketch_interaction_controller_test.cpp
tests/pm04b2_axis_tree_viewer_test.cpp
tests/pm04b2_axis_tree_viewer_ui_test.cpp
tests/pm03c2_cad_workbench_datum_plane_test.cpp
tests/pm05f_r2_native_workbench_edit_test.cpp
tests/kernel_native/pm05c2b_part_edge_topology_test.cpp
tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp
tests/pm03a_datum_schema_v10_test.cpp
tests/pm03e_datum_backed_sketch_test.cpp
tests/pm05b1_edge_feature_schema_v14_test.cpp
docs/internal/PART_DOCUMENTS.md
docs/product/pl/PARTS.md
docs/product/en/PARTS.md
docs/browser/index.html
work/RG-02C_B1_EDGE_AXIS_ACCEPTANCE_EVIDENCE.md
```

The evidence file is newly created by this implementation only, not a source of invented test results. `docs/browser/index.html` is generated by official DOC-01 procedure, never hand-edited. All other paths, especially `tests/CMakeLists.txt`, `.github/workflows/**`, `src/kernel/**`, `src/kernel_occt/**`, `src/viewer/**`, `src/viewer_qt_occt/**`, remain STOP unless a specific amendment is separately documented. This list is the executable allowlist only **after separate exact-head contract merge and `ACTIVE.yaml` activation**. H1 implementation starts with the narrowest subset; do not write all files just because they appear in the list.

**Narrow existing Sketch Axis Tree/Undo fixture test compatibility addendum — 2026-10-15:** Windows H2 Draft FAST [#3260](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38781068285) found two source-field accesses in already registered `tests/sk04c_part_sketch_interaction_controller_test.cpp` (around 1295, 1349) which presumed `PartAxis::source` was always `SketchLineAxisSource`. Since B1 intentionally introduces a discriminated `PartAxisSource`, this is a **test syntax adaptation**, not new behavior: use a type-checked SketchLine source payload to access `EntityId`, retain exact same assertions covering Sketch authored Axis, Line deletion, Undo/Redo and original source identity. It MUST NOT move existing Sketch Axis Tree rows, relax any interaction or undo check, replace tests with mocks, or affect other `tests/**` paths. This one additional path becomes writable only after exact-head governance check and protected-main squash of the containing Work Contract PR. No new compiler/test target, UI or semantic authority.

**Narrow v19 legacy assertion and Revolve integration addendum — 2026-10-15:** The user's express delegation to implement also permits exactly three additional existing paths in the allowlist above, and no others. `tests/pm02e_sketch_support_schema_v9_test.cpp` and `tests/pm05b3_generated_surface_reference_v14_test.cpp` may change **ONLY** assertions of the NEW writer's native current domain schema v18 → v19 when objectively emitted by the current writer; preserve existing old-file v9/v14/v18 fixture parsing, malformed-source rejection and historical provenance, without skipping/weakening any tests. `src/part/feature_evaluation.cpp` is needed **ONLY** to pass the same optional exact provider from Part evaluation into Revolve's existing AxisReference resolver for a new Edge Axis; preserve all Sketch Line/Origin Revolve signed and stage semantics and existing Q0/Q2 reuse. This bounded amendment is not authority for `src/kernel_occt/**`, other tests, new CMake/CI, unrelated operations or future Axis constructors.

**CI:** genuine Windows FOCUSED (already-registered native Axis/schema/test targets) → Draft FAST → Ready FULL Core+native Kernel+real Desktop Qt/OCCT; official DOC-01, exact-head evidence CLOSURE, Owner manual full lifecycle, technical FINAL PASS and guarded production squash only after PASS. Preserve all existing tests and CTest resource locks; no skipped Full counted as completed Full.

**B1 delivery scope remains the Owner-corrected and signed UX on PR #410**. This technical implementation consent does not license `Sketch Line` as a Work Features constructor or moving existing Sketch Axis from its source Sketch. Every new Edge Axis belongs to Reference Geometry. PM-06/G1/other Axis kinds remain inactive.

### B1-D2 current native Part writer compatibility witness — narrow Owner-delegated extension (2026-10-15)

**Reason from real RED (no waived tests):** Windows Ready FULL [#3282](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38790204731) built the entire Desktop graph successfully and Core **25/25 PASS**, but native Kernel **56/57 PASS (1 RED)**: existing registered `pm02jr2.add_surface_continuation` line 1521 asserts that a **newly written** Part package descriptor is exactly native schema `18`. The accepted B1 new writer is explicitly schema **v19**, so the exact current-writer assertion must become `19`. The same test's *separate historic v16 read fixture*, v15/v16 discriminant refusal and all actual legacy negative tests must stay unmodified. This is version consistency, **not** changing old files or disabling tests. Real native B-Rep topological Edge orientation-reversal GAP remains separate.

**Exactly ONE additional executable file to the finite §0A allowlist:** `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp`, **only** the current Part writer descriptor expected version assertion `18 → 19`. No other source, helper, new target, CMake/CI, new resolver, test deletion or additional constructor is authorized. Owner already expressly delegated continuation through bounded technical corrections; retain exact-head governance and Windows FOCUSED/Ready FULL gates after change.

### B1-D2 three existing Desktop native writer-version assertions — RED-driven addendum (2026-10-15)

**Real Windows FULL evidence:** after bounded legacy-axis PM-04 compatibility fixes and single PM-02JR2 current writer `v19` correction, [FULL #3285](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38791599358) compiled the whole Desktop and passed real Core **25/25** and native Kernel **57/57**. Desktop **113/117** with four independent RED: PM-04B2 `axis_tool == nullptr` conflicts with the Owner-approved **new visible Work Features Axis button**, and exactly three existing UI-era test entry guards assert the **CURRENT writer** `PartDocumentStore::current_schema_version == 18` despite accepted writer v19: PM-03A, PM-03E and PM-05B1. PM-04B2 is ALREADY in §0A; do not create a duplicate test or UI implementation.

**ONLY three new exact allowed test paths** are `tests/pm03a_datum_schema_v10_test.cpp`, `tests/pm03e_datum_backed_sketch_test.cpp`, `tests/pm05b1_edge_feature_schema_v14_test.cpp` (already registered and executed). Change ONLY their current-writer top-level `18 → 19` assertion. Keep original v10/v14/older native fixtures, all explicit historical schema values, Datum/Sketch/body semantics, negative validations, CTest registration and full assertions UNCHANGED. Do not downgrade production writer or relabel a historic v18 file as v19.

**Already-authorized PM-04B2 UI compatibility correction:** in `tests/pm04b2_axis_tree_viewer_test.cpp`, replace the obsolete `axis_tool == nullptr` assertion with a positive check for the physical one-row Work Features `Axis` button, retaining the assertion that authored Sketch-Line Axis stays under its source Sketch; no move to Reference Geometry. New `Edge` Axis is placed under Reference Geometry, preserving original PM-04 behavior.

**Governance:** Owner delegated all required RG-02C/B1 implementation decisions, subject to strict scope and real test gates. This addendum changes ONLY this existing `work/**` contract; exact-head Windows governance success and guarded squash first. Then fix only these four source-tested assertions on the product PR branch and rerun Windows FULL. Do not waive genuine regressions, claim product FINAL PASS, or claim the B-Rep native reversed topological Edge fixture as tested.

## 1. What A0 truly proved and what is still missing

- A0 ran real OCCT native FOCUSED #3230 1/1 PASS, FAST #3231 93/93 PASS, Ready FULL #3232 199/199 PASS and DOC-01, closure #3234, and Owner accepted technical FINAL PASS with the **explicit GAP**: no real test fixture actually reverses native B-Rep topological Edge orientation. Mathematical tangent `+t/-t` and actual physical terminal comparison do NOT prove that missing provider case.
- A0's `A0AxisLine` / `a0StraightEdgeLine` live **only inside** already registered `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp`; they are not part of product `PartAxis`, not a persisted source, not production commands, and cannot be imported as a second parallel resolver. Product evaluation must reuse the proven **Part-owned** stage-aware `MaterialEdgeReference` author/resolve and scoped exact native Edge queries; test-only formulas may guide evidence but are not product APIs.
- Existing `src/part/include/simplesolid2/part/axis.hpp` defines `PartAxis::source` as **one `SketchLineAxisSource`**, not a source variant. Current `src/part/axis_evaluation.cpp` gets authored Axis support from the Sketch; `src/application/axis_draft.cpp` and `src/application/document_session.cpp` create/re-source Sketch Line Axes; `src/part/part_document_store.cpp` native schema is **v18**; `src/ui/cad_workbench.cpp` contains Axis draft/Operations infrastructure but no visible Work Features `Axis` button in current toolbar; current Tree nests Sketch Axes beneath Sketches. New Edge Axis cannot be achieved by merely exposing a button without bounded model, transaction, serialization and dependency changes.

## 2. Proposed smallest end-to-end product scope — EXACTLY ONE NEW SOURCE KIND

**New source kind: straight material Body Edge at explicit Body history stage.** Proposed Part-owned durable authoring carries a validated `MaterialEdgeReference` with its `BodyStageRef`, not a native Edge token, topological index, camera direction, tessellated midpoint, world-XYZ, stored last-good line, or detached B-Rep. One source per authored `AxisId`; `AxisReference` to Origin X/Y/Z or authored AxisId remains unchanged.

**Required architecture D2:** use a discriminated source representation for `SketchLineAxisSource` **or** `StraightMaterialEdgeAxisSource` (shape/compatibility to be agreed). Avoid layering `optional<Edge>` over a fabricated Sketch Line source. Existing schema-v18 Sketch-Line Axis records must round-trip with identical identity and semantic meaning. A new native schema version **is proposed as a separately ratified option, NOT implicitly approved**; determine reader/writer field grammar, backward load policy and invariant before editing `part_document_store.cpp`. Do not change ModelingSemanticsVersion or silently reinterpret old Axis records without Owner D2.

**Precise Axis meaning:** the geometry is an **unoriented infinite line** through the bounded straight material Edge. Its evaluated point/tangent are derived anew from the actual current exact native line; `u` and `-u` describe the same Axis. No positive source direction or Axis `Reverse`. If Revolve needs a signed direction, **Revolve feature's own authored Reverse/angle direction** retains current semantics; native Edge parameter orientation never chooses it. The interaction between current PM-04 deterministic Sketch Line direction and new unoriented Edge source needs a deliberate regression matrix; do not silently rewrite existing PM-04 behavior.

**Stage/dependencies:** author only certified material Edge from one actual current valid BodyStageRef, earlier than dependent consumer stage and with no dependency cycles. Later Features do not retarget the source to the final Body or a same-looking Edge. Upstream edits re-resolve source or fail closed with structured Missing/Ambiguous/Unsupported/Blocked/Stale diagnostics; no last-good geometry. A producer replacement with equal B-Rep but different FeatureId does not steal source meaning.

**Existing identity and complete lifecycle:** one stable Part-owned `AxisId`, one native `PartAxis` object, same visibility/edit/delete commands and transaction/Undo protocols; no new independent Axis registry, CommandDispatcher, global Point subsystem or standalone Datum Axis. Create/Finish one transaction; Cancel/invalid pick/preview/hover zero mutations; Edit/Replace source preserves AxisId, visibility and dependent Revolve AxisReference; Delete retains fail-closed downstream authored references, Undo restores, Redo stable. Real native Part Save/Close/Reopen and fresh OCCT provider reconstruct same source stage and derived line, or retain repairable authored source with typed failure. Show/Hide never changes evaluation.

**Reviewed UX:** after technical semantics are independently accepted and activated, add the visible Work Features `Axis` button beside Datum Plane. `Operations / Axis / Constructor` has **one item only `Edge`**, one Edge source/status/stage/preview, sticky Finish/Cancel, existing Preview and Properties grammar. Edge-sourced Axis Tree row goes to Reference Geometry, existing Sketch-Line Axis stays under Sketch, Origin X/Y/Z under Origin. Use the existing viewport Edge selection and stage evidence; no `Sketch Line` constructor or `SKETCHLINE` CLI mode. CAD input must be scoped to the one Axis Edge draft with the existing Finish/Cancel/source controls; do not create new global verbs without additional Owner approval.

## 3. Candidate implementation path matrix — D2 REVIEW ONLY, NOT AN ACCEPTED ALLOWLIST

This is a **candidate** one-constructor vertical-slice change set. Before an executable Work Contract can be Owner-accepted, ratify **exact files** (trim the list whenever existing helpers are sufficient), serialization details, and whether a smaller first executable package should cover the headless model/lifecycle before the UI. **No path in this list is authorized for writes now.**

| Existing area | Candidate production paths / reason | Reuse guard |
| --- | --- | --- |
| Part Axis source and evaluation | `src/part/include/simplesolid2/part/axis.hpp`, `src/part/axis.cpp`, `src/part/include/simplesolid2/part/axis_evaluation.hpp`, `src/part/axis_evaluation.cpp` | Preserve Sketch source evaluation, AxisReference, use current strict material Edge resolution and OCCT provider; no duplicate resolver |
| Native Part schema and model validation | `src/part/part_document.cpp`, `src/part/include/simplesolid2/part/part_document_store.hpp`, `src/part/part_document_store.cpp` | v18 reader backward compatibility and versioned new source; no native tokens; choose exact new format in D2 |
| Transactional Axis draft/commands | `src/application/include/simplesolid2/application/document_session.hpp`, `src/application/document_session.cpp`, `src/application/include/simplesolid2/application/axis_draft.hpp`, `src/application/axis_draft.cpp` | Extend existing Create/Edit/Delete/Visibility, document revisions, AxisId cursor, Undo rather than new commands framework |
| One Workbench/selection/UI path | `src/ui/cad_workbench.hpp`, `src/ui/cad_workbench.cpp`, `src/ui/part_document_tree_controller.cpp`, conditional `src/ui/part_viewport_controller.hpp` and `src/ui/part_viewport_controller.cpp` ONLY if existing strict selection seam needs additive evidence | Same toolbar group, Axis Operations and footer, strict native Edge picking; Sketch-Line Axis Tree unchanged; no viewer engine rewrite |
| Already registered semantic/native tests | `tests/pm04a_axis_schema_v12_test.cpp`, `tests/pm04b_axis_lifecycle_draft_test.cpp`, `tests/pm04c1_revolve_semantic_kernel_test.cpp`, `tests/pm04e1_revolve_persistence_test.cpp`, `tests/pm04e2_revolve_lifecycle_cold_rebuild_test.cpp`, `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` (only new native reversal fixture if truly required) | Add new assertions to existing targets; retain Q0/Q2/A0 and PM-04 baseline, no duplicated helper/fixtures or new CMake registrations |
| Already registered UI/native regression | `tests/pm04b2_axis_tree_viewer_test.cpp`, `tests/pm04b2_axis_tree_viewer_ui_test.cpp`, `tests/pm03c2_cad_workbench_datum_plane_test.cpp`, `tests/pm05f_r2_native_workbench_edit_test.cpp` | Real native OCCT + Qt Create/Edit/Cancel/Tree/hover/cold persistence, preserve tests' existing resource locks |
| DOC-01 before production completion | `docs/internal/PART_DOCUMENTS.md`, `docs/product/pl/PARTS.md`, `docs/product/en/PARTS.md`, `docs/browser/index.html` generated only by official docs tool | Update actual as-built state when real feature ships; do not label proposed feature as delivered |
| Factual evidence | one future `work/RG-02C_B1_EDGE_AXIS_ACCEPTANCE_EVIDENCE.md` | Honest counts, source SHA, manual Owner review and real RED/GAP. Do not manufacture PASS |

**Excluded unless separately ratified:** `src/kernel/**`, `src/kernel_occt/**`, `src/viewer/**`, `src/viewer_qt_occt/**`, additional `tests/CMakeLists.txt`, `.github/workflows/**`, `scripts/**`, broadened native schema/compiler framework, additional Axis constructors, generic Point/MID/CENTER, Mirror/Pattern, G1 and PM-06. If an excluded API/file becomes necessary, STOP and return to Owner D2 rather than silently widen.

## 4. Required F01–F13 acceptance gates — prospective only

- **B1-F01:** Existing Sketch-line Axis still authored via Sketch and stays beneath source Sketch in Tree, same AxisId and existing PM-04 Create/Edit/Undo; `Constructor` of new Work Features Axis contains exactly `Edge`.
- **B1-F02:** Strict straight current-stage material Edge only: real OCCT physical edge, provider query exact bounded line; rejects Arc, seam/closed/wrong provider, nonmaterial, unsupported/ambiguous/unavailable source, not tessellated straight.
- **B1-F03:** Stage-qualified native line re-resolves deterministically and stays geometrically equivalent under reversed tangent sign and different physical endpoint representative. **Actual native topological Edge orientation reversal** needs an explicit Owner D2 production acceptance decision, not automatic A0 GAP forgiveness.
- **B1-F04:** Original feature edits re-evaluate; same-shape replacement producer does not rebind; later Features do not move acquired source stage; invalid cycle/forward stage refused.
- **B1-F05:** One new AxisId and one undoable Part transaction on Finish; hover, invalid pick, Cancel/Esc zero mutations and no leaked cursor; repeated Undo/Redo restores durable meaning.
- **B1-F06:** Edit/Replace Edge retains AxisId, visibility, dependency and one Undo; failed Edit leaves original authored intent.
- **B1-F07:** Delete retains dependent Revolve authored AxisId for honest Blocked/Missing, Undo restores; existing Origin and Sketch Line Revolve unchanged.
- **B1-F08:** New Edge Axis listed once in Reference Geometry; Sketch Axis once under its original Sketch; built-in Origin axes remain Origin; shared selection and Properties are correct.
- **B1-F09:** Individual/mixed Reference Geometry Show/Hide covers Datum and Edge Axis atomically without duplicating semantic visibility commands.
- **B1-F10:** Real viewer hover/one-Edge click and preview, subtype/error diagnostic, Finishing/Cancel; no last-good preview or global picking redesign, same Operations footer and focus rules as Datum.
- **B1-F11:** GUI and contextual CAD input use exactly the same authored Axis command and strict stage source; no free text XYZ and no `SKETCHLINE` constructor verb.
- **B1-F12:** Native Part cold Save/Close/Reopen and new OCCT provider reconstruct AxisId, source stage, current/effective line/visibility and dependent Revolve, or typed repairable unavailable state; verify supported previous schemas.
- **B1-F13:** All existing Q0/Q1/Q2/A0, PM-04, S4A/B/Datum, native Qt/OCCT tests retained; explicit FOCUSED → Draft FAST → Ready FULL Core+Kernel+Desktop and DOC-01/browser parity, plus Owner manual acceptance with actual GUI/CLI lifecycle. Separate Owner technical FINAL PASS and distinct guarded production PR merge permission required.

## 5. Mandatory unresolved D2 technical decisions BEFORE executable acceptance

1. **Type/schema:** Choose exact PartAxis source representation and backward-compatible storage grammar; decide whether next native schema version is needed and formal old-reader policy; **no implicit v19**. Existing `PartDocumentStore::current_schema_version = 18`.
2. **Native orientation risk:** A0 had no true native reversed B-Rep orientation case. Recommend a bounded exact-provider fixture as an acceptance prerequisite for shipping a durable unoriented Axis; Owner must explicitly accept this test requirement **or** explicitly defer it with a documented production risk. Mathematical +/− sign remains insufficient evidence.
3. **Axis-line/Revolve signed contract:** Clarify consumer-reversal semantics with an unoriented Axis without regression of existing oriented Sketch Line source or Revolve Reverse. No provider order selecting positive direction.
4. **One vertical slice vs headless first:** Resolve whether accepted executable package includes the reviewed UI lifecycle now or must first ship a smaller headless source/schema + native acceptance behind no visible entry, then separately authorize UI activation. Either way, **the complete approved UX lifecycle remains the delivery target**, not a source-only partial product presented as done.
5. **Fixed allowlist/tests:** Decide precise min viable code/test/docs paths from §3 (especially any required native provider integration currently absent from `resolveAxisReference`). Missing allowlisted path or architectural contradiction is an explicit STOP, not D0 permission.
6. **CLI details:** Keep existing `AXIS/SOURCE/FINISH/CANCEL` semantics; any new verb for a singleton `Edge` mode is optional and must be Owner-approved, not invented mid-coding.

This file is **a technical D2 question set with candidate scope**, not a ratified executable Work Contract. The next act after independent Owner technical D2 review is a bounded **accepted executable contract** with exact mutation allowlist, approved schema/API, FOCUSED/FAST/FULL and DOC-01 obligations, **separate governance squash**, **dedicated `work/ACTIVE.yaml` activation PR** and its own independent guarded merge. Neither Owner's UX signoff nor documentation PRs permit code changes before that.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: eventual production Axis Edge + persisted sources, lifecycle, dependent Revolve, UI/CLI and Tree placement change as-built system/product behavior. The present ONE `work/**` governance proposal does not change `docs/internal/**`, bilingual product documentation or generated Browser, and shall not claim shipping feature coverage.
