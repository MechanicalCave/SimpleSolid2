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

Tree selection updates that authority. Qt/OCCT picking emits only neutral `SelectionIntent` values carrying `PresentationToken`; the controller maps tokens back to semantic built-in roles.

The controller pushes one coherent selection to Tree and Viewer. Primary selection drives Properties.

Current mouse grammar is:

```text
Left click                 → replace semantic selection
Ctrl + Left click          → toggle semantic selection member
Left click on empty space  → clear semantic selection
Middle drag                → Pan
Shift + Middle drag        → Orbit
Mouse wheel                → Zoom
Right click                → reserved; currently controlled no-op where no menu exists
```

Transient OCCT detection/highlight is not semantic selection.

<!-- section-id: internal.cad-workbench-viewer.viewer-boundary -->
## Viewer provider boundary

`IDocumentViewport` is provider-neutral. It exposes camera/navigation, reference scene, presentation selection and selection-intent transport.

`QtOcctViewerWidget` implements that contract behind the `viewer_qt_occt` module.

The production executable creates the concrete provider only in the application composition root and injects it as a neutral `ViewportSurface`.

The concrete provider contains recoverable OCCT/standard exceptions at its public/event boundary, logs the failure and fails closed instead of allowing those exceptions to unwind through Workbench UI.

Boundary tests reject OCCT/provider tokens from domain/application APIs and Workbench UI.

<!-- section-id: internal.cad-workbench-viewer.sketch-edit -->
## Part Sketch host and 3D edit context

SK-01 adds the first durable Part-hosted Sketch lifecycle without adding 2D drawing entities.

The editor toolbar above the 3D Viewport provides the Part `Sketch` launcher. Operations is reserved for the active tool/edit context rather than acting as a permanent tool catalog. During support pick it presents contextual guidance/Cancel; during Sketch edit it presents `Finish Sketch`.

While the Sketch tool is active, the user selects one semantic built-in Origin plane: XY, XZ or YZ. Origin axes/point are rejected. The selected Tree item or Viewer token is only command input; the durable Sketch support is the semantic built-in reference role.

Creation executes through `CreatePartSketchCommand`, DocumentSession validation/history and a Part transaction. One successful creation adds one authored Sketch with stable SketchId, Origin-plane support, explicit SketchPlacement and persistent visibility. Undo removes that Sketch and Redo restores the same SketchId/state.

The active Sketch editor remains the existing Document Viewport. On entry, the runtime grid changes to the Sketch U/V frame, the camera aligns normal to the support plane and Fit is requested. This is only an initial view: normal Pan, Zoom, Orbit and ViewCube navigation remain available immediately afterward and do not mutate SketchPlacement, DocumentRevision or dirty state.

`Finish Sketch` exits only the runtime editor context. The authored Sketch remains in the Part and in the normal Document Tree. An existing Sketch re-enters edit by Tree double-click or `Edit Sketch` context action; the Tree carries only transient SketchId metadata and the Workbench revalidates that stable ID against the active Part before editing. Re-entering edit is runtime-only and does not dirty the Part.

Switching/closing Documents clears the runtime edit context safely.

SK-01 supports only empty Sketches on XY/XZ/YZ Origin planes. Datum/Construction Plane support, planar model-face support, topology naming, 2D entities, constraints and solver behavior remain outside this implementation.

<!-- section-id: internal.cad-workbench-viewer.navigation -->
## Navigation and responsive ViewCube

The shared viewport supports middle-button Pan, Shift + middle-button Orbit, wheel Zoom, Fit All, six orthogonal standard views, Isometric and eight corner orientations, plus Orthographic/Perspective.

`ViewCubeWidget` is an overlay in the shared Editor Surface and invokes only `IDocumentViewport`.

WB-01A replaced the fixed button-grid presentation with a compact responsive CAD navigation control. It tracks its host Editor Surface and switches to a compact mode when space is constrained, so narrow splitter layouts do not force a large Viewer minimum width or overlap the right-side panels.

Because the production Qt/OCCT viewport is a native `WA_PaintOnScreen` child, overlay movement/resize explicitly schedules a coalesced repaint of the underlying Viewer surface. This prevents stale ViewCube pixels from remaining after splitter or right-panel resize without changing the provider-neutral Viewer API.

Navigation changes are runtime-only and do not increment DocumentRevision, set needsSave or create CAD Undo entries.

<!-- section-id: internal.cad-workbench-viewer.provider-presentation -->
## Current OCCT presentation

The provider currently presents Origin point, X/Y/Z axes, principal Origin planes when visible, a non-selectable reference grid and selected/primary visual emphasis.

Principal reference planes use finite provider presentation geometry; no native OCCT presentation object is durable semantic identity.

There is no modeled Part B-Rep in WB-01/WB-01A. The OCCT provider is presentation/navigation infrastructure, not Part evaluation.

<!-- section-id: internal.cad-workbench-viewer.runtime -->
## Runtime lifetime and stress coverage

Selection, primary selection, camera, projection, transient detection and grid presentation are runtime-only.

Persistent Origin visibility is authored state and is intentionally separate.

Closing/reopening the application recreates runtime view and selection state while preserving saved visibility in the Part file.

The native WB-01A stress regression runs the real Workbench and Qt/OCCT Viewer through 100 iterations combining clicks, empty-space clear, Ctrl-selection, right-click no-op, Show/Hide, Undo/Redo, navigation and repeated narrow/wide resizing.
