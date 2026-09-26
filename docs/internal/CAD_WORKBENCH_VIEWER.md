# CAD Workbench and Viewer — As-built

<!-- doc-id: internal.cad-workbench-viewer -->
<!-- document-kind: internal -->

<!-- section-id: internal.cad-workbench-viewer.shell -->
## Shared Workbench shell

WB-01 implements one reusable `CadWorkbenchShell` containing Document Tree, an editor tool-launch strip above the Editor Surface, Properties, contextual Operations and Status/Diagnostics.

WS-01 moves Project-level Document Tabs out of `CadWorkbenchShell` and into Project Workspace Shell. The Workbench shell therefore represents only one active CAD Document editor.

The shell owns layout only. The current Part composition is implemented by `CadWorkbench` plus narrow adapters.

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

Normal MOVE reuses that same semantic query/selection bridge. Selection-first MOVE freezes an existing non-empty selection immediately. Command-first MOVE starts with an empty selection in Select objects mode: click/Ctrl/Window/Crossing collect entities, blank LMB is a no-op, and Enter/Space/RMB completes collection only when non-empty. Grips are hidden/inactive during normal MOVE stages.

Rectangle results are canonicalized as semantic EntityIds and provider result order never chooses primary. Sketch point/rectangle queries remain independent from mutable OCCT selection.

Pointer routing and cursor mode remain independent runtime axes. Ordinary Select and MOVE object collection use the pick-box cursor; creation tools, MOVE Base Point/destination and direct edit use the crosshair path. Toolbar, Operations, Command Line, keyboard and bounded RMB handling are adapters to the same semantic interaction authority.

<!-- section-id: internal.cad-workbench-viewer.viewer-boundary -->
## Viewer provider boundary

`IDocumentViewport` remains provider-neutral. It exposes camera/navigation, reference scene, authored Sketch scene, transient preview scene, presentation selection, neutral pointer transport, primary-pointer routing/cursor mode, Sketch point/rectangle queries, finite Sketch grip scene/query, runtime hover/active-grip presentation and the selection-box overlay channel.

The authored Sketch scene carries one semantic presentation token per entity. Line uses one segment; Circle and Arc use one token plus an ordered finite point chain. Qt/OCCT may display a curve as multiple derived native segments, but all of those segments map back to the single semantic token.

Grip keys are `PresentationToken + SketchGripRole` only inside the Viewer boundary. PartViewportController immediately maps them back to `SketchId + EntityId + semantic role`. No Qt/OCCT handle becomes CAD identity.

The production executable creates the concrete Qt/OCCT provider only in the composition root and injects it as a neutral `ViewportSurface`. Provider-native exceptions are contained at the Viewer boundary and fail closed.

<!-- section-id: internal.cad-workbench-viewer.sketch-edit -->
## Part Sketch host and 3D edit context

SK-01 through SK-06A established durable Sketch hosting, provider-neutral presentation/input, Line/Circle/Arc creation, selection and direct manipulation. SK-07A adds normal MOVE through the same semantic interaction and transaction path.

In Part modeling the editor toolbar provides `Sketch`. In Sketch edit it provides checkable `Select`, `Line`, `Circle`, `Arc` and `Move`. The compact Command Line accepts `SELECT`, `LINE`, `CIRCLE`, `ARC` and `MOVE`.

Creation behavior is unchanged. For MOVE:

- selection-first: activate Move with a non-empty semantic selection and immediately enter Specify Base Point;
- command-first: activate Move with empty selection, collect objects by point/Ctrl/Window/Crossing, then Enter/Space/RMB to continue;
- Base Point: accept any finite resolved Sketch-local point without authored mutation;
- destination: preview the frozen mixed Line/Circle/Arc set using one delta `destination - base`;
- LMB or Enter commits a valid destination; Esc cancels and preserves the affected selection;
- zero-delta completion returns to Select without revision, dirty-state or history change.

The provider-independent translation core is shared by normal MOVE and Center-grip Move. Center grips still move the complete frozen selection using the grip start as implicit base. Owner-only reshape semantics for Line/Circle/Arc are unchanged.

A non-zero accepted MOVE executes one semantic `UpdateSketchGeometryCommand`, validates the captured revision and all frozen EntityIds, stages one Part state, commits atomically, increments revision once and creates one Undo entry. EntityIds are preserved. Undo/Redo first cancels transient MOVE/direct manipulation, then runs ordinary DocumentSession history and reconciles surviving semantic selection.

Switching tools or Documents clears uncommitted MOVE state safely. Save/Close/Reopen uses unchanged Part schema v4; moved canonical geometry and EntityIds persist without a migration.

Space inside text-entry focus remains text input. SK-07A does not activate Rotate/Scale/Mirror/Copy, semantic Space CycleEditMode, ordinary-Select RMB context or Repeat Last Command.

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

Selection, primary selection, hover, active grip, direct-manipulation session, normal MOVE stage/frozen snapshot/Base Point/current destination, camera, projection, transient detection, grid presentation, active-Sketch presentation tokens, Sketch Origin overlay, preview scene, Sketch point/rectangle/grip query results, grip scene, selection-box overlay, pointer routing, cursor mode, the active `SketchInteractionState`, Select/MOVE drag state and Command Line text/prompt state are runtime-only.

Persistent Origin visibility and authored Sketch geometry remain separate domain state.

Closing/reopening the application recreates runtime view/interaction state while preserving saved authored geometry and visibility in the Part file.

The native WB-01A stress regression continues to exercise the real Workbench and Qt/OCCT Viewer through repeated selection, navigation, resizing and lifecycle operations; SK-07A does not create a second provider-owned transform or selection state.
