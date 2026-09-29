# CAD Workbench and Viewer — As-built

<!-- doc-id: internal.cad-workbench-viewer -->
<!-- document-kind: internal -->

<!-- section-id: internal.cad-workbench-viewer.shell -->
## Shared Workbench shell

`CadWorkbenchShell` is the reusable one-document editor layout containing Document Tree, an editor tool-launch strip above the Editor Surface, Properties, contextual Operations and Status/Diagnostics. Project-level Document Tabs live outside it in `ProjectWorkspaceShell`, so the Workbench shell represents only one active CAD Document editor.

The shell owns layout only. The current Part composition is implemented by `CadWorkbench` plus narrow adapters.

`ProjectWorkspaceShell` owns one visible global Command Line surface plus the Qt keyboard/focus adapter. A provider-neutral `CadInputSession` owns only the runtime text buffer, active generic endpoint and submission transport. The active Workbench/tool owns semantic interpretation. `CadWorkbench` is currently the Part endpoint and adapts the Sketch interaction state; Project Workspace does not know Sketch commands or `PointRequest`.

<!-- section-id: internal.cad-workbench-viewer.active-document -->
## Active document context

ProjectSession may keep several canonical DocumentSessions open, while Project Workspace Shell owns `Workspace | Document(DocumentId)` navigation and Project-level Document Tabs.

`CadWorkbench` receives only the currently active Part `DocumentSession` plus Workspace path context needed for presentation. It does not own ProjectSession, discovery or tabs.

Project-level tab changes ask the host to bind a different open DocumentSession. Navigating to Workspace detaches the Workbench without closing the DocumentSession. Returning to that Document rebinds it.

Camera state is stored in Workbench runtime state keyed by DocumentId so Document → Workspace → Document and inter-Document switching can restore the view while the Project remains open. Project close resets that runtime state.

Detach-before-destroy ordering is mandatory: Tree/Viewport controllers are disconnected from the active DocumentSession before ProjectSession erases it. This prevents runtime cleanup from dereferencing a destroyed session during Document or Project close.

<!-- section-id: internal.cad-workbench-viewer.origin -->
## Origin and reference scene

PartViewportController maps the Part's seven deterministic built-in Origin roles to provider-neutral `ReferencePresentation` objects and runtime presentation tokens.

The reference grid is a Viewer presentation primitive. It is not a Part semantic object and is not selectable.

Persistent Origin visibility comes from PartDocument authored presentation state. Hidden references still exist semantically and remain present in the Tree.

Scene replacement clears native detected/selected state before removing provider presentation objects. A failed native replacement is contained at the provider boundary and leaves provider presentation state coherent instead of propagating a process-level failure.

<!-- section-id: internal.cad-workbench-viewer.selection -->
## Selection authority and input grammar

PartViewportController owns document-scoped built-in-reference selection, while host-neutral `SketchInteractionState` owns active-Sketch semantic selection as `EntityId` values plus optional primary identity.

Qt/OCCT picking/query surfaces emit only runtime `PresentationToken` values. The controller maps Sketch tokens immediately to `SketchId + EntityId`; provider object identity and presentation tokens never enter authored CAD state.

Ordinary Sketch Select keeps the established Line/Circle/Arc grammar:

```text
Left click unselected entity → add it and make it primary
Left click selected entity   → keep membership; make it primary
Ctrl + Left click entity     → toggle EntityId membership
Left click blank             → clear Sketch selection
Ctrl + Left click blank      → keep Sketch selection
Left-to-right drag           → add Window hits
Right-to-left drag           → add Crossing hits
Ctrl + rectangle             → toggle returned EntityIds
Esc in Select                → clear selection
Middle drag                  → Pan
Shift + Middle drag          → Orbit
Mouse wheel                  → Zoom
```

Normal Move/Copy/Rotate/Scale/Mirror commands reuse that same semantic query/selection bridge. Selection-first activation freezes an existing non-empty selection immediately. Command-first activation starts in Select objects mode: click/Ctrl/Window/Crossing collect entities, blank LMB is a no-op, and Enter/Space/RMB completes collection only when non-empty. Grips are hidden/inactive during normal common-transform/COPY stages.

COPY retains the frozen original source selection across repeated placements. New copied entities do not take over selection, so later placements continue to refer to the original source set.

Rectangle results are canonicalized as semantic EntityIds and provider result order never chooses primary. Sketch point/rectangle queries remain independent from mutable OCCT selection.

Pointer routing and cursor mode remain independent runtime axes. Ordinary Select and transform/COPY object collection use the pick-box cursor; creation tools, transform reference/destination points, COPY placement and direct edit use the crosshair path. Toolbar, Operations, Command Line, keyboard and bounded RMB handling are adapters to the same semantic interaction authority.

<!-- section-id: internal.cad-workbench-viewer.viewer-boundary -->
## Viewer provider boundary

`IDocumentViewport` remains provider-neutral. It exposes camera/navigation, reference scene, authored Sketch scene, Profile scene, transient Sketch/Profile preview scenes, presentation selection, neutral pointer transport, primary-pointer routing/cursor mode, Sketch point/rectangle queries, finite Sketch grip scene/query, runtime hover/active-grip presentation and the selection-box overlay channel.

The authored Sketch scene carries one semantic presentation token per entity. Line uses one segment; Circle and Arc use one token plus an ordered finite point chain. It also carries the authored Regular/Construction role as presentation input. Qt/OCCT may display a curve as multiple derived native segments, but all of those segments map back to the single semantic token. Construction uses a dashed native line aspect; dash style is derived presentation only and never semantic identity.

Grip keys are `PresentationToken + SketchGripRole` only inside the Viewer boundary. PartViewportController immediately maps them back to `SketchId + EntityId + semantic role`. Profile presentation tokens are likewise runtime-only and are mapped immediately to Part-owned `ProfileId`. No Qt/OCCT handle becomes CAD identity.

The production executable creates the concrete Qt/OCCT provider only in the composition root and injects it as a neutral `ViewportSurface`. Provider-native exceptions are contained at the Viewer boundary and fail closed.

<!-- section-id: internal.cad-workbench-viewer.sketch-edit -->
## Part Sketch host and 3D edit context

The current Part Sketch editor presents and edits durable Line/Circle/Arc geometry and the Part-owned Profile workflow through the same semantic interaction and transaction path.

In Part modeling the editor toolbar provides `Sketch`. In Sketch edit the tool strip is visibly organized as:

```text
Select

Create
  Line
  Circle
  Arc

Profile
  Profile

Modify
  Move
  Copy
  Rotate
  Scale
  Mirror
```

The workspace-global Command Line is context-sensitive. In an active Sketch with no semantic input request it can submit `SELECT`, `LINE`, `CIRCLE`, `ARC`, `MOVE`, `COPY`, `ROTATE`, `SCALE`, `MIRROR`, `PROFILE` and `EDITPROFILE`. An active Profile session additionally accepts `ADD`, `SUBTRACT`, `FIND`, `FINISH`, `CANCEL` and the documented option toggles. While a semantic PointRequest is active, that request receives the submitted text before top-level command activation. It may resolve the existing bare Direct Distance scalar; during active direct grip manipulation it may also consume the tool-local token `C` to enable Grip Copy. `C` is not a top-level command alias. Enter consumes one submitted token whether accepted or rejected; an invalid token creates no authored mutation, the active point/tool stage remains authoritative, the editable buffer becomes empty and a runtime diagnostic reports the rejection.

D makes that ownership explicit in code. `application::CadInputSession` remains transport-only: it owns the live buffer, endpoint attachment/lifetime, context-generation binding and diagnostic transport, but it does not know Sketch tools, PointRequest, command keywords or numeric meaning.

The current Sketch semantic text endpoint is `application::SketchCadInputSemanticEndpoint`. It owns PointRequest-before-command precedence, the exact current Sketch command keyword grammar, the bounded active-grip `C` keyword and bare Direct Distance parsing, then dispatches typed semantic actions through `ISketchCadInputSemanticTarget`. `PartSketchInteractionController` implements that target by delegating to the existing tool activation, Grip Copy enable and `submitDirectDistance()` paths. The generic `CadInputSession` still does not know Grip Copy or Sketch command meaning, and no second mutation, transaction or history path is introduced.

`CadWorkbench` remains the current `ICadInputEndpoint` lifetime/presentation adapter. It validates the context generation, supplies the current UI decimal separator as a neutral UTF-8 formatting value, delegates submitted text to the Application semantic endpoint and mirrors the returned diagnostic. Raw keyword interpretation and numeric validation no longer live in QWidget code.

For accepted top-level Sketch commands, the Qt adapter preserves the pre-D presentation behavior by returning keyboard focus to the viewport after semantic activation. Direct Distance submissions remain owned by the active PointRequest path and do not introduce an extra top-level focus transfer.

The Command Line is keyboard-first. With a normal CAD surface such as the viewport focused, printable unmodified text is appended to the same runtime buffer and mirrored immediately in Command Line without transferring Qt focus. Clicking Command Line remains an equivalent adapter to the same buffer. Real text editors, editable properties, modal dialogs and application shortcuts keep their own keyboard ownership.

Each live token is bound to an opaque `CadInputContextGeneration` supplied by the active semantic endpoint. The Sketch adapter advances that generation when the owning semantic context/request is replaced — for example tool/stage/base-request replacement, active-grip edit-mode replacement, authored revision change, entering/leaving a Sketch or switching the active endpoint. The live buffer and diagnostic are cleared when the observed generation changes. Pointer-direction movement inside the same PointRequest does not advance the semantic context and therefore does not clear a partially entered value. On submit, the generation that owned the token is passed back to the endpoint and is validated before token interpretation or any domain effect.

The QApplication-level event filter is global only as a transport hook; capture authority is Workspace-local. Printable CAD input is accepted only when the active top-level window is the owning Workspace, the focus and key target belong to its active Document Workbench, no popup/menu or modal surface owns the keyboard, the focus is not a real text editor, and an active CAD endpoint exists. A foreign non-modal window or another visible Workspace therefore cannot feed the background buffer.

Delete follows the same precedence rule as the rest of live CAD input. With normal CAD focus and a non-empty viewport-entered buffer, Delete is consumed by the input layer and does not reach semantic Delete Selection; the append-only viewport buffer is left unchanged and Backspace remains the character-removal key. With an empty buffer, existing Delete Selection behavior is unchanged. With direct QLineEdit/text-editor focus, Delete remains ordinary local text editing.

The Command Line presentation is a geometry-stable single row. Its diagnostic region is permanently reserved and single-line; a long message is elided in place (with the full message available as tooltip) instead of wrapping, changing the input width or shrinking/moving the Viewer above it.

`PartSketchInteractionController` owns one runtime-only **last repeatable command** identity for the active Sketch edit session. Successful explicit activation of Line/Circle/Arc/Move/Copy/Rotate/Scale/Mirror through toolbar or Command Line updates that one value. Select, Delete, selection changes, grip/direct manipulation, Undo/Redo and Esc do not replace it.

In ordinary Select with viewport CAD focus, Enter or Space repeats that remembered command through the same existing activation methods. Repeat starts a fresh command invocation: it uses the current semantic selection and does not replay prior Base/Reference/axis/placement points, prior selection snapshots, preview state or copied EntityIds. An empty remembered state is a no-op. The remembered identity is cleared by Sketch edit begin/end, so it does not leak across Sketches, Documents or reopen.

Existing key precedence remains authoritative and now sits below the global live-buffer rule. When the CAD input buffer is non-empty, Enter submits it, Backspace edits it and Esc clears it before any tool cancellation. When the buffer is empty, existing viewport semantics remain unchanged: active grip Space cycles semantic `DirectEditMode`, ordinary-Select Enter/Space may Repeat Last Command, transform Select objects Enter/Space completes collection, final-stage Enter commits, Delete edits selection and hierarchical Esc cancels the active interaction. Space in a real text editor remains text. Ordinary-Select RMB context remains outside the current surface.

Grip Copy is runtime mutation policy inside the existing direct-manipulation session, not a third `DirectEditMode`. `C` + Enter while a grip owns the active PointRequest enables it. In Move mode the complete frozen selection is duplicated; in Reshape mode only the active-grip owner is duplicated. The preview geometry is the same existing Reshape/Move preview; only commit policy changes from update-in-place to the existing `DuplicateSketchGeometryCommand`. Originals remain selected, copies receive fresh IDs, repeated placements keep the interaction-start source/pivot, mode change through Space turns Copy OFF, and repeated numeric placement requires a newly established pointer direction.

Move/Copy/Rotate/Scale/Mirror share one frozen-selection common-transform interaction pipeline. Selection-first activation skips object collection. Command-first activation collects objects with the ordinary semantic selection grammar and freezes the affected/source EntityIds before reference-point stages begin.

Move uses Base Point → destination and previews one translation `destination - base`.

COPY also uses Base Point → placement point, but commit duplicates rather than edits. Preview is computed from the original frozen mixed Line/Circle/Arc source. An accepted non-zero placement executes one `DuplicateSketchGeometryCommand`, leaves the originals unchanged and selected, creates fresh EntityIds for every duplicate, refreshes the authored scene and remains in COPY for another placement. Each later placement uses the same source snapshot and Base Point, not the previously created copy.

Exact zero displacement is intentionally not a COPY commit. It allocates no EntityIds, changes no revision/dirty/history state and leaves COPY active at the placement stage.

Each accepted repeated placement is a separate Part transaction and Undo entry. Undo/Redo is a boundary for an active transient COPY session: the transient preview is cancelled first, then ordinary global history runs. Redo restores the same copied identities. `DocumentSession` preserves the Sketch identity high-water through Undo so a new COPY in the same session cannot reuse the undone IDs. Undo back to the saved authored state remains clean; when a later committed copy is saved, its state persists the preserved high-water through the existing schema-v6 `next_entity_id` field.

Rotate uses Base Point → Reference Point → destination. The Reference Point must differ from Base. The preview uses the signed angle between the reference and destination vectors in the Sketch frame, with positive counter-clockwise rotation. A zero-angle completion is a clean no-op.

Scale uses Base Point → Reference Point → destination. The factor is the ratio of destination distance to reference distance about Base. Only finite factors `> 0` are valid; values between 0 and 1 reduce geometry, factor 1 is a clean no-op, and zero/negative scale does not commit.

Mirror uses first axis point → second axis point. The two points must be distinct and define an infinite mirror line. Reflection preserves EntityIds; reflected Arcs reverse signed sweep orientation so the authored directed arc matches the reflected geometry. Geometry that is exactly unchanged by the chosen axis completes as a no-op.

Pointer movement updates runtime-only preview from the interaction-start geometry snapshot. LMB at the final point stage or Enter commits the current valid transform/placement.

Direct Distance is available at point stages that have an established semantic base: Line next point, grip Reshape/Move (including when Grip Copy is ON), normal MOVE destination and normal COPY placement. The user establishes a free pointer direction, types a bare non-negative distance in Command Line and presses Return. The shared resolver computes `base + normalize(pointer - base) * distance`; the resulting point then enters the same operation path as a clicked point. `.` is always accepted as decimal separator and the current UI-locale decimal separator is accepted as well. Missing/zero pointer direction, malformed text or non-finite values fail closed.

The UI locale is adapter context only: Qt supplies its decimal separator to the neutral Application parser, while `parseBareCadDistance` owns the accepted/rejected numeric grammar without depending on QString or QLocale.

Numeric Rotate angle, Scale factor, Cartesian/polar coordinate entry, unit expressions, Dynamic Input, Ortho/Polar and snapping are not implemented. Viewport printable text may feed the workspace CAD input buffer, while the active semantic request/tool remains the only authority that can interpret or accept the submitted token.

Non-COPY transforms preserve existing EntityIds. COPY allocates fresh identities only at accepted placement commit; preview and cancelled/zero placements consume none. Save/Close/Reopen uses current Part schema v6 and persists entity roles, `next_entity_id`, Profiles/RegionIntent and `next_profile_id` together with authored Sketch geometry.

Center grips remain Move-only and translate the complete frozen selection using the grip's interaction-start location as implicit base.

Line Start/End, Circle quadrant and Arc Start/End/Mid grips still default to owner-only Reshape. While one of those grips is active, viewport Space cycles `Reshape ↔ Move` without ending the DirectManipulationSession. The semantic state preserves the same active grip, interaction-start pivot, frozen selection and current resolved pointer. Reshape preview is recomputed from the interaction-start owner geometry; Move preview is recomputed from the interaction-start complete selection geometry. Cycling itself creates no authored mutation, revision, dirty-state or history entry, and Operations presents the active state as `Grip — Reshape` or `Grip — Move`.

LMB or Enter commits the geometry for the currently active direct-edit mode through the existing semantic geometry-update command/Part transaction path. Esc cancels the complete transient manipulation and preserves selection. Grip Copy modifier and Rotate/Scale/Mirror+Copy remain outside the current surface.

Switching tools or Documents clears uncommitted transform/COPY/direct-manipulation state safely. Space inside text-entry focus remains text input.

<!-- section-id: internal.cad-workbench-viewer.navigation -->
## Navigation and provider-surface Navigation Cube

The shared viewport supports middle-button Pan, Shift + middle-button Orbit, wheel Zoom, Fit All, Orthographic/Perspective projection and a provider-surface 3D Navigation Cube.

The visible Cube is rendered and hit-tested inside the Qt/OCCT native graphics surface. The former QWidget ViewCube is no longer a competing product authority. The common Viewer boundary carries only provider-neutral navigation actions; Qt/OCCT owns Cube pixels, labels, DPI anchoring, hit testing and animation presentation.

The Cube follows the current camera orientation relative to the model/world axes. Its six labeled faces, 12 edges and 8 corners select canonical orientations. Face/edge/corner transitions are animated. When a canonical face is active, adjacent-view arrows perform exact 90-degree view changes and CW/CCW controls perform exact 90-degree roll. Home selects Top-Front-Right isometric orientation and Fit All.

Camera orientation and projection are independent. Cube orientation actions and Home preserve the current projection mode. A separate provider-surface `ORTHO/PERSP` control next to the Cube changes only the projection; it does not change camera direction, target, up vector or Fit state. The Cube itself remains the same orientation manipulator in both projection modes.

Navigation changes are runtime-only and do not increment DocumentRevision, set needsSave or create CAD Undo entries.

<!-- section-id: internal.cad-workbench-viewer.provider-presentation -->
## Current OCCT presentation

The provider presents visible Origin references, a non-selectable reference grid, active-Sketch authored Line/Circle/Arc geometry, intrinsic Sketch Origin, valid visible Part Profiles, transient Sketch/Profile preview, runtime selection box, selected/primary/hover emphasis and finite semantic grips.

Circle/Arc authored geometry is carried as one semantic curve presentation token with an ordered point chain; Qt/OCCT derives line-segment presentation objects without promoting those provider segments to semantic identity. Point/rectangle query likewise evaluates those derived projected segments but returns only the semantic token.

Sketch grips are provider-only `AIS_Point` presentations deactivated from native OCCT selection and hit-tested separately in logical screen space. Their point aspects use custom square bitmaps: hollow for idle, cyan-emphasized hollow for hover and slightly larger filled yellow for active/captured. Marker dimensions are rebuilt for device-pixel ratio changes. Grip query runs before authored geometry query so an overlapping visible grip wins.

The Sketch selection box remains OCCT `AIS_RubberBand` in the same native graphics surface. Profile regions are derived planar OCCT faces with preserved holes, boundary display and transparency/selection styling. Profile fill uses an unlit shading model so its semantic presentation color does not depend on camera/light angle, plus provider-only polygon depth bias to avoid coplanar z-fighting; neither setting changes authored geometry. Native detection maps the picked Profile object to its neutral runtime token and then immediately to ProfileId. The separate Profile preview scene can present both the resulting draft and an optional hovered-candidate emphasis region; Subtract keeps the result cyan and overlays the candidate in red-orange. Principal reference planes remain finite provider presentation geometry. No native provider object is durable CAD identity.

There is still no modeled Part B-Rep at this milestone; the OCCT provider remains presentation/navigation infrastructure.

<!-- section-id: internal.cad-workbench-viewer.presentation-recovery -->
## Presentation failure and recovery

Authored CAD state remains authoritative when provider presentation fails. A semantic command commits through the existing validation/transaction/history path first; rebuilding reference/Sketch presentation is a later runtime operation and cannot roll that successful command back.

`PartViewportController` records a runtime-only degraded-presentation state when the full authored/reference refresh cannot be applied by the bool-returning Viewer boundary. `CadWorkbench` surfaces that state in Status/Diagnostics with the bounded message `3D presentation update failed. Authored CAD state remains authoritative; the next full refresh will retry.`

The next normal full refresh reconstructs reference and authored Sketch presentation from the current `DocumentSession`. Only a complete successful refresh clears the degraded state and diagnostic. Failure and recovery do not change DocumentRevision, dirty state, Undo/Redo depth or durable identity. Transient preview failure likewise creates no authored mutation.

E2 Release evidence retained the current full authored-scene replacement strategy. The measured runner showed an approximately 33 ms native presentation floor through roughly 1,000 line objects and approximately 83 ms around 4,000–5,000 native presentation objects; a 5,000-line accepted mutation plus full authored refresh measured about 150 ms median. These values are engineering evidence, not Product latency guarantees. Large/highly segmented Sketches remain a documented scale risk; no differential-update or public `IDocumentViewport` expansion was introduced.

Circle/Arc sampled point chains, native line segments and Profile face tessellation remain presentation-only derived data. They never define closure, intersections, loops, regions or Profile identity. Shared 2D region analysis operates on accepted authored/evaluated 2D geometry outside the Viewer; Part alone owns durable ProfileId/RegionIntent semantics.

<!-- section-id: internal.cad-workbench-viewer.runtime -->
## Runtime lifetime and stress coverage

Selection, primary selection, hover, active grip, current DirectEditMode, the direct-manipulation interaction-start owner geometry plus complete frozen-selection geometry, common-transform/COPY tool and stage, frozen source selection/geometry snapshot, Base/Reference/axis points, active PointRequest, shared pointer candidate, Direct Distance resolution, current pointer-derived preview, the last repeatable Sketch command identity, camera, projection, transient detection, grid presentation, active-Sketch presentation tokens, Sketch Origin overlay, preview scene, Sketch point/rectangle/grip query results, grip scene, selection-box overlay, pointer routing, cursor mode, the active `SketchInteractionState`, Select/transform drag state and workspace CAD input buffer/prompt/diagnostic state are runtime-only.

Persistent Origin visibility, authored Sketch geometry and entity roles, committed copied entities, model-local identity high-water, Part Profiles/RegionIntent and ProfileId high-water are authored/durable state. COPY and Profile hover/draft preview are never persisted.

Closing/reopening the application recreates runtime view/interaction state while preserving saved authored geometry, copied EntityIds, the persisted next-identity cursor and visibility in the Part file.

The native Workbench/Qt-OCCT stress regression continues to exercise repeated selection, navigation, resizing and lifecycle operations. COPY does not introduce a provider-owned duplication state or second semantic selection authority.

