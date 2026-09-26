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

During Sketch Select the grammar is shared by Line/Circle/Arc:

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
Right click                  → reserved; controlled no-op
```

Rectangle results are canonicalized as semantic EntityIds and provider result order never chooses primary.

Sketch point/rectangle queries are independent from mutable OCCT selection. Lines are projected as segments; Circle/Arc use deterministic semantic curve presentations tessellated only for runtime drawing/query. Point queries return the nearest eligible semantic token inside bounded screen tolerance. Window requires the projected authored presentation to be fully contained; Crossing accepts intersection/touch/containment. Built-in references, grid, intrinsic Sketch Origin and transient preview are excluded.

Pointer routing and cursor mode remain independent runtime axes. Sketch edit uses `spatial_tool_input`; Select projects the pick-box cursor, while Line/Circle/Arc creation and direct edit use the crosshair path. Toolbar, Operations, Command Line and keyboard handlers are adapters to the same interaction authority.

<!-- section-id: internal.cad-workbench-viewer.viewer-boundary -->
## Viewer provider boundary

`IDocumentViewport` remains provider-neutral. It exposes camera/navigation, reference scene, authored Sketch scene, transient preview scene, presentation selection, neutral pointer transport, primary-pointer routing/cursor mode, Sketch point/rectangle queries, finite Sketch grip scene/query, runtime hover/active-grip presentation and the selection-box overlay channel.

The authored Sketch scene carries one semantic presentation token per entity. Line uses one segment; Circle and Arc use one token plus an ordered finite point chain. Qt/OCCT may display a curve as multiple derived native segments, but all of those segments map back to the single semantic token.

Grip keys are `PresentationToken + SketchGripRole` only inside the Viewer boundary. PartViewportController immediately maps them back to `SketchId + EntityId + semantic role`. No Qt/OCCT handle becomes CAD identity.

The production executable creates the concrete Qt/OCCT provider only in the composition root and injects it as a neutral `ViewportSurface`. Provider-native exceptions are contained at the Viewer boundary and fail closed.

<!-- section-id: internal.cad-workbench-viewer.sketch-edit -->
## Part Sketch host and 3D edit context

SK-01 through SK-05A established durable Sketch hosting, provider-neutral presentation/input, Select/Line interaction and direct manipulation. SK-06A extends that same path with Circle and Arc rather than introducing another editor authority.

In Part modeling the editor toolbar provides `Sketch`. In Sketch edit it provides checkable `Select`, `Line`, `Circle` and `Arc`. Operations remains contextual: Select shows selection/Delete; creation tools show their current point stage plus Finish/Cancel for that tool; `Finish Sketch` exits the entire edit context. The compact Command Line below the viewport accepts `SELECT`, `LINE`, `CIRCLE` and `ARC`.

Sketch creation still supports only XY/XZ/YZ built-in Origin planes and runs through `CreatePartSketchCommand`, DocumentSession and a Part transaction. Entering edit aligns the initial camera/grid to the Sketch frame without locking later Pan/Zoom/Orbit/ViewCube navigation.

The active Sketch authored scene now presents Line/Circle/Arc plus the intrinsic runtime Origin marker. Circle and Arc are canonical semantic entities; runtime tessellation exists only in presentation/preview.

Creation adapters use the same resolved-input seam:

- Line: continuous first/next point;
- Circle: Center then Radius; exact zero radius creates no command;
- Arc: Start, Through, End; duplicate/collinear/invalid triples create no command.

Each successful primitive creation executes one semantic add command, creates one fresh EntityId, one revision change and one Undo entry. The tool remains active for the next primitive. Pre-existing selection remains stored while creation tools hide/deactivate grips; newly created geometry is not auto-selected.

Select projects grips for every selected editable entity. All grips use the same state-based square marker language: hollow idle, hollow cyan hover, filled yellow active/captured; marker size is DPI/screen-space aware and hit tolerance is independent from visible pixels. Line exposes Start/Center/End; Circle exposes Center and four quadrants; Arc exposes Center/Start/End/Mid.

Center grips start common Move over the complete frozen mixed selection. Line Start/End, Circle quadrant and Arc Start/End/Mid start owner-only reshape. Preview uses the transient Sketch preview channel and does not change authored state/history. LMB/Enter commits one semantic `UpdateSketchGeometryCommand`; Esc cancels preview while preserving selection. The command validates captured revision and all targeted EntityIds, stages one Part state and commits atomically while preserving edited EntityIds.

Mixed Delete uses one semantic erase command for the selected EntityId set and is all-or-nothing. Undo/Redo first cancels transient creation/manipulation state, then uses ordinary DocumentSession history and reconciles surviving semantic selection against newly rebuilt presentation tokens.

Switching/closing Documents clears runtime edit state safely. If history removes the active Sketch, presentation/preview/grips are cleared and ordinary viewport routing is restored.

Snapping/inference, numeric input, constraints, authored dimensions, R7 common transforms/Copy and broader Sketch support remain later roadmap work.

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

Selection, primary selection, hover, active grip, direct-manipulation session, camera, projection, transient detection, grid presentation, active-Sketch presentation tokens, Sketch Origin overlay, preview scene, Sketch point/rectangle/grip query results, grip scene, selection-box overlay, pointer routing, cursor mode, the active `SketchInteractionState`, Select drag state and Command Line text/prompt state are runtime-only.

Persistent Origin visibility is authored state and is intentionally separate.

Closing/reopening the application recreates runtime view and selection state while preserving saved visibility in the Part file.

The native WB-01A stress regression runs the real Workbench and Qt/OCCT Viewer through 100 iterations combining clicks, empty-space clear, Ctrl-selection, right-click no-op, Show/Hide, Undo/Redo, navigation and repeated narrow/wide resizing.
