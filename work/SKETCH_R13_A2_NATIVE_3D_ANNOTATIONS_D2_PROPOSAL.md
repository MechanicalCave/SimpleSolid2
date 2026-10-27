# SS2 R13-A2 — Native 3D Sketch relations and Driving labels
## Exact provider-neutral Viewer/binding D2 proposal (E1 BOUNDARIES ACCEPTED CONDITIONALLY)

**Status:** OWNER D2 ACCEPTED E0 (2026-10-24), E1 P1–P4 BOUNDARIES (2026-10-27), CONDITIONAL EXECUTION; E2 NOT accepted; no merge/release authority  
**Date:** 2026-10-23  
**Foundation:** v1.0 FROZEN; Constitution and Architecture; ADR-0008, 0009, 0011, 0012–0014, 0016–0017  
**Program:** `work/ACTIVE.yaml` → R13-A2, Sketch Roadmap v1.11  
**Parent accepted contract:** `work/SKETCH_R13_A2_RELATIONS_DRIVING_DIMENSIONS_D2_WORK_CONTRACT_PROPOSAL.md` §§1–4, 6B  
**Read-only implementation audit:** PR #528 head `7874a47d7fca540fb5f6b694c05beda1924b00a0`; protected main `68fc4f33aaeee0e4741bc20b269b665839fa06cf`  
**Separate solver distribution proposal:** PR #529 — L-DLL feasibility selected, C ABI/path approval still pending  
**E1 Owner D2 scope is recorded below. Exact-header API/signature review, prerequisite E0 native proof and governance CLOSURE remain gates for E1 implementation. This Draft PR must not merge without separate, explicit Owner guarded-squash authorization.**

## Owner's separate E1 P1–P4 D2 disposition — 2026-10-27

**Recorded Owner response:** `akceptuje - kontynuuj`, responding directly to a request to approve **E1 P1–P4 from PR #530** for the first *native, selectable glyphs of six Sketch relation kinds*, explicitly excluding Delete/Freeze, Driving Dimensions, strict v22, production PlaneGCS distribution and product merge authority.

**Accepted bounded design:** provider-neutral glyph scene and hit result, application-owned `SketchId + ConstraintId` binding to transient presentation tokens, revision/incarnation/stage/scene-generation checks, native 3D OCCT presentation and unambiguous semantic hit precedence as described in §§2–5. Implementation is conditional on closing the independent E0 current-stage native test gate, reviewing and freezing concrete C++ public signatures and hit contracts against the actual native interface, using only the exact needed paths from §5, registered Windows RED→GREEN and the parent contract's DOC-01 and Owner proof. This decision is **not** blanket permission to add labels/dimensions, renderer infrastructure outside §5, new dependencies or CI changes.

**Governance disposition:** This records design authority, not product acceptance, independent Technical FINAL, legal release, or guarded merge. PR #530 stays Draft pending its **new exact-HEAD governance CLOSURE**, review and separate guarded-squash permission. Historical 2026-10-24 E0-only language below describes the earlier decision and does not supersede this 2026-10-27 limited E1 acceptance.

## Owner's explicit E0-only decision — 2026-10-24

**Owner response:** `akceptuje - kontynuuj`, given in direct reply to the explicit question: accept **E0 only from this PR #530** — reuse existing validated Part numerical stage gateway as one current/revision/incarnation-bound geometry source for Viewer, semantic picking and Profile/Kernel; **without authorizing E1/E2, v22 or PlaneGCS distribution**.

**Approved E0 direction, bounded execution:** retain authored geometry seeds and semantic ConstraintId unchanged; derive only disposable, provenance-checked current-stage solved geometry from `part::evaluateStageLocalSolve` and existing Part Profile/Feature evaluator. The UI may borrow the already explicitly gated DEV solver for source-backed experiments but cannot introduce an independently trusted Qt solver, a new public Viewer glyph/annotation interface, arbitrary source-stage identity, a persistence/schema change, a vendor artifact, or release packaging. Fail closed on invalid solution and clear stale scene/selection; prove meaningful RED→GREEN with real Windows registered tests. E0 implementation remains subject to exact active contract paths, independent technical and real-native evidence, documentation impact when shipped, and **separate guarded product/governance merge authorization**.

**At the time of this 2026-10-24 E0-only disposition**, E1 P1–P4 were not accepted; the independent 2026-10-27 Owner decision above now conditionally accepts only E1 relation glyph boundaries, **not** E2 numeric labels, strict v22, LGPL binary distribution or Delete/Freeze. Neither decision grants guarded merge permission.

## 0. Decision requested; why this is not a Qt paint patch

Owner-accepted A2 requires selectable native 3D relation glyphs and movable, directly editable numeric labels in the **existing** Part Sketch Edit viewport. Qt Operations rows or a screen-only fake diagram do **not** satisfy that goal. Current A2.0 exposes six relation controls and a DEV-only original PlaneGCS route, but `sketchAuthoredRelationsList` is intentionally read-only; no semantic Viewer annotation hit route exists. Driving schema v22 is **not** accepted and no dimension authoring can be claimed.

Propose **one narrow, renderer-neutral runtime annotation scene/pick mechanism**, carried by the current `viewer::IDocumentViewport` and bound back to `SketchId + ConstraintId` **only in application/UI state**. Ship no second Sketcher, no new persistent glyph identity, no OCCT handle/ordinal identity, no solver or Part ownership inside Viewer. Use two explicitly phased consumers:

- **E1 / R13-A2.0:** render and exactly pick the six authored A1 relation glyphs from the *effective current-stage* Sketch. Selection is transient. Actual Delete/freeze remains **disabled** until its separate accepted transaction D2 is satisfied.
- **E2 / R13-A2.2 (future, NOT unlocked by E1):** render selectable Driving Length/Radius/Diameter technical labels, arrows and witnesses; edit numeric text **on the visible label**, drag only authored relative placement. Requires separately accepted strict v22 and numeric solver/command gates first. E1 acceptance does not itself activate E2.

**Hard prerequisite uncovered by exact #528 source audit:** an accepted `AddSolvedSketchRelationCommand` writes the *authored relation* but intentionally does **not** overwrite authored Line seeds with the solved candidate; `PartViewportController::effectiveSketchModelForPresentation` returns `hosted.model` unchanged for an unlinked Sketch. Thus the existing `wb01.cad_workbench` DEV-preview fixture asserts authored Horizontal/Undo but does **not** assert that the originally skew Line becomes horizontal in the rendered Scene or the Part Profile/Kernel. A selectable glyph alone could therefore decorate unsolved geometry, a product-level false success. **E0 effective-stage geometry coherence must precede E1 glyphs; see §1A and §6.** This was initially source-backed; the follow-on real Windows controller/SketchScene RED evidence is now captured below. That evidence does **not** test native OCCT rendering or original PlaneGCS preview=ON.

This proposal freezes the **boundary**, not product implementation or release packaging. Its runtime semantics must be reviewed against actual Qt/OCCT mouse event flow before Owner acceptance.

## 1. Source-backed architecture baseline

Already present on #528:

- `viewer::IDocumentViewport` has `setSketchScene`, `setSketchGripScene`, `setSketchInteractionPresentation`, `querySketchGrip`, `querySketchPresentation`, `setSketchMeasureCueScene`, `setSpatialPointerHandler` and `projectWorldPoint`. Its lower-level API has no Sketch-domain `ConstraintId`.
- `viewer::SketchScene` and `SketchGripScene` use **runtime** `PresentationToken`, validated scenes and neutral geometry.
- `ui::PartViewportController` already maps `PresentationToken ↔ SketchEntityAddress`, maintains an exact-revision `CurrentSketchModelSnapshot`, and projects `sketch::Point2` with resolved Part Sketch support into real world/viewport coordinates. This is the natural ownership boundary.
- `ui::PartSketchInteractionController` owns one active semantic tool, source-role acquisition, Part incarnation/revision safeguards and a guarded DEV relation command. The native UI currently has no annotation selection or direct label editor.
- `sketch::SketchConstraintState` already owns `ConstraintId`, kind and stable entity/element references; it does **not** persist screen coordinates.

Any implementation has to use the **current effective Sketch**, not raw authored seeds when solving/projection changes current geometry. The source stage, evaluated geometry and annotation scene must come from the same valid snapshot.

## 1A. New E0 prerequisite — one effective Sketch result for Viewer, Profile and Kernel (D2 STOP)

**Observed code path, exact #528 at `7874a47d`:**

1. `src/application/document_session.cpp`, `DocumentSession::execute(AddSolvedSketchRelationCommand)`: `preflightLocalRelation` returns an independently validated candidate, but `SketchModel::restore(*preflight.proposed_authored)` commits only authored seeds + relation. The derived candidate intentionally remains ephemeral; that preserves ADR-0008/0009 and is **correct**.
2. `src/ui/part_viewport_controller.cpp`, `currentSketchInteractionModel()` and `effectiveSketchModelForPresentation()`: for a Sketch with **zero PG-01 projection bindings**, both currently return original `hosted.model` (the authored seeds), not a solver-evaluated, stage-authorized Sketch. `buildSketchScene()` uses that model; its snapshot is revision-checked but not a local-solver projection.
3. **Existing Part-side solver gateway is NOT missing:** `src/part/include/simplesolid2/part/effective_sketch_projection.hpp` already declares `prepareStageLocalSolveProblem`, `evaluateStageLocalSolve`, `resolveStageSolvedProfileKernelInput` and `evaluatePartWithLocalSolve`. In `src/part/profile_kernel_input.cpp`, unconstrained legacy Profile evaluation is unchanged, **constrained input fails closed as `unresolved_profile` unless a validated stage-solved candidate arrives**, and `resolveStageSolvedProfileKernelInput` independently rechecks the exact Part evaluation lease. The gap is **wiring a single coherent stage-evaluation authority into native Viewport/runtime and downstream application consumption**, not building a second solver or bypassing Part. Reuse this existing gateway and its typed rejection semantics; any required new method/signature must be independently Owner-reviewed.
4. `tests/cad_workbench_test.cpp` native DEV-preview conditional adds Line `(0,1)→(5,2)`, creates Horizontal and checks `ConstraintId`, Part revision, status, Undo/Redo. It lacks an assertion on **rendered geometry becoming horizontal** and lacks a Profile/Extrude consumer. Existing preview=OFF FAST 94/94 and older manual FULL therefore do not close E0.

**Actual Windows E0 witness (new, still RED; no production E0 fix):** The registered `sk04c.part_sketch_interaction_controller` on separate test-only Draft [#531](https://github.com/MechanicalCave/SimpleSolid2/pull/531) was run against product #528 as a deterministic test-double solver and `IDocumentViewport` scene-capture implementation. The original RED on [Windows FAST #40067012907](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/40067012907), head `f8f9cc4f4bfc5ec67eb8bb024f6b80b1d11e1960`, was 93/94 PASS, failure only at `near(shown.start.y, shown.end.y)`. **Strengthened exact-head RED** on [Windows FAST #40084207088](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/40084207088), test SHA `cca06a8e2e99f66a8fb348387373e937ffc4b677`, MSVC build PASS and **93/94 PASS**, fails at line **4568** on the **same actual published `SketchScene` horizontal-geometry assertion**, and proves first that the same document/revision/incarnation, with `part::evaluateStageLocalSolve`, yields a `candidate_accepted` Horizontal line with stable `EntityId` while authored seed remains skew. The VIEWER bridge alone does not consume that proven candidate. The standalone Part stage-solver oracle PASSED before the expected Viewer RED. No material is merged; #528 remains unchanged. These are deterministic Qt/controller + neutral-scene tests, **NOT original PlaneGCS on Windows and NOT a live `QtOcctViewerWidget` / actual OCCT glyph or Profile→Extrude equivalence test**. All of those stronger product/native gates still apply.

**Required E0 decision, not yet accepted or implemented:** **reuse the existing Part `evaluateStageLocalSolve` / `resolveStageSolvedProfileKernelInput` / `evaluatePartWithLocalSolve` evaluation authority**, rather than invent a competing solving layer. A **single domain/application-owned, stage/revision/incarnation-bound disposable evaluated Sketch** must feed (a) native Sketch scene/pick and future 3D glyph anchors, **and** (b) Part Profile/Kernel input where applicable. The same source constraints and one validated solver contract must determine the effective geometry, without materializing it into persistent authored seeds. An R13 constraint may be authored while no released solver is available, but the renderer must not present seed geometry as a successfully solved current state; downstream Profile/Body must be explicitly Unavailable/Blocked on unsolved/unsupported stages as appropriate. A prior last-good result is not current truth. No UI-only cache may override Part's effective stage.

**Minimum E0 evaluation boundary** (exact function names/types remain Owner D2):
```text
Part current DocumentId / incarnation / revision / Sketch support & producing Body stage
+ authored Sketch (seeds, ConstraintIds, semantic source refs)
+ current evaluated PG-01 linked-source inputs (when supported)
+ explicitly accepted solver + numerical/DoF/conflict policy
    -> accepted disposable EffectiveSketchModel, with provenance lease
    OR typed Failed / Missing / Ambiguous / Unsupported / Blocked
    -> ONE coherent source for Viewer geometry, semantic picking,
       ProfileRegionIntent resolution and native Kernel consumer
```
The backend yields a candidate, but **Part's existing revision-bound evaluation authority** validates and publishes the result; Viewer renders it but never decides model meaning. Do not silently re-solve in Qt with different constraints/policy than Kernel or use an old disposable result after a new DocumentRevision. E0 may be validated locally with the original opt-in PlaneGCS on the existing DEV branch; it **does not** grant release authority until the separate PR #529 DLL/installed-consumer/LGPL gate.

**E0 STOP conditions:** failure of unique current stage/support, missing source, backend missing or rejected, nonfinite/residual/DoF mismatch, stale Part incarnation/revision, difference between native displayed geometry and consumed Profile/Kernel effective model, or any required new public API/track path outside the accepted parent contract. Before E0 D2 acceptance, do not add a cosmetic horizontal icon over the still-oblique source Line. This E0 gate is a **request to extend/clarify technical authority**, not permission for code or schema changes.

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

## 2A. E1-only concrete public C++ signatures for independent D2 review (2026-10-27)

**Status: EXACT SIGNATURE CANDIDATE — NOT YET OWNER-FROZEN.** The Owner accepted E1 P1–P4 boundaries on 2026-10-27, **not** these newly derived method signatures, OCCT hit radius or acceptance of missing E0 original-provider evidence. Do not write public product headers based solely on this section. This packet resolves otherwise underspecified native source details into reviewable choices.

**Finite first tranche (only E1, no dormant E2 API):**
- Only `src/viewer/include/simplesolid2/viewer/sketch_presentation.hpp` and `src/viewer/include/simplesolid2/viewer/document_viewport.hpp` would receive new public types/methods after exact-signature acceptance. `SketchDimensionAnnotationScene`, inline editing and `setSketchDimensionAnnotationScene` are *deferred*, not stubbed in E1, even though §2 previews their future shape.
- Renderer-neutral `SketchAnnotationToken` is distinct from current `PresentationToken`, whose tokens represent actual Sketch entities. Neither token is persistent. A glyph has one Part-authored `ConstraintId` **only in UI/application binding**, never in the Viewer type, native AIS object or file.
- A glyph stores its resolved world anchor and owning current Sketch support basis. The view may render a compact non-zoomable native OCCT glyph at that world anchor; projection and native hit testing use current Qt logical pixels and DPR.

Candidate declarations (names and field layout to be explicitly frozen before writing headers):

```cpp
// sketch_presentation.hpp -- E1 ONLY, proposed
struct SketchAnnotationToken final {
    std::uint64_t value{};
    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }
    friend constexpr bool operator==(
        const SketchAnnotationToken&,
        const SketchAnnotationToken&) = default;
};

struct SketchAnnotationGeneration final {
    std::uint64_t value{};
    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }
    friend constexpr bool operator==(
        const SketchAnnotationGeneration&,
        const SketchAnnotationGeneration&) = default;
};

enum class SketchConstraintGlyphKind : std::uint8_t {
    horizontal, vertical, coincident, parallel, perpendicular, fix
};

struct SketchConstraintGlyph final {
    SketchAnnotationToken token;
    SketchConstraintGlyphKind kind{SketchConstraintGlyphKind::horizontal};
    Point3 anchor;            // resolved current-stage world XYZ
    Vec3 sketch_u_axis;        // current support, world-space
    Vec3 sketch_v_axis;        // current support, world-space
    bool selected{};
    bool hovered{};
    [[nodiscard]] bool valid() const noexcept;
};

struct SketchConstraintGlyphScene final {
    SketchAnnotationGeneration generation;
    std::vector<SketchConstraintGlyph> glyphs;
    [[nodiscard]] bool valid() const noexcept;
};

enum class SketchAnnotationPickStatus : std::uint8_t {
    unavailable, miss, single, ambiguous
};
enum class SketchAnnotationHitRole : std::uint8_t {
    constraint_glyph  // dimension role not compiled in E1
};
struct SketchAnnotationPickResult final {
    SketchAnnotationPickStatus status{
        SketchAnnotationPickStatus::unavailable};
    SketchAnnotationGeneration generation;
    std::optional<SketchAnnotationToken> token;
    std::optional<SketchAnnotationHitRole> role;
    [[nodiscard]] bool valid() const noexcept;
};

// document_viewport.hpp -- E1 ONLY, proposed
virtual bool setSketchConstraintGlyphScene(
    const SketchConstraintGlyphScene& scene) {
    return scene.valid() && scene.glyphs.empty();
}
[[nodiscard]] virtual SketchAnnotationPickResult
querySketchAnnotation(ViewportPoint2 point) {
    (void)point;
    return {};
}
```

**Validation and transitions to be acceptance-tested:**
1. A **nonempty** glyph scene requires nonzero generation, valid unique tokens, one of exactly six kinds, finite XYZ, finite nonzero/nonparallel `sketch_u_axis` and `sketch_v_axis`. The Viewer must reject duplicates, unsupported enum casts and malformed data without publishing a half-installed scene. An empty scene with zero generation is an explicit **clear/revoke**, not a new selectable generation. A nonzero-generation empty scene is an installed empty snapshot.
2. A native glyph scene setter succeeds only after replacing its entire glyph set and selection/pick authority. Native renderer objects must be cleared after scene replacement, Sketch Edit exit or failed-current-stage evaluation. The default `IDocumentViewport` setter rejects a **nonempty** scene, so neutral/headless Viewers cannot falsely report rendered E1 glyphs.
3. `querySketchAnnotation` returns **unavailable** with no token/role when input is invalid, renderer not ready or native query failed; **miss** with the installed generation and no token/role on a valid query with zero hits; **single** with exact generation, one token and `constraint_glyph` role; **ambiguous** with the current generation and NO token/role if 2+ glyph hit regions overlap. Invalid/status-role combinations are not accepted as valid results. No nearest-wins selection on ambiguity.
4. Native glyph rendering may use existing `AIS_TextLabel` / 3D-point primitives with a screen-constant size and world anchor. Native hit regions must match the actual visual glyphs and use Qt-logical-position→physical-pixel DPR conversion for 100/125/150/200%; their exact radius/bounds and OCCT presentation are **subject to Windows RED fixture evidence before frozen implementation**. Native geometry is not a QPainter-only or Operations-row imitation.
5. UI allocates a **fresh scene generation and runtime tokens** for every newly published current-stage snapshot and revokes the previous mapping first. A hit resolves only through `(DocumentId, incarnation, revision, SketchId, support/stage, glyph generation, token) → ConstraintId`. Part authoring never trusts a hit token directly. Stale callbacks, overlapping glyphs, Undo/Redo, previous Sketch/Part, ABA/cold reopen and switching active relation target keep **zero** authorized semantic mutations.
6. E1 read-only selection/inspection does NOT enable Delete/Freeze or dimension editing and does not allocate Part IDs, dirty a document, call a solver on pointer movement, or add Undo. The existing sketch entity/grip acquisition always takes precedence while a relation target is active; view navigation keeps its existing handlers.

**Registered RED→GREEN plan; no test registration, CI or new source paths:**
- `tests/sk03a_viewer_native_input_test.cpp`: compile/new glyph interface only after accepted signature, real exposed OCCT window, absent/replaced/hidden glyph, exact hit and overlap=ambiguous, stale generation after native scene replace, world XYZ from XY/XZ/oblique support and HiDPI.
- `tests/sk03a_part_viewport_controller_test.cpp`: glyph anchors derived from the **same** accepted evaluated Sketch as SketchScene/Profile/Body; `SketchId/ConstraintId` bindings are generation-, stage-, revision- and incarnation-guarded, including reentrant Undo and failed solve.
- `tests/sk04c_part_sketch_interaction_controller_test.cpp` and `tests/cad_workbench_test.cpp`: Select-vs-tool hit precedence, read-only inspection, no Part changes, Esc, visibility toggles, switch/reopen and failed-stage clears.
- Preserve every existing test and independently verify exact-head Windows FOCUSED → FAST → FULL. This plan **does not itself authorize** a new E1 executable while E0 original PlaneGCS UI PREVIEW=ON remains unverified.

**Hard sequencing:** E0 genuine original PlaneGCS 0.8.0 DEV `SS2_R13_PLANEGCS_UI_PREVIEW=ON` in actual native Windows Workbench + native geometry/pick has not yet been evidenced by a passing registered exact-head combined lane; ordinary FULL tests with a deterministic double do not count. The existing CI lane `windows-planegcs-native-core` only runs for historical PR #506 and is not a native Workbench preview=ON fixture. Any changed workflow/test-registration path requires its own explicit D2 authorization. Freeze these exact signatures by independent Owner review before new public header/source mutations; governance CLOSURE and E0 proof do not themselves approve future implementation diff.

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

**E0 gate — required BEFORE E1 glyph implementation or claiming a functioning Relations UI:**

- Add registered **real Windows native original-PlaneGCS preview=ON** fixture for a nonhorizontal source Line `(0,1)→(5,2)`: after Horizontal, inspect the actual published `SketchScene` 3D world endpoints and require horizontal U/V geometry within the accepted evaluated tolerance, *not merely an authored ConstraintId*. The same exact effective model must be used by Sketch source selection; committed authored seed must remain unchanged. Verify scene before authoring and after Undo/Redo, camera republish, context switch and cold new-process replay where the installed backend is available.
- Cross-domain tests with a previously accepted Profile/Extrude consumer must use evaluated line geometry or **fail closed with typed current-state failure** when constraints cannot be evaluated. Never quietly build stale/unconstrained geometry. Prove source-stage equality among Viewer, Profile and Kernel, zero false-current bodies and no changed authored seed. Positive new-process replay/product claims are gated on the approved original production solver package; DEV-only evidence does not replace them.
- Preserve the already executed **controller/neutral-Scene** real Windows RED [#40084207088](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/40084207088) (93/94, stage gateway oracle GREEN then SketchScene RED); this does **not** replace the **still pending original-PlaneGCS preview=ON** and actual OCCT/provider/Kernel profile witnesses above. After Owner E0 D2 approval, minimal stage-coherent E0 implementation must pass all independently required D2/path and FOCUSED → FAST → FULL evidence. An ordinary preview=OFF green FAST is insufficient. **No E1 "complete" label without E0.**

**E1 gate, before merging even a partial UI feature:**

- Registered Windows native real-`QtOcctViewerWidget` test RED for missing glyph, exact hit, ambiguity and reentrant/stale scene generation. Native provider must produce real 3D objects/hits; a stub-scene-only pass is insufficient.
- Six relation glyph types rendered against their **actual resolved Line/endpoint** in XY/XZ/YZ and supported arbitrary planar Body Face Sketch frames, with responsive 3D pan/zoom/isometric/HiDPI; no use of raw authored seed geometry when the effective stage differs.
- Same-`ConstraintId` repeated scene projection, selection and read-only inspection; false picked owners = **0**; hidden/ambiguous/blocked/stale hits fail closed; distinct `ConstraintId` glyphs co-located stay ambiguous.
- All native input precedence paths: Select vs relation target, current grip, navigation, Esc and deletion of source entity. No unintended Part revision/Undo/solver calls. Reentrant callback and same-Sketch ABA tests.
- Runtime DEV solver authoring must still use the existing exact-stage guarded command; E1 does not assume shipped/distributable PlaneGCS or mark A2 complete.

**E2 gate, separately after v22/solver D2:** actual label geometry and select/double-click/edit/Enter/Esc/drag/overlap at 100/125/150/200% DPI; current-value no-dialog creation; invalid-value no authoring; 1-step Undo, deterministic Redo; Save/Close/**new-process** Reopen with a distinct provider; underlying numeric value unchanged by label dragging; no stale/foreign annotation acceptance; relevant Profile/Extrude/Revolve failed-stage repair.

Both gates require exact-head Windows FOCUSED → FAST → Ready FULL (Core + kernel-native + Desktop, with **no removed/weakened tests**), DOC-01 internal + canonical PL/EN + regenerated Product Browser and verifier, real Owner Windows Manual Functional+Visual PASS, independent Technical FINAL, and **separate** guarded-squash merge authorization. PR #528 FAST 94/94 on `7874a47d` and older FULL on `dc24b03` are historical current-slice evidence, not E1/E2 PASS. Preview=ON with real PlaneGCS 0.8.0 on Windows is still **unproven**, and a green preview=OFF FAST cannot replace it.

## 7. Decision options and hard STOP

**E0 was accepted independently; E1 P1–P4 boundaries are now conditionally Owner-accepted as recorded above.** Before source mutation, close the E0 native proof and freeze exact signatures/hit behavior under the accepted finite §5 subset; obtain current governance-only CLOSURE. This is not E2/v22 or release/merge permission. Reserve E2 implementation until strict v22, solver/package and Delete/Freeze gates receive independent D2 acceptance. Owner may request a concrete signature amendment before acceptance.

**Rejected alternatives:** (a) Qt list icons instead of real Viewer glyphs; (b) pixel/name/geometric similarity as persistent identity; (c) import SS1 standalone Sketch canvas; (d) generic catch-all annotation engine for future Drawing; (e) fake glyphs that cannot be selected or whose selection bypasses current Part guard.

**Current STOP:** E1 P1–P4 scope is Owner-accepted but cannot claim E0 closure or begin unreviewed public API implementation until the native E0 prerequisite and concrete signature gates pass. E2/v22 authoring, new provider/CI paths, Delete/Freeze and DLL distribution are NOT authorized. No Owner manual/release/merge claim. #530 remains Draft with independent governance CLOSURE and guarded merge still required.

## 8. First Owner/manual DEV UI smoke protocol (2026-10-27; pre-E1 glyph release)

**Status: proposed, not yet executed; *not* proof of installed original PlaneGCS / E1 manual acceptance.** This is the first manual functional + visual evaluation of the *existing* local DEV-ONLY Relations Workbench, **not** production/installer proof. Do not show the six E1 glyphs as implemented until the future native E1 renderer and exact signature gates have passed. The existing E0 suite has Windows FULL **203/203 PASS at `37cd1fec`** and new native Undo/Redo/lease **FOCUSED 1/1 PASS at `e5ef833d`**, both using a deterministic solver double in the native joined test; these are independent evidence.

### Environment and source integrity

Use a **separate clean Windows x64 / MSVC 2022 build directory**, with working Qt6+OpenCASCADE local environment (`./ss2.ps1 setup` must already have populated `.ss2-local/env.ps1`) and the **original source-backed PlaneGCS 0.8.0** already available offline. No download, vendor edit or distribution. `src/CMakeLists.txt` verifies the complete vendor source manifest SHA-256 and requires known Eigen3/Boost headers; if any input is missing, **STOP**, do not swap in a different solver or bypass verification.

Illustrative PowerShell build sequence for the **actual same Windows runner** `SS2-Windows`. Historical source integrity and native feasibility were demonstrated by original-source [Windows PR #506 run #39630814429](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39630814429) on **2026-10-21**: `r13a1.planegcs_native` and `r13a1.planegcs_occt_native` both **PASS (2/2)**. It reported the unmodified source tree at `D:\SimpleSolid2\build\b1a\source\planegcs-0.8.0`; its existing Boost and Eigen directories are shown below. The same named runner `SS2-Windows` also executed the ordinary native Workbench E0 [#40494267907](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/40494267907) on **2026-10-27** (default UI preview OFF).

**New actual M0 observation (2026-10-27, exact earlier test HEAD `a0e080c3`):** operator's clean Windows isolated local build really found original PlaneGCS source, Eigen, Boost, Qt/OCCT, verified original wrapper SHA-256, built `ss2_r13_planegcs_dev.lib` and `SimpleSolid2.exe` and confirmed `SS2_R13_PLANEGCS_DEV=ON`, `SS2_R13_PLANEGCS_UI_PREVIEW=ON`. The registered real native Workbench CTest ran **4.34 s and FAILED**: it printed `E0_ORIGINAL_PLANEGCS_NATIVE_WORKBENCH_PREVIEW_ON_PASS` (XY genuine Horizontal achieved) then `SK-03A native input failure: E0 original PlaneGCS XZ Horizontal world-Z AIS`. Thus **XZ RED** and **cold-reopen NOT REACHED**, so M0 stays RED. A following OCCT `wglMakeCurrent()` context error does not yet establish cause. A second local original-PlaneGCS source-backed rerun on PR #533 `d03ca09e` produced `E0_XZ_DIAG` confirming the authored XZ line has native original seed crossings V=8.2 and V=9.8, then a fresh accepted native Horizontal AIS at Vleft=8 and Vright=8, one object, new consistent token and no wrong-XY hit. The previous RED was caused by the **test oracle incorrectly requiring world-Z ≈9** even though underconstrained Horizontal requires only equal local V. There was no demonstrated XZ solve or coordinate-transform defect. PR #533 test-only `8a46624f` retains all required semantic, world-XZ pick and AIS invalidation gates, removes the unjustified absolute-Z bound, and requires Redo to recover the same original numerical stage. **The corrected actual original-on M0 run is still required and not yet PASS**, including XZ Undo/Redo and downstream Save/reopen; run updated pinned SHA above. Do not mislabel compiler or XY-only success as combined PASS.

These are **historical paths**, *not* a claim that the files still exist or that the native Workbench ON test has run. Check the paths on the actual runner **before** CMake, stop on any missing item, and allow `src/CMakeLists.txt` to reverify every consumed original source SHA-256. Use the **exact checked-out PR #533 HEAD** shown below (update the pinned SHA explicitly if the PR head changes). This clean, dedicated build launches the same binary after CTest without implicit reconfigure/build:

```powershell
. .\scripts\ss2-common.ps1
Import-SS2LocalEnvironment
$cmake = Get-SS2CMakeExe
# Exact PR #533 HEAD checked on 2026-10-27. If PR #533 moves,
# update this SHA only after verifying the new PR head independently.
$expectedE0Head = '8a46624fd39110d84f102b0a26c6a2bca2178fc0'
$actualE0Head = (& git rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actualE0Head -ne $expectedE0Head) {
    throw "Wrong E0 source HEAD: expected $expectedE0Head, got $actualE0Head"
}
$build = Join-Path (Get-Location).Path 'build\r13-a2-manual-dev'
# Previously verified by the same runner in PR #506 original-source
# Windows CTest; read-only existing inputs. Never silently download them.
$source = 'D:\SimpleSolid2\build\b1a\source\planegcs-0.8.0'
$boost = 'D:\SimpleSolid2\.simplesolid-env\vcpkg-installed\00c5775211f45cd08b37fce0484b4cb940e422ab-x64-windows\x64-windows\include'
$eigen = Join-Path $boost 'eigen3'
foreach ($requiredFile in @(
    (Join-Path $source 'pyproject.toml'),
    (Join-Path $source 'src\wrapper.h'),
    (Join-Path $source 'src\planegcs\GCS.cpp'),
    (Join-Path $eigen 'Eigen\Core'),
    (Join-Path $boost 'boost\graph\adjacency_list.hpp')
)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "M0 STOP: original source/dependency no longer on runner: $requiredFile"
    }
}
Write-Host "[M0] source: $source"
Write-Host "[M0] Eigen: $eigen"
Write-Host "[M0] Boost: $boost"
& $cmake -S . -B $build -G 'Visual Studio 17 2022' -A x64 `
    '-DSS2_BUILD_DESKTOP=ON' `
    '-DSS2_R13_PLANEGCS_DEV=ON' `
    '-DSS2_R13_PLANEGCS_UI_PREVIEW=ON' `
    "-DSS2_PLANEGCS_080_SOURCE_DIR=$source" `
    "-DSS2_PLANEGCS_EIGEN_INCLUDE_DIR=$eigen" `
    "-DSS2_PLANEGCS_BOOST_INCLUDE_DIR=$boost"
if ($LASTEXITCODE -ne 0) { throw 'DEV preview configure FAILED' }
& $cmake --build $build --config Debug --target sk03a_viewer_native_input_test simplesolid2
if ($LASTEXITCODE -ne 0) { throw 'DEV preview native test + app build FAILED' }
$ctest = Join-Path (Split-Path $cmake -Parent) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest -PathType Leaf)) {
    throw 'Expected matching ctest.exe next to configured cmake.exe'
}
# Check actual CMake configuration, not just a successful OFF-mode CTest.
$cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt')
foreach ($required in @(
    'SS2_R13_PLANEGCS_DEV:BOOL=ON',
    'SS2_R13_PLANEGCS_UI_PREVIEW:BOOL=ON'
)) {
    if ($cache -notcontains $required) {
        throw "Required original PlaneGCS preview CMake flag missing: $required"
    }
}
# -V includes the exact native fixture's own ON/OFF evidence marker.
$nativeLog = & $ctest --test-dir $build -C Debug --no-tests=error `
    -R '^sk03a[.]viewer_native_input$' -V 2>&1
$nativeExit = $LASTEXITCODE
$nativeLog | ForEach-Object { Write-Host "$_" }
if ($nativeExit -ne 0) {
    throw 'Original PlaneGCS Workbench + OCCT DEV native CTest FAILED'
}
$nativeProof = $nativeLog | Out-String
foreach ($positiveProof in @(
    'E0_ORIGINAL_PLANEGCS_NATIVE_WORKBENCH_PREVIEW_ON_PASS',
    'E0_ORIGINAL_PLANEGCS_XZ_NATIVE_WORKBENCH_PASS',
    'E0_ORIGINAL_PLANEGCS_NATIVE_WORKBENCH_COLD_REOPEN_PASS'
)) {
    if ($nativeProof -notmatch $positiveProof) {
        throw "Original PlaneGCS native Workbench missing ON-mode proof: $positiveProof"
    }
}
foreach ($wrongMode in @(
    'E0_NATIVE_WORKBENCH_PREVIEW_OFF_PASS',
    'E0_NATIVE_WORKBENCH_XZ_PREVIEW_OFF_PASS',
    'E0_NATIVE_WORKBENCH_COLD_REOPEN_PREVIEW_OFF_PASS'
)) {
    if ($nativeProof -match $wrongMode) {
        throw "Wrong preview-OFF native Workbench branch executed: $wrongMode"
    }
}
# Launch the SAME already-built binary, without ss2.ps1 run.
# ss2.ps1 run -> ss2-build.ps1 -> ss2-configure.ps1 would otherwise
# perform another CMake configure/build after we captured proof.
$exe = Join-Path $build 'src\Debug\SimpleSolid2.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Original PlaneGCS Workbench DEV binary missing: $exe"
}
Write-Host "[M0] Git HEAD: $(& git rev-parse HEAD)"
Write-Host "[M0] Direct DEV executable: $exe"
Write-Host "[M0] DEV executable SHA256: $((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash)"
& $exe
if ($LASTEXITCODE -ne 0) {
    throw "Original PlaneGCS DEV Workbench exited abnormally: $LASTEXITCODE"
}
```

**Registered native Workbench gate (PR #533 `9a6327d9`):** `sk03a.viewer_native_input` now instantiates an actual `CadWorkbench` with exposed `QtOcctViewerWidget`, enters XY Sketch Edit, verifies a skew Line in native AIS/picking, emits a real native Qt mouse click and uses the Relations → Horizontal buttons. The ordinary CI preview-OFF path asserts no false authored Constraint/Part/Undo changes. The separately configured **preview-ON path requires source-verified original PlaneGCS**, successful authoring, original seed preservation, Workbench feedback and native AIS. Run the CTest above on the **exact PR #533 branch SHA** (XZ authoring check introduced at test-only HEAD `a0e080c3`) with both explicit flags ON, and preserve full CMakeCache and CTest logs; OFF green is not ON proof. The Owner visual/manual test must additionally inspect rendered horizontal endpoints, Undo/Redo, camera/picking, and fresh process/reopen. The same registered fixture also operates the real Workbench **Save** control with a valid native checkpoint, loads a new `.ss2part` Part incarnation through `PartDocumentStore`, deactivates the former session, re-enters Sketch Edit on the same actual OCCT window and verifies a fresh AIS/pick token and current effective stage. The M0 script above requires **all three** source-backed positive markers: XY Horizontal, XZ Horizontal in actual world-Z native AIS (including Qt Undo/Redo) and same-Workbench native `.ss2part` cold reopen. It rejects all three corresponding preview-OFF markers. The XZ check is a genuine native mouse-pick/Qt Relations operation and requires the solver to produce horizontal world-XZ geometry, not merely text saying the relation was accepted. This CTest still runs in one process; independent cold process plus visually observed behavior remains an Owner manual obligation.

**Reproducibility safeguard:** `ss2.ps1 run` is intentionally *not* used in M0 because it calls `scripts/ss2-run.ps1`, which first invokes `ss2-build.ps1` and thus `ss2-configure.ps1` again. While existing CMake cache options normally persist, a fresh configure/build after the positive CTest changes the evidence boundary. Launching `$build/src/Debug/SimpleSolid2.exe` directly under the already imported `Import-SS2LocalEnvironment` environment preserves the verified source-configured build. Capture the SHA-256 of this exact binary and the corresponding CMakeCache; if the binary cannot be found, **STOP** instead of falling back to a default build. The console log and a video/screenshot still need separate provenance tied to the same SHA.

**Verification scope:** GitHub Actions' existing `windows-planegcs-native-core` job is explicitly limited to PR #506 and tests `SS2_BUILD_DESKTOP=OFF`; the ordinary PR #533 `windows-msvc-focused` job builds `SS2_R13_PLANEGCS_UI_PREVIEW=OFF`. The evidence from these two jobs establishes *runner feasibility only*, not an executed combined UI-preview-on run. There is no supported remote runner shell or workflow-dispatch action exposed by the currently connected GitHub integration. The above M0 invocation therefore requires an authorized operator on `SS2-Windows` (or separately approved GitHub workflow expansion with governance amendment) and a screen recording of the real application. Do not change the protected workflow or broaden product code as a workaround.

No release packaging or binary export is part of this procedure. On an unfamiliar machine, first verify toolchain, OCCT runtime DLL availability and graphical native Viewer exposure. A `SS2_R13_PLANEGCS_UI_PREVIEW=OFF` build is **not** evidence for this protocol.

### M0 manual pass/fail script — actual Workbench + real OCCT

1. Start the above DEV application, create a Part, enter an **XY Sketch Edit** and draw one noticeably sloped Line; confirm it is visible and selectable in the *native* 3D Viewer, not merely a Qt Operations row.
2. Choose Relations and author **Horizontal** by the supported genuine Sketch spatial picking route. Confirm that the original source is visibly replaced by a horizontal **current effective line** in the OCCT viewport, with one accepted relation listed in Operations; if no solver or current stage is available, the UI must fail closed rather than announce a false accepted/current solution.
3. Test **Undo**: the relation disappears and the originally sloped line is again visible/selectable at its correct world position. Test **Redo**: the same authored relation reappears, while the native Viewer shows only the newly evaluated horizontal line; an old displayed AIS or stale pick is a FAIL.
4. Orbit/pan/zoom, repeat native hits on the actual current visible geometry, leave Sketch Edit and re-enter. Native picks must map to current Sketch owner, not the previous displayed coordinates. Capture original expected XY before/after and native screen recording.
5. **Save/Close/Reopen** in a new document session, re-enter Sketch Edit and verify durable authored Line/Constraint semantics plus a fresh effective numerical render. Stop and record a blocker if original backend cannot be reattached or the current-stage provenance is unavailable; do not mislabel an old renderer image as success.
6. Record exact source branch SHA, full CMakeCache DEV flags, external original 0.8.0 manifest verification, Windows MSVC/Qt/OCCT environment, CTest logs if applicable, screenshot/video of the *real* Viewer and every PASS/FAIL separately. A partial/manual fail is RED; do not downgrade the acceptance rule.

**Explicit limits:** M0 is an *engineering DEV-original-solver* gate and does not prove production LGPL installed-consumer packaging. It has **no selectable native 3D E1 glyph yet** and no Dimensions/v22. M1 glyph Manual Functional+Visual PASS must follow the separately frozen E1 public signatures, registered native RED→GREEN and exact Windows Full/Docs gates. OWNER approval of implementation and merge is independent.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This change is a governance-only proposal with no shipped functionality. Any later accepted E1/E2 production implementation must update the internal as-built and paired PL/EN product documentation as declared in the parent active R13-A2 Work Contract, then regenerate the Product Browser using `./ss2.ps1 docs`.
