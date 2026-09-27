# CAD Workbench and Viewer — As-built

<!-- doc-id: internal.cad-workbench-viewer -->
<!-- document-kind: internal -->

<!-- section-id: internal.cad-workbench-viewer.shell -->
## Shared Workbench shell

WB-01 implements one reusable `CadWorkbenchShell` containing Document Tree, an editor tool-launch strip above the Editor Surface, Properties, contextual Operations and Status/Diagnostics.

WS-01 moves Project-level Document Tabs out of `CadWorkbenchShell` and into Project Workspace Shell. The Workbench shell therefore represents only one active CAD Document editor.

The shell owns layout only. The current Part composition is implemented by `CadWorkbench` plus narrow adapters.

WB-02 moves Command Line ownership one level above the active Document Workbench. `ProjectWorkspaceShell` owns one visible global Command Line surface plus the Qt keyboard/focus adapter. A provider-neutral `CadInputSession` owns only the runtime text buffer, active generic endpoint and submission transport. The active Workbench/tool still owns semantic interpretation. `CadWorkbench` is currently the first endpoint and adapts the existing Sketch interaction state; Project Workspace does not know Sketch commands or `PointRequest`.

WB-01A stabilized the existing shell/Viewer interaction without adding a new CAD domain.

<!-- section-id: internal.cad-workbench-viewer.active-document -->
## Active document context

ProjectSession may keep several canonical DocumentSessions open, while Project Workspace Shell owns `Workspace | Document(DocumentId)` navigation and Project-level Document Tabs.

`CadWorkbench` receives only the currently active Part `DocumentSession` plus Workspace path context needed for presentation. It does not own ProjectSession, discovery or tabs.

Project-level tab changes ask the host to bind a different open DocumentSession. Navigating to Workspace detaches the Workbench without closing the DocumentSession. Returning to that Document rebinds it.

Camera state is stored in Workbench runtime state keyed by DocumentId so Document → Workspace → Document and inter-Document switching can restore the view while the Project remains open. Project close resets that runtime state.

SK-01A/WS-01 require detach-before-destroy ordering: Tree/Viewport controllers are disconnected from the active DocumentSession before ProjectSession erases it. This prevents runtime cleanup from dereferencing a destroyed session during Document or Project close.

<!-- section-id: internal.cad-workbench-viewer.origin -->
## Origin and reference scene

PartViewportController maps the Part's seven deterministic built-in Origin roles to provider-neutral `ReferencePresentation` objects and runtime presentation tokens.

The reference grid is a Viewer presentation primitive. It is not a Part semantic object and is not selectable.

Persistent Origin visibility comes from PartDocument authored presentation state. Hidden references still exist semantically and remain present in the Tree.

WB-01A scene replacement clears native detected/selected state before removing provider presentation objects. A failed native replacement is contained at the provider boundary and leaves provider presentation state coherent instead of propagating a process-level failure.

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

`IDocumentViewport` remains provider-neutral. It exposes camera/navigation, reference scene, authored Sketch scene, transient preview scene, presentation selection, neutral pointer transport, primary-pointer routing/cursor mode, Sketch point/rectangle queries, finite Sketch grip scene/query, runtime hover/active-grip presentation and the selection-box overlay channel.

The authored Sketch scene carries one semantic presentation token per entity. Line uses one segment; Circle and Arc use one token plus an ordered finite point chain. Qt/OCCT may display a curve as multiple derived native segments, but all of those segments map back to the single semantic token.

Grip keys are `PresentationToken + SketchGripRole` only inside the Viewer boundary. PartViewportController immediately maps them back to `SketchId + EntityId + semantic role`. No Qt/OCCT handle becomes CAD identity.

The production executable creates the concrete Qt/OCCT provider only in the composition root and injects it as a neutral `ViewportSurface`. Provider-native exceptions are contained at the Viewer boundary and fail closed.

<!-- section-id: internal.cad-workbench-viewer.sketch-edit -->
## Part Sketch host and 3D edit context

The current Part Sketch editor presents and edits durable Line/Circle/Arc geometry through the same semantic interaction and transaction path.

In Part modeling the editor toolbar provides `Sketch`. In Sketch edit the tool strip is visibly organized as:

```text
Select

Create
  Line
  Circle
  Arc

Modify
  Move
  Copy
  Rotate
  Scale
  Mirror
```

The workspace-global Command Line is context-sensitive. In an active Sketch with no semantic input request it can submit the existing command keywords `SELECT`, `LINE`, `CIRCLE`, `ARC`, `MOVE`, `COPY`, `ROTATE`, `SCALE` and `MIRROR`. While a semantic PointRequest is active, that request receives the submitted text before top-level command activation. It may resolve the existing bare Direct Distance scalar. Enter consumes one submitted token whether accepted or rejected; an invalid token creates no authored mutation, the active point/tool stage remains authoritative, the editable buffer becomes empty and a runtime diagnostic reports the rejection.

WB-02 makes that surface keyboard-first. With a normal CAD surface such as the viewport focused, printable unmodified text is appended to the same runtime buffer and mirrored immediately in Command Line without transferring Qt focus. Clicking Command Line remains an equivalent adapter to the same buffer. Real text editors, editable properties, modal dialogs and application shortcuts keep their own keyboard ownership.

AUDIT-01 Package A hardens that routing boundary. Each live token is bound to an opaque `CadInputContextGeneration` supplied by the active semantic endpoint. The Sketch adapter advances that generation when the owning semantic context/request is replaced — for example tool/stage/base-request replacement, active-grip edit-mode replacement, authored revision change, entering/leaving a Sketch or switching the active endpoint. The live buffer and diagnostic are cleared when the observed generation changes. Pointer-direction movement inside the same PointRequest does not advance the semantic context and therefore does not clear a partially entered value. On submit, the generation that owned the token is passed back to the endpoint and is validated before token interpretation or any domain effect.

The QApplication-level event filter is global only as a transport hook; capture authority is Workspace-local. Printable CAD input is accepted only when the active top-level window is the owning Workspace, the focus and key target belong to its active Document Workbench, no popup/menu or modal surface owns the keyboard, the focus is not a real text editor, and an active CAD endpoint exists. A foreign non-modal window or another visible Workspace therefore cannot feed the background buffer.

Delete follows the same precedence rule as the rest of live CAD input. With normal CAD focus and a non-empty viewport-entered buffer, Delete is consumed by the input layer and does not reach semantic Delete Selection; the append-only viewport buffer is left unchanged and Backspace remains the character-removal key. With an empty buffer, existing Delete Selection behavior is unchanged. With direct QLineEdit/text-editor focus, Delete remains ordinary local text editing.

The Command Line presentation is a geometry-stable single row. Its diagnostic region is permanently reserved and single-line; a long message is elided in place (with the full message available as tooltip) instead of wrapping, changing the input width or shrinking/moving the Viewer above it.

`PartSketchInteractionController` owns one runtime-only **last repeatable command** identity for the active Sketch edit session. Successful explicit activation of Line/Circle/Arc/Move/Copy/Rotate/Scale/Mirror through toolbar or Command Line updates that one value. Select, Delete, selection changes, grip/direct manipulation, Undo/Redo and Esc do not replace it.

In ordinary Select with viewport CAD focus, Enter or Space repeats that remembered command through the same existing activation methods. Repeat starts a fresh command invocation: it uses the current semantic selection and does not replay prior Base/Reference/axis/placement points, prior selection snapshots, preview state or copied EntityIds. An empty remembered state is a no-op. The remembered identity is cleared by Sketch edit begin/end, so it does not leak across Sketches, Documents or reopen.

Existing key precedence remains authoritative and now sits below the global live-buffer rule. When the CAD input buffer is non-empty, Enter submits it, Backspace edits it and Esc clears it before any tool cancellation. When the buffer is empty, existing viewport semantics remain unchanged: active grip Space cycles semantic `DirectEditMode`, ordinary-Select Enter/Space may Repeat Last Command, transform Select objects Enter/Space completes collection, final-stage Enter commits, Delete edits selection and hierarchical Esc cancels the active interaction. Space in a real text editor remains text. Ordinary-Select RMB context remains outside the current surface.


Move/Copy/Rotate/Scale/Mirror share one frozen-selection common-transform interaction pipeline. Selection-first activation skips object collection. Command-first activation collects objects with the ordinary semantic selection grammar and freezes the affected/source EntityIds before reference-point stages begin.

Move uses Base Point → destination and previews one translation `destination - base`.

COPY also uses Base Point → placement point, but commit duplicates rather than edits. Preview is computed from the original frozen mixed Line/Circle/Arc source. An accepted non-zero placement executes one `DuplicateSketchGeometryCommand`, leaves the originals unchanged and selected, creates fresh EntityIds for every duplicate, refreshes the authored scene and remains in COPY for another placement. Each later placement uses the same source snapshot and Base Point, not the previously created copy.

Exact zero displacement is intentionally not a COPY commit. It allocates no EntityIds, changes no revision/dirty/history state and leaves COPY active at the placement stage.

Each accepted repeated placement is a separate Part transaction and Undo entry. Undo/Redo is a boundary for an active transient COPY session: the transient preview is cancelled first, then ordinary global history runs. Redo restores the same copied identities. `DocumentSession` preserves the Sketch identity high-water through Undo so a new COPY in the same session cannot reuse the undone IDs. Undo back to the saved authored state remains clean; when a later committed copy is saved, its state persists the preserved high-water through the existing schema-v4 `next_entity_id` field.

Rotate uses Base Point → Reference Point → destination. The Reference Point must differ from Base. The preview uses the signed angle between the reference and destination vectors in the Sketch frame, with positive counter-clockwise rotation. A zero-angle completion is a clean no-op.

Scale uses Base Point → Reference Point → destination. The factor is the ratio of destination distance to reference distance about Base. Only finite factors `> 0` are valid; values between 0 and 1 reduce geometry, factor 1 is a clean no-op, and zero/negative scale does not commit.

Mirror uses first axis point → second axis point. The two points must be distinct and define an infinite mirror line. Reflection preserves EntityIds; reflected Arcs reverse signed sweep orientation so the authored directed arc matches the reflected geometry. Geometry that is exactly unchanged by the chosen axis completes as a no-op.

Pointer movement updates runtime-only preview from the interaction-start geometry snapshot. LMB at the final point stage or Enter commits the current valid transform/placement.

SK-07F adds Direct Distance to point stages that have an established semantic base: Line next point, grip Reshape/Move, normal MOVE destination and normal COPY placement. The user establishes a free pointer direction, types a bare non-negative distance in Command Line and presses Return. The shared resolver computes `base + normalize(pointer - base) * distance`; the resulting point then enters the same operation path as a clicked point. `.` is always accepted as decimal separator and the current UI-locale decimal separator is accepted as well. Missing/zero pointer direction, malformed text or non-finite values fail closed.

This does not add numeric Rotate angle, Scale factor, Cartesian/polar coordinate entry, unit expressions, Dynamic Input, Ortho/Polar or snapping. WB-02 changes only transport and focus ownership: viewport printable text may now feed the workspace CAD input buffer, while the active semantic request/tool remains the only authority that can interpret or accept the submitted token.

Non-COPY transforms preserve existing EntityIds. COPY allocates fresh identities only at accepted placement commit; preview and cancelled/zero placements consume none. Save/Close/Reopen uses unchanged Part schema v4 and persists `next_entity_id` together with authored Sketch geometry.

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

The provider presents visible Origin references, a non-selectable reference grid, active-Sketch authored Line/Circle/Arc geometry, intrinsic Sketch Origin, transient creation/direct-edit preview, runtime selection box, selected/primary/hover emphasis and finite semantic grips.

Circle/Arc authored geometry is carried as one semantic curve presentation token with an ordered point chain; Qt/OCCT derives line-segment presentation objects without promoting those provider segments to semantic identity. Point/rectangle query likewise evaluates those derived projected segments but returns only the semantic token.

Sketch grips are provider-only `AIS_Point` presentations deactivated from native OCCT selection and hit-tested separately in logical screen space. Their point aspects use custom square bitmaps: hollow for idle, cyan-emphasized hollow for hover and slightly larger filled yellow for active/captured. Marker dimensions are rebuilt for device-pixel ratio changes. Grip query runs before authored geometry query so an overlapping visible grip wins.

The Sketch selection box remains OCCT `AIS_RubberBand` in the same native graphics surface. Principal reference planes remain finite provider presentation geometry. No native provider object is durable CAD identity.

There is still no modeled Part B-Rep at this milestone; the OCCT provider remains presentation/navigation infrastructure.

<!-- section-id: internal.cad-workbench-viewer.runtime -->
## Runtime lifetime and stress coverage

Selection, primary selection, hover, active grip, current DirectEditMode, the direct-manipulation interaction-start owner geometry plus complete frozen-selection geometry, common-transform/COPY tool and stage, frozen source selection/geometry snapshot, Base/Reference/axis points, active PointRequest, shared pointer candidate, Direct Distance resolution, current pointer-derived preview, the last repeatable Sketch command identity, camera, projection, transient detection, grid presentation, active-Sketch presentation tokens, Sketch Origin overlay, preview scene, Sketch point/rectangle/grip query results, grip scene, selection-box overlay, pointer routing, cursor mode, the active `SketchInteractionState`, Select/transform drag state and workspace CAD input buffer/prompt/diagnostic state are runtime-only.

Persistent Origin visibility, authored Sketch geometry, committed copied entities and the model-local identity high-water are authored/durable state. COPY preview itself is never persisted.

Closing/reopening the application recreates runtime view/interaction state while preserving saved authored geometry, copied EntityIds, the persisted next-identity cursor and visibility in the Part file.

The native Workbench/Qt-OCCT stress regression continues to exercise repeated selection, navigation, resizing and lifecycle operations. COPY does not introduce a provider-owned duplication state or second semantic selection authority.

