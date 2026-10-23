# SS2 R13-A2 — Native 3D Sketch relations and Driving labels
## Exact provider-neutral Viewer/binding D2 proposal (NOT ACCEPTED)

**Status:** DRAFT / OWNER D2 REVIEW REQUIRED — proposal only; no executable product permission  
**Date:** 2026-10-23  
**Foundation:** v1.0 FROZEN; Constitution and Architecture; ADR-0008, 0009, 0011, 0012–0014, 0016–0017  
**Program:** `work/ACTIVE.yaml` → R13-A2, Sketch Roadmap v1.11  
**Parent accepted contract:** `work/SKETCH_R13_A2_RELATIONS_DRIVING_DIMENSIONS_D2_WORK_CONTRACT_PROPOSAL.md` §§1–4, 6B  
**Read-only implementation audit:** PR #528 head `7874a47d7fca540fb5f6b694c05beda1924b00a0`; protected main `68fc4f33aaeee0e4741bc20b269b665839fa06cf`  
**Separate solver distribution proposal:** PR #529 — L-DLL feasibility selected, C ABI/path approval still pending  
**Do not merge this proposal or implement its new public interface without independent Owner D2 acceptance, exact-head governance CLOSURE and separate guarded-squash authority.**

## 0. Decision requested; why this is not a Qt paint patch

Owner-accepted A2 requires selectable native 3D relation glyphs and movable, directly editable numeric labels in the **existing** Part Sketch Edit viewport. Qt Operations rows or a screen-only fake diagram do **not** satisfy that goal. Current A2.0 exposes six relation controls and a DEV-only original PlaneGCS route, but `sketchAuthoredRelationsList` is intentionally read-only; no semantic Viewer annotation hit route exists. Driving schema v22 is **not** accepted and no dimension authoring can be claimed.

Propose **one narrow, renderer-neutral runtime annotation scene/pick mechanism**, carried by the current `viewer::IDocumentViewport` and bound back to `SketchId + ConstraintId` **only in application/UI state**. Ship no second Sketcher, no new persistent glyph identity, no OCCT handle/ordinal identity, no solver or Part ownership inside Viewer. Use two explicitly phased consumers:

- **E1 / R13-A2.0:** render and exactly pick the six authored A1 relation glyphs from the *effective current-stage* Sketch. Selection is transient. Actual Delete/freeze remains **disabled** until its separate accepted transaction D2 is satisfied.
- **E2 / R13-A2.2 (future, NOT unlocked by E1):** render selectable Driving Length/Radius/Diameter technical labels, arrows and witnesses; edit numeric text **on the visible label**, drag only authored relative placement. Requires separately accepted strict v22 and numeric solver/command gates first. E1 acceptance does not itself activate E2.

This proposal freezes the **boundary**, not product implementation or release packaging. Its runtime semantics must be reviewed against actual Qt/OCCT mouse event flow before Owner acceptance.

## 1. Source-backed architecture baseline

Already present on #528:

- `viewer::IDocumentViewport` has `setSketchScene`, `setSketchGripScene`, `setSketchInteractionPresentation`, `querySketchGrip`, `querySketchPresentation`, `setSketchMeasureCueScene`, `setSpatialPointerHandler` and `projectWorldPoint`. Its lower-level API has no Sketch-domain `ConstraintId`.
- `viewer::SketchScene` and `SketchGripScene` use **runtime** `PresentationToken`, validated scenes and neutral geometry.
- `ui::PartViewportController` already maps `PresentationToken ↔ SketchEntityAddress`, maintains an exact-revision `CurrentSketchModelSnapshot`, and projects `sketch::Point2` with resolved Part Sketch support into real world/viewport coordinates. This is the natural ownership boundary.
- `ui::PartSketchInteractionController` owns one active semantic tool, source-role acquisition, Part incarnation/revision safeguards and a guarded DEV relation command. The native UI currently has no annotation selection or direct label editor.
- `sketch::SketchConstraintState` already owns `ConstraintId`, kind and stable entity/element references; it does **not** persist screen coordinates.

Any implementation has to use the **current effective Sketch**, not raw authored seeds when solving/projection changes current geometry. The source stage, evaluated geometry and annotation scene must come from the same valid snapshot.

## 2. Proposed public presentation shape — bounded D2

**P1.** Add the following narrowly typed **renderer-neutral presentation** records to the existing `viewer/sketch_presentation.hpp`, and only the corresponding methods to `viewer/document_viewport.hpp`. Exact C++ signatures and validation will be frozen by this decision before source mutation:

```text
SketchAnnotationToken      -- runtime PresentationToken-class key, never persisted
SketchAnnotationGeneration -- runtime, nonzero, monotonically advanced per scene
SketchConstraintGlyph      -- {token, neutral glyph kind, anchor 3D,
                               optional direction/orientation, selected/hovered}
SketchConstraintGlyphScene -- {generation, vector<SketchConstraintGlyph>}
SketchDimensionAnnotation  -- {token, neutral linear/radial appearance,
                               world anchors/witnesses, formatted text,
                               world-space label anchor, selected/hovered}
SketchDimensionAnnotationScene -- {generation, vector<SketchDimensionAnnotation>}
SketchAnnotationPickResult -- {completed/status: miss|single|ambiguous|invalid,
                               generation, optional runtime token, hit role}
IDocumentViewport::setSketchConstraintGlyphScene(scene) -> bool
IDocumentViewport::setSketchDimensionAnnotationScene(scene) -> bool
IDocumentViewport::querySketchAnnotation(viewport point, hit policy) -> result
```

The records carry enough explicit units/frame meaning for a single renderer to compute glyphs, dimensions and exact hit bounds. `valid()` rejects invalid/duplicate tokens, invalid/nonfinite world geometry, empty dimension text, unsupported kind/role combinations and inconsistent scene generation. Empty valid scenes **clear** native annotation and pick state. Public Viewer records must **not** require inclusion of `sketch::ConstraintId`, `sketch::SketchModel`, Qt or OCCT headers.

**P2 — explicit §2.4 interpretation requiring Owner agreement.** Durable `ConstraintId` remains attached to each authored annotation in the **application-side scene builder/binding record**, paired with its allocated runtime presentation token; only the runtime token/visuals cross the lower Viewer API. This retains the accepted contract's "semantic ID ↔ scene record" intent while preventing a domain dependency in the shared Viewer. An alternative that copies `ConstraintId`/Sketch-domain types into the Viewer API is **rejected unless separately Owner-authorized**. The renderer cannot infer a semantic ID from a pixel position or glyph kind.

**P3.** A result of `querySketchAnnotation` has **no authoring authority**. The exact runtime token must resolve through the current application binding for the same scene generation and same:
```text
DocumentId + Part incarnationToken + DocumentRevision
+ SketchId + evaluated support/stage context + annotation scene generation
+ ConstraintId + annotation role
```
Re-check current Part source references/constraint identity **at command execution**. Reject stale session, stale scene, missing/ambiguous owner, foreign Sketch, missing/blocked stage, invisible/disabled annotation, invalid input or scene replacement with **no Part change**; never "nearest" rebind. A token from the previous camera redraw, session, Undo/Redo, Save/Reopen or different Sketch must not authorize a new mutation. Camera pan/zoom may change only disposable projection; if pick geometry is republished, generation must change deterministically and old results be rejected.

**P4.** `PartViewportController` provides the one binding table; `PartSketchInteractionController` owns selected/hovered semantic constraint state and exact tool routing; `CadWorkbench` hosts the Qt label editor and Operations visibility switches; `QtOcctViewerWidget` implements draw and hit mechanics only. Native glyphs must be visible on real geometry in the actual 3D viewport; **no Operations-only substitute**. The renderer must not create/delete constraints, mutate dimension values, or write persistence.

## 3. Interaction arbitration and exact hit behavior

1. Annotation picking is enabled only in a valid Part Sketch Edit context. **Select** can select a visible annotation by exact 3D-viewport hit. During active Relations source capture, relation Line/endpoint acquisition keeps ownership; an overlapping glyph must **not** steal its hit or convert into an inferred constraint. Existing navigation gestures always remain navigation.
2. Use Qt logical-pixel hit coordinates converted consistently using device pixel ratio; no geometric/world-space epsilon becomes pick authority. Query must explicitly distinguish **none**, **one**, **multiple**, **query failure**. Two overlapping glyphs/labels at the same location are **Ambiguous** and produce **no selected semantic owner**, not "topmost/first" implicit authoring. Any cycle/disambiguation feature needs a later explicit UI contract.
3. A selected native annotation and source Sketch entity are distinct transient selections. Selection feedback must be visibly attributable to one `ConstraintId`. One click may never both select a constraint glyph and delete an underlying Line. Read-only inspection and visibility toggles do not dirty Part, add Undo or change `ConstraintId`.
4. For the eventual E2 label: double-click **actual visible numeric text** opens a Qt inline editor anchored to current rendered label bounds; the editor receives text/Enter/Esc/Backspace focus and must not forward those keys to Sketch Delete. **Enter** invokes a new guarded semantic Part command; failed parsing/solve retains editable text and changes no Part state. **Esc** cancels editor only, no commit. Focus loss alone never commits.
5. For E2 dragging: explicit label hit starts a draft; camera movements and source-geometry drag are excluded; commit only when a valid current-revision semantic placement command runs. Persist relative Sketch U/V placement under approved v22, **not** screen positions. Release after camera change, stale stage or overlapping target cancels without an Undo entry.
6. `Delete` on a *selected annotation* must not reuse the generic Sketch-entity deletion route. It remains disabled for E1 until the already identified separate **Delete/Freeze all Relations D2** is integrated. Once approved, it must snapshot last accepted effective geometry and delete exactly one authored constraint under one guarded transaction, revalidate survivors, and retain dependent Part/Projection semantics. Never implement a cosmetic delete of only a glyph.

## 4. Fail-closed lifecycle invariants

- Switching Part, Sketch, tool, support stage, document incarnation, Undo/Redo, redo, closing, failed evaluation or recompute invalidates annotation bindings and clears scenes/selection/inline editor; no stale painted overlay may falsely represent current Body/Sketch truth.
- If a current-stage solver or projected-source resolution fails, preserve authored IDs/constraints in Part, show structured blocked/error status; do **not** draw old solved geometry as current or let it be picked. Unaffected valid annotations may display only when their owners/current geometry remain independently provable.
- Old/ambiguous glyph hits, token replay, stale callbacks, reentrant Qt events and ABA reopen must fail closed before executing any command, even if `DocumentRevision` and `ConstraintId` numerically match.
- Failed scene publication, incomplete hit query, hidden annotation or lost current stage leaves **zero** Part revisions, zero Undo entries, zero ID allocations and zero current-scene selection.
- The feature works with the real `QtOcctViewerWidget` and software/HiDPI scaling in 3D orthographic, perspective/isometric and pan/zoom. Viewer implementation detail remains private. No second renderer/editor stack and no SS1 SketchSession import.

## 5. Conditional exact implementation paths, E1 only

All are **existing exact paths already named** in parent contract §4, but this proposal by itself **grants no permission to edit them**. First implementation should be restricted to paths actually needed, with the rest left unchanged.

```text
src/viewer/include/simplesolid2/viewer/sketch_presentation.hpp
src/viewer/include/simplesolid2/viewer/document_viewport.hpp
src/viewer_qt_occt/include/simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp
src/viewer_qt_occt/qt_occt_viewer_widget.cpp
src/ui/part_viewport_controller.hpp
src/ui/part_viewport_controller.cpp
src/ui/part_sketch_interaction_controller.hpp
src/ui/part_sketch_interaction_controller.cpp
src/ui/cad_workbench.hpp
src/ui/cad_workbench.cpp
tests/sk03a_viewer_native_input_test.cpp
tests/sk03a_part_viewport_controller_test.cpp
tests/sk04c_part_sketch_interaction_controller_test.cpp
tests/cad_workbench_test.cpp
```

No new source/test path, generated HTML edit, new dependency, solver/backend, store/schema, `tests/CMakeLists.txt`, workflow, Project model or platform API beyond the **two existing Viewer headers** proposed above. If an actual renderer mechanism needs unlisted paths, STOP and request an exact Owner D2 amendment first. E2 additionally requires the **independently frozen v22 schema and numeric command**, not merely E1 passing. Any Draw/Projection/Assembly reuse is excluded.

## 6. Mandatory evidence (real RED -> GREEN, not screenshot theater)

**E1 gate, before merging even a partial UI feature:**

- Registered Windows native real-`QtOcctViewerWidget` test RED for missing glyph, exact hit, ambiguity and reentrant/stale scene generation. Native provider must produce real 3D objects/hits; a stub-scene-only pass is insufficient.
- Six relation glyph types rendered against their **actual resolved Line/endpoint** in XY/XZ/YZ and supported arbitrary planar Body Face Sketch frames, with responsive 3D pan/zoom/isometric/HiDPI; no use of raw authored seed geometry when the effective stage differs.
- Same-`ConstraintId` repeated scene projection, selection and read-only inspection; false picked owners = **0**; hidden/ambiguous/blocked/stale hits fail closed; distinct `ConstraintId` glyphs co-located stay ambiguous.
- All native input precedence paths: Select vs relation target, current grip, navigation, Esc and deletion of source entity. No unintended Part revision/Undo/solver calls. Reentrant callback and same-Sketch ABA tests.
- Runtime DEV solver authoring must still use the existing exact-stage guarded command; E1 does not assume shipped/distributable PlaneGCS or mark A2 complete.

**E2 gate, separately after v22/solver D2:** actual label geometry and select/double-click/edit/Enter/Esc/drag/overlap at 100/125/150/200% DPI; current-value no-dialog creation; invalid-value no authoring; 1-step Undo, deterministic Redo; Save/Close/**new-process** Reopen with a distinct provider; underlying numeric value unchanged by label dragging; no stale/foreign annotation acceptance; relevant Profile/Extrude/Revolve failed-stage repair.

Both gates require exact-head Windows FOCUSED → FAST → Ready FULL (Core + kernel-native + Desktop, with **no removed/weakened tests**), DOC-01 internal + canonical PL/EN + regenerated Product Browser and verifier, real Owner Windows Manual Functional+Visual PASS, independent Technical FINAL, and **separate** guarded-squash merge authorization. PR #528 FAST 94/94 on `7874a47d` and older FULL on `dc24b03` are historical current-slice evidence, not E1/E2 PASS. Preview=ON with real PlaneGCS 0.8.0 on Windows is still **unproven**, and a green preview=OFF FAST cannot replace it.

## 7. Decision options and hard STOP

**Recommended Owner D2:** accept **P1–P4 and E1 only** as the exact finite provider-neutral glyph scene, query, application binding and hit precedence, conditional on the 14 existing paths in §5, with no release/merge permission. Reserve E2 implementation until strict v22, solver/package and Delete/Freeze gates receive independent D2 acceptance. Owner may request a concrete signature amendment before acceptance.

**Rejected alternatives:** (a) Qt list icons instead of real Viewer glyphs; (b) pixel/name/geometric similarity as persistent identity; (c) import SS1 standalone Sketch canvas; (d) generic catch-all annotation engine for future Drawing; (e) fake glyphs that cannot be selected or whose selection bypasses current Part guard.

**STOP now:** no D2 acceptance yet; no E1 public API or source changes, no E2 production work, no v22, no provider/CI new paths, no DLL distribution, no Owner/manual/merge claim. Do not treat this proposal's PR creation, green governance CLOSURE or a generic `kontynuuj` as approval of its public API.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This change is a governance-only proposal with no shipped functionality. Any later accepted E1/E2 production implementation must update the internal as-built and paired PL/EN product documentation as declared in the parent active R13-A2 Work Contract, then regenerate the Product Browser using `./ss2.ps1 docs`.
