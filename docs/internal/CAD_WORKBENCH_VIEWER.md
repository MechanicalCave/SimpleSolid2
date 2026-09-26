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

The current Part viewport controller owns document-scoped runtime selected set plus primary selection.

Tree selection updates the existing built-in-reference authority. Qt/OCCT legacy picking emits only neutral `SelectionIntent` values carrying `PresentationToken`; the controller maps reference tokens back to semantic built-in roles. R3 assigns runtime tokens to active-Sketch authored Lines. SK-04B adds explicit point/rectangle Sketch presentation queries: the provider returns only current runtime `PresentationToken` values and `PartViewportController` maps them immediately to `SketchId + EntityId`. Semantic Sketch selection remains owned by the host-neutral `SketchInteractionState` introduced in SK-04A; Viewer tokens never enter it.

The controller pushes one coherent selection to Tree and Viewer. Primary selection drives Properties.

Current mouse grammar depends on semantic context. Outside Sketch edit, built-in reference selection keeps the existing replace/toggle/blank-clear behavior. During Sketch Select, primary pointer input is routed through the Sketch coordinator:

```text
Left click unselected Line → add it and make it primary
Left click selected Line   → keep membership; make it primary
Ctrl + Left click Line     → toggle Sketch EntityId membership
Left click blank           → clear Sketch EntityId selection
Ctrl + Left click blank    → keep Sketch selection
Left-to-right drag         → add Window hits
Right-to-left drag         → add Crossing hits
Ctrl + rectangle           → toggle returned EntityIds
Esc in Select              → clear selection
Middle drag                → Pan
Shift + Middle drag        → Orbit
Mouse wheel                → Zoom
Right click                → reserved; controlled no-op
```

Rectangle results are canonicalized as semantic EntityIds and never use provider result order to choose primary. Existing selection is not implicitly erased.

Transient OCCT detection/highlight is not semantic selection. SK-04B keeps Sketch hit testing independent from the mutable OCCT selection set: authored active-Sketch Lines are projected through the current view and queried in screen space. Point query returns the nearest eligible Line within provider pick tolerance. Rectangle query supports explicit `window` (projected segment fully contained) and `crossing` (projected segment intersects/touches or is contained) rules. Built-in references, grid, intrinsic Sketch Origin and transient preview Lines are outside these Sketch queries.

Pointer routing and cursor mode remain independent runtime axes. Entering Sketch edit now creates one bounded `PartSketchInteractionController`, resets the single `SketchInteractionState` to Select and configures `spatial_tool_input + select_pick_box`. Activating Line keeps spatial routing and switches to `create_edit_crosshair`. The toolbar, Operations, Command Line and keyboard handlers are adapters to that same interaction authority.

<!-- section-id: internal.cad-workbench-viewer.viewer-boundary -->
## Viewer provider boundary

`IDocumentViewport` is provider-neutral. It exposes camera/navigation, the existing reference scene, an active-Sketch authored scene, a separate transient Sketch preview scene, presentation selection, neutral selection-intent transport, neutral spatial-pointer transport, primary-pointer routing, runtime cursor mode, active-Sketch point/rectangle presentation queries, a finite Line-grip scene/query contract, runtime Sketch hover/active-grip presentation and the existing selection-box overlay channel.

SK-05A deliberately extends that finite surface instead of introducing a generic overlay framework. A grip key is `PresentationToken + SketchGripRole` only inside the Viewer boundary. `PartViewportController` immediately maps the owner token back to `SketchId + EntityId` and then to the semantic `LineGripRef`; neither Qt/OCCT object identity nor presentation tokens enter authored CAD state.

`QtOcctViewerWidget` implements that contract behind the `viewer_qt_occt` module. Provider-native Qt events and OCCT view objects do not cross the boundary: spatial input is exported as logical viewport coordinates plus a finite 3D `Ray3` and the smallest neutral modifier state required by the active interaction. SK-04C transports Control as provider-neutral runtime data; Qt modifier enums remain provider-local.

The production executable creates the concrete provider only in the application composition root and injects it as a neutral `ViewportSurface`.

The concrete provider contains recoverable OCCT/standard exceptions at its public/event boundary, logs the failure and fails closed instead of allowing those exceptions to unwind through Workbench UI.

Boundary tests reject OCCT/provider tokens from domain/application APIs and Workbench UI.

<!-- section-id: internal.cad-workbench-viewer.sketch-edit -->
## Part Sketch host and 3D edit context

SK-01 adds the durable Part-hosted Sketch lifecycle. SK-02B embeds durable Shared 2D Line geometry, and SK-03A adds its provider-neutral active-Sketch presentation/input boundary without implementing interactive Line creation.

The editor toolbar above the 3D Viewport is contextual. In Part modeling it provides the Part `Sketch` launcher. Entering Sketch edit replaces that tool surface with `Select` and `Line`, whose checked state is projected from the single interaction state. Operations remains contextual rather than a permanent catalog: during support pick it presents guidance/Cancel; during Sketch Select it presents selection state and Delete Selection; during Line it presents the point prompt plus Finish Line/Cancel Line; `Finish Sketch` remains the whole-context action. Leaving or losing Sketch edit restores the Part modeling surfaces.

A compact Sketch Command Line is hosted directly below the central Viewport while Sketch edit is active. It accepts only `SELECT` and `LINE` in R4 and invokes the same coordinator methods as the toolbar. Recognized submission returns focus to the Viewport; text-focus Esc/Delete remain local to the text control. Global Workbench Status remains a separate diagnostic/result channel.

While the Sketch tool is active, the user selects one semantic built-in Origin plane: XY, XZ or YZ. Origin axes/point are rejected. The selected Tree item or Viewer token is only command input; the durable Sketch support is the semantic built-in reference role.

Creation executes through `CreatePartSketchCommand`, DocumentSession validation/history and a Part transaction. One successful creation adds one authored Sketch with stable SketchId, Origin-plane support, explicit SketchPlacement and persistent visibility. Undo removes that Sketch and Redo restores the same SketchId/state.

The active Sketch editor remains the existing Document Viewport. On entry, the runtime grid changes to the Sketch U/V frame, the camera aligns normal to the support plane and Fit is requested. The active Sketch's authored Lines are transformed from local U/V into 3D presentation, and the intrinsic Sketch Origin is shown as a runtime overlay rather than authored Point geometry. This is only an initial view: normal Pan, Zoom, Orbit and ViewCube navigation remain available immediately afterward and do not mutate SketchPlacement, DocumentRevision or dirty state.

`Finish Sketch` exits only the runtime editor context. The authored Sketch remains in the Part and in the normal Document Tree. An existing Sketch re-enters edit by Tree double-click or `Edit Sketch` context action; the Tree carries only transient SketchId metadata and the Workbench revalidates that stable ID against the active Part before editing. Re-entering edit is runtime-only and does not dirty the Part.

Switching/closing Documents clears the runtime edit context safely. If history removes the Sketch currently named by the runtime edit context, the controller fails closed: active-Sketch presentation and preview are cleared and ordinary viewport routing/cursor state is restored.

R3 also provides an independent transient Line-preview channel and maps provider rays back into active Sketch U/V through the metric `SketchPlacement`. The mapping therefore remains valid after camera orbit and is not based on screen orientation. Preview/ray activity is runtime-only and does not mutate authored state, revision, dirty state or Undo history.

SK-04C wires SK-04A and SK-04B through one bounded Part/UI coordinator. Line accepts points on primary press, renders rubber-band preview on move and executes exactly one `AddSketchLineCommand` for each committed segment. Successful commits refresh authored presentation and advance the continuous anchor; exact-zero candidates create no command/history entry. Select performs point queries on release and Window/Crossing rectangle queries after a logical-pixel drag threshold. Delete executes one atomic `EraseSketchEntitiesCommand` for the current semantic EntityId set.

SK-05A extends that same coordinator with Line hover/grip projection and direct manipulation. All selected Lines expose Start/Center/End grips in Select; grips are hidden while Line creation is active without clearing selection. Grip hit testing has priority over underlying authored geometry. Clicking Start/End starts owner-only Reshape; clicking Center starts Move for the frozen selected EntityId set. The user may release the activating click, move the pointer freely, then commit with a later LMB or Enter. Esc discards only the transient edit and preserves selection; a subsequent Select-mode Esc clears it.

The direct-manipulation start captures the current DocumentRevision. Preview uses the shared resolved-input seam and the existing transient Sketch preview channel; it never edits the live model. Commit emits one semantic `UpdateSketchLinesCommand` containing resulting geometry, validates the captured revision and complete EntityId target set, stages one authored state and commits once. Existing EntityIds are preserved. Exact no-op state creates no revision/history entry; any invalid/missing target or stale revision fails closed with no partial mutation.

Undo/Redo first calls the interaction state's history cancellation, clears preview/box state and returns to Select, then runs ordinary DocumentSession history. If the Sketch survives, selection is reconciled against the current SketchModel and projected through newly rebuilt presentation tokens/grips. If history removes the active Sketch, edit closes fail-closed.

Snapping/inference, coordinate/value entry, constraints, additional primitives, additional edit operations and RMB command grammar remain later roadmap work. Sketch support remains limited to XY/XZ/YZ Origin planes; Datum/Construction Plane and planar model-face support remain later stages.

<!-- section-id: internal.cad-workbench-viewer.navigation -->
## Navigation and provider-surface Navigation Cube

The shared viewport supports middle-button Pan, Shift + middle-button Orbit, wheel Zoom, Fit All, Orthographic/Perspective projection and a provider-surface 3D Navigation Cube.

The visible Cube is rendered and hit-tested inside the Qt/OCCT native graphics surface. The former QWidget ViewCube is no longer a competing product authority. The common Viewer boundary carries only provider-neutral navigation actions; Qt/OCCT owns Cube pixels, labels, DPI anchoring, hit testing and animation presentation.

The Cube follows the current camera orientation relative to the model/world axes. Its six labeled faces, 12 edges and 8 corners select canonical orientations. Face/edge/corner transitions are animated. When a canonical face is active, adjacent-view arrows perform exact 90-degree view changes and CW/CCW controls perform exact 90-degree roll. Home selects Top-Front-Right isometric orientation and Fit All.

Camera orientation and projection are independent. Cube orientation actions and Home preserve the current projection mode. A separate provider-surface `ORTHO/PERSP` control next to the Cube changes only the projection; it does not change camera direction, target, up vector or Fit state. The Cube itself remains the same orientation manipulator in both projection modes.

Navigation changes are runtime-only and do not increment DocumentRevision, set needsSave or create CAD Undo entries.

<!-- section-id: internal.cad-workbench-viewer.provider-presentation -->
## Current OCCT presentation

The provider currently presents Origin point, X/Y/Z axes, principal Origin planes when visible, a non-selectable reference grid, active-Sketch authored Lines, the intrinsic active-Sketch Origin overlay, a separate transient Line/direct-edit preview channel, a runtime Sketch selection-box overlay, selected/primary emphasis, entity hover and finite Line grips with normal/hover/active styling. The Sketch selection-box is rendered by OCCT `AIS_RubberBand` in the same native graphics surface rather than by a translucent QWidget above `WA_PaintOnScreen`.

SK-04B Sketch point/rectangle queries operate from the provider's current 3D→screen projection of authored Sketch Lines rather than creating an OCCT semantic selection set. SK-05A grip objects are likewise provider presentation only: they are deactivated from native OCCT selection and hit-tested in logical screen space with a bounded pixel tolerance. Grip query is evaluated before Line geometry query in Sketch Select so an overlapping visible grip wins without turning provider object identity into CAD identity.

Principal reference planes use finite provider presentation geometry; no native OCCT presentation object is durable semantic identity.

There is no modeled Part B-Rep in WB-01/WB-01A. The OCCT provider is presentation/navigation infrastructure, not Part evaluation.

<!-- section-id: internal.cad-workbench-viewer.runtime -->
## Runtime lifetime and stress coverage

Selection, primary selection, hover, active grip, direct-manipulation session, camera, projection, transient detection, grid presentation, active-Sketch presentation tokens, Sketch Origin overlay, preview scene, Sketch point/rectangle/grip query results, grip scene, selection-box overlay, pointer routing, cursor mode, the active `SketchInteractionState`, Select drag state and Command Line text/prompt state are runtime-only.

Persistent Origin visibility is authored state and is intentionally separate.

Closing/reopening the application recreates runtime view and selection state while preserving saved visibility in the Part file.

The native WB-01A stress regression runs the real Workbench and Qt/OCCT Viewer through 100 iterations combining clicks, empty-space clear, Ctrl-selection, right-click no-op, Show/Hide, Undo/Redo, navigation and repeated narrow/wide resizing.
